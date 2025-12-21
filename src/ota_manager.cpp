#include "ota_manager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

#include "config_manager.h"
#include "web_server.h"

namespace {
constexpr const char* kUserAgent = "BroncoControls/OTA";
constexpr std::uint32_t kMinIntervalMinutes = 5;
constexpr std::uint32_t kMaxIntervalMinutes = 24 * 60;

bool beginsWith(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string sanitizeMd5(const std::string& md5) {
    std::string cleaned;
    cleaned.reserve(md5.size());
    for (char c : md5) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            cleaned.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return cleaned;
}

std::string resolveUrl(const std::string& base_url, const std::string& candidate) {
    if (candidate.empty()) {
        return "";
    }
    if (beginsWith(candidate, "http://") || beginsWith(candidate, "https://")) {
        return candidate;
    }

    if (!candidate.empty() && candidate.front() == '/') {
        const auto proto_pos = base_url.find("//");
        if (proto_pos == std::string::npos) {
            return candidate;
        }
        const auto host_end = base_url.find('/', proto_pos + 2);
        const std::string origin = (host_end == std::string::npos) ? base_url : base_url.substr(0, host_end);
        return origin + candidate;
    }

    const auto last_slash = base_url.find_last_of('/');
    if (last_slash == std::string::npos) {
        return candidate;
    }
    return base_url.substr(0, last_slash + 1) + candidate;
}

std::string readJsonString(JsonVariantConst value) {
    if (value.isNull()) {
        return "";
    }
    if (value.is<const char*>()) {
        return std::string(value.as<const char*>());
    }
    if (value.is<String>()) {
        return std::string(value.as<String>().c_str());
    }
    if (value.is<std::string>()) {
        return value.as<std::string>();
    }
    return "";
}

bool beginHttp(HTTPClient& http, WiFiClientSecure& secure_client, const std::string& url) {
    if (beginsWith(url, "https://")) {
        secure_client.setInsecure();
        return http.begin(secure_client, url.c_str());
    }
    return http.begin(url.c_str());
}
}

OTAUpdateManager& OTAUpdateManager::instance() {
    static OTAUpdateManager mgr;
    return mgr;
}

void OTAUpdateManager::begin() {
    const auto& ota_cfg = ConfigManager::instance().getConfig().ota;
    enabled_ = ota_cfg.enabled;
    auto_apply_ = ota_cfg.auto_apply;
    manifest_url_ = ota_cfg.manifest_url;
    expected_channel_ = ota_cfg.channel.empty() ? "stable" : ota_cfg.channel;
    const std::uint32_t minutes = std::max(kMinIntervalMinutes, std::min(kMaxIntervalMinutes, ota_cfg.check_interval_minutes));
    check_interval_ms_ = minutes * 60UL * 1000UL;
    last_check_ms_ = 0;
    wifi_ready_ = false;
    pending_manual_check_ = false;
    last_status_ = enabled_ ? "waiting-for-wifi" : "disabled";

    if (manifest_url_.empty()) {
        enabled_ = false;
        last_status_ = "missing-manifest-url";
        Serial.println("[OTA] Disabled: manifest URL not configured");
    }
}

void OTAUpdateManager::loop(const WifiStatusSnapshot& wifi_status) {
    if (!enabled_) {
        return;
    }

    if (!wifi_status.sta_connected) {
        wifi_ready_ = false;
        return;
    }

    if (!wifi_ready_) {
        wifi_ready_ = true;
        pending_manual_check_ = true;  // First connection triggers immediate check
        setStatus("wifi-ready");
    }

    const std::uint32_t now = millis();
    const bool due = (last_check_ms_ == 0) || (now - last_check_ms_ >= check_interval_ms_);
    if (!due && !pending_manual_check_) {
        return;
    }

    pending_manual_check_ = false;
    last_check_ms_ = now;

    ManifestInfo manifest;
    if (!fetchManifest(manifest)) {
        return;
    }

    applyManifest(manifest);
}

void OTAUpdateManager::triggerImmediateCheck() {
    pending_manual_check_ = true;
}

bool OTAUpdateManager::fetchManifest(ManifestInfo& manifest) {
    if (manifest_url_.empty()) {
        setStatus("manifest-url-empty");
        return false;
    }

    HTTPClient http;
    WiFiClientSecure secure_client;
    if (!beginHttp(http, secure_client, manifest_url_)) {
        setStatus("manifest-begin-failed");
        return false;
    }

    http.setUserAgent(kUserAgent);
    http.setTimeout(20000);

    const int http_code = http.GET();
    if (http_code != HTTP_CODE_OK) {
        setStatus(std::string("manifest-http-") + std::to_string(http_code));
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(4096);
    const auto err = deserializeJson(doc, payload);
    if (err) {
        setStatus(std::string("manifest-parse-") + err.c_str());
        return false;
    }

    manifest.version = readJsonString(doc["version"]);
    manifest.channel = readJsonString(doc["channel"]);

    JsonVariantConst firmware_node = doc["firmware"];
    if (firmware_node.isNull()) {
        JsonObjectConst files = doc["files"];
        if (!files.isNull()) {
            firmware_node = files["firmware"];
        }
    }

    manifest.firmware_url = resolveUrl(manifest_url_, readJsonString(firmware_node["url"]));
    manifest.md5 = sanitizeMd5(readJsonString(firmware_node["md5"]));
    manifest.size = static_cast<std::uint32_t>(firmware_node["size"] | 0);

    if (manifest.version.empty() || manifest.firmware_url.empty()) {
        setStatus("manifest-missing-fields");
        return false;
    }

    return true;
}

bool OTAUpdateManager::applyManifest(const ManifestInfo& manifest) {
    if (!expected_channel_.empty() && !manifest.channel.empty() && manifest.channel != expected_channel_) {
        setStatus("manifest-channel-mismatch");
        return false;
    }

    if (!isNewerVersion(manifest.version)) {
        setStatus("up-to-date");
        return true;
    }

    if (!auto_apply_) {
        setStatus(std::string("update-available-") + manifest.version);
        return true;
    }

    return downloadAndInstall(manifest);
}

bool OTAUpdateManager::downloadAndInstall(const ManifestInfo& manifest) {
    setStatus(std::string("downloading-") + manifest.version);

    HTTPClient http;
    WiFiClientSecure secure_client;
    if (!beginHttp(http, secure_client, manifest.firmware_url)) {
        setStatus("firmware-begin-failed");
        return false;
    }

    http.setUserAgent(kUserAgent);
    http.setTimeout(60000);

    const int http_code = http.GET();
    if (http_code != HTTP_CODE_OK) {
        setStatus(std::string("firmware-http-") + std::to_string(http_code));
        http.end();
        return false;
    }

    std::uint32_t content_length = static_cast<std::uint32_t>(http.getSize());
    if (content_length == 0) {
        content_length = manifest.size;
    }

    if (!Update.begin(content_length > 0 ? content_length : UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
        setStatus("update-begin-failed");
        http.end();
        return false;
    }

    if (!manifest.md5.empty()) {
        if (!Update.setMD5(manifest.md5.c_str())) {
            setStatus("md5-invalid");
            http.end();
            return false;
        }
    }

    WiFiClient* stream = http.getStreamPtr();
    const size_t written = Update.writeStream(*stream);
    http.end();

    if (written == 0) {
        setStatus("firmware-empty");
        return false;
    }

    if (!Update.end(true)) {
        Update.printError(Serial);
        setStatus("update-end-failed");
        return false;
    }

    setStatus(std::string("updated-to-") + manifest.version);

    auto& config = ConfigManager::instance().getConfig();
    config.version = manifest.version;
    ConfigManager::instance().save();

    delay(500);
    ESP.restart();
    return true;
}

bool OTAUpdateManager::isNewerVersion(const std::string& remote_version) const {
    const std::string current = ConfigManager::instance().getConfig().version;
    return compareVersions(remote_version, current) > 0;
}

int OTAUpdateManager::compareVersions(const std::string& lhs, const std::string& rhs) {
    auto tokenize = [](const std::string& value) {
        std::vector<int> parts;
        std::string token;
        for (char c : value) {
            if (c == '.' || c == '-' || c == '_') {
                if (!token.empty()) {
                    parts.push_back(std::atoi(token.c_str()));
                    token.clear();
                }
                if (c == '-' || c == '_') {
                    break;  // Ignore suffixes like -beta
                }
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                token.push_back(c);
            }
        }
        if (!token.empty()) {
            parts.push_back(std::atoi(token.c_str()));
        }
        while (parts.size() < 3) {
            parts.push_back(0);
        }
        return parts;
    };

    const std::vector<int> lhs_parts = tokenize(lhs);
    const std::vector<int> rhs_parts = tokenize(rhs);
    const std::size_t count = std::min(lhs_parts.size(), rhs_parts.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (lhs_parts[i] > rhs_parts[i]) {
            return 1;
        }
        if (lhs_parts[i] < rhs_parts[i]) {
            return -1;
        }
    }
    return 0;
}

void OTAUpdateManager::setStatus(const std::string& status) {
    last_status_ = status;
    Serial.printf("[OTA] %s\n", status.c_str());
}
