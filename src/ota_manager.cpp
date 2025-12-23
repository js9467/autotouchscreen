#include "ota_manager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <lvgl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

#include "config_manager.h"
#include "web_server.h"

namespace {
constexpr const char* kUserAgent = "BroncoControls/OTA";
constexpr std::uint32_t kMinIntervalMinutes = 5;
constexpr std::uint32_t kOnlineMinIntervalMinutes = 2;
constexpr std::uint32_t kMaxIntervalMinutes = 24 * 60;

std::uint32_t clampIntervalMinutes(std::uint32_t minutes) {
    return std::max(kMinIntervalMinutes, std::min(kMaxIntervalMinutes, minutes));
}

std::uint32_t onlineIntervalMs(std::uint32_t base_interval_ms) {
    const std::uint32_t online_min_ms = kOnlineMinIntervalMinutes * 60UL * 1000UL;
    return std::min(base_interval_ms, online_min_ms);
}

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
    const std::uint32_t minutes = clampIntervalMinutes(ota_cfg.check_interval_minutes);
    base_check_interval_ms_ = minutes * 60UL * 1000UL;
    last_check_ms_ = 0;
    wifi_ready_ = false;
    internet_available_ = false;
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
        internet_available_ = false;
        if (pending_manual_check_ || manual_install_requested_) {
            Serial.printf("[OTA] Manual check blocked: WiFi STA not connected\n");
            pending_manual_check_ = false;
            manual_install_requested_ = false;
            setStatus("waiting-for-wifi");
        }
        return;
    }

    if (!wifi_ready_) {
        wifi_ready_ = true;
        pending_manual_check_ = true;  // First connection triggers immediate check
        setStatus("wifi-ready");
        Serial.printf("[OTA] WiFi now ready\n");
    }

    const std::uint32_t now = millis();
    const std::uint32_t interval_ms = internet_available_
        ? onlineIntervalMs(base_check_interval_ms_)
        : base_check_interval_ms_;
    const bool due = (last_check_ms_ == 0) || (now - last_check_ms_ >= interval_ms);
    
    if (pending_manual_check_) {
        Serial.printf("[OTA] Processing pending manual check\n");
    }
    
    if (!due && !pending_manual_check_) {
        return;
    }

    Serial.printf("[OTA] Starting OTA check (due=%d, manual=%d)\n", due, pending_manual_check_);
    pending_manual_check_ = false;
    last_check_ms_ = now;

    ManifestInfo manifest;
    if (!fetchManifest(manifest)) {
        manual_install_requested_ = false;
        return;
    }

    applyManifest(manifest, manual_install_requested_);
    manual_install_requested_ = false;
}

void OTAUpdateManager::triggerImmediateCheck(bool install_now) {
    Serial.printf("[OTA] triggerImmediateCheck called, install_now=%d, enabled=%d, wifi_ready=%d\n", 
                  install_now, enabled_, wifi_ready_);
    if (!enabled_) {
        setStatus("disabled");
        manual_install_requested_ = false;
        return;
    }
    pending_manual_check_ = true;
    if (install_now) {
        manual_install_requested_ = true;
    }
    if (wifi_ready_) {
        setStatus("manual-check-requested");
    } else {
        setStatus("waiting-for-wifi");
    }
}

bool OTAUpdateManager::fetchManifest(ManifestInfo& manifest) {
    if (manifest_url_.empty()) {
        setStatus("manifest-url-empty");
        return false;
    }

    setStatus("checking-connectivity");
    Serial.println("[OTA] Checking network connectivity...");
    
    // Test DNS resolution
    Serial.printf("[OTA] Testing DNS for: %s\n", "image-optimizer-still-flower-1282.fly.dev");
    IPAddress ip;
    if (!WiFi.hostByName("image-optimizer-still-flower-1282.fly.dev", ip)) {
        Serial.println("[OTA] DNS resolution failed - checking network status");
        Serial.printf("[OTA] WiFi Status: %d\n", WiFi.status());
        Serial.printf("[OTA] Local IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[OTA] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("[OTA] DNS: %s\n", WiFi.dnsIP().toString().c_str());
        
        // Try resolving a known good domain
        IPAddress test_ip;
        bool can_resolve_google = WiFi.hostByName("www.google.com", test_ip);
        Serial.printf("[OTA] Can resolve google.com: %s\n", can_resolve_google ? "YES" : "NO");
        
        internet_available_ = can_resolve_google;
        if (!can_resolve_google) {
            setStatus("manifest-dns-failed-no-internet");
            return false;
        } else {
            setStatus("manifest-dns-failed-fly-dev");
            return false;
        }
    }
    internet_available_ = true;
    Serial.printf("[OTA] DNS resolved to: %s\n", ip.toString().c_str());

    HTTPClient http;
    WiFiClientSecure secure_client;
    if (!beginHttp(http, secure_client, manifest_url_)) {
        setStatus("manifest-begin-failed");
        return false;
    }

    http.setUserAgent(kUserAgent);
    http.setTimeout(30000);  // Increased timeout to 30 seconds
    http.setConnectTimeout(15000);  // 15 second connection timeout

    Serial.println("[OTA] Sending HTTP GET request...");
    const int http_code = http.GET();
    Serial.printf("[OTA] HTTP response code: %d\n", http_code);
    
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

bool OTAUpdateManager::applyManifest(const ManifestInfo& manifest, bool force_install) {
    if (!expected_channel_.empty() && !manifest.channel.empty() && manifest.channel != expected_channel_) {
        setStatus("manifest-channel-mismatch");
        return false;
    }

    if (!isNewerVersion(manifest.version)) {
        setStatus("up-to-date");
        return true;
    }

    if (!auto_apply_ && !force_install) {
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

    // Download in chunks to allow UI updates and show progress
    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buffer[1024];
    unsigned long last_update_ms = millis();
    
    Serial.printf("[OTA] Starting download: %u bytes\n", content_length);
    
    // Show OTA update screen
    showOtaScreen(manifest.version);
    
    while (stream->connected() && (content_length == 0 || written < content_length)) {
        size_t available = stream->available();
        if (available > 0) {
            size_t to_read = (available > sizeof(buffer)) ? sizeof(buffer) : available;
            size_t read_bytes = stream->readBytes(buffer, to_read);
            
            if (read_bytes > 0) {
                size_t chunk_written = Update.write(buffer, read_bytes);
                if (chunk_written != read_bytes) {
                    Serial.printf("[OTA] Write failed: expected %u, wrote %u\n", read_bytes, chunk_written);
                    break;
                }
                written += chunk_written;
                
                // Update progress bar every second
                unsigned long now = millis();
                if (now - last_update_ms >= 1000) {
                    if (content_length > 0) {
                        uint8_t progress = (written * 100) / content_length;
                        Serial.printf("[OTA] Progress: %u%% (%u/%u bytes)\n", progress, written, content_length);
                        updateOtaProgress(progress);
                    }
                    last_update_ms = now;
                    yield(); // Allow other tasks to run
                }
            }
        } else {
            delay(1);
        }
        
        // Timeout check
        if (content_length > 0 && written >= content_length) {
            break;
        }
    }
    
    http.end();
    Serial.printf("[OTA] Download complete: %u bytes\n", written);

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

// OTA update screen with progress bar
static lv_obj_t* ota_screen = nullptr;
static lv_obj_t* ota_bar = nullptr;
static lv_obj_t* ota_label = nullptr;

void OTAUpdateManager::showOtaScreen(const std::string& version) {
    if (ota_screen != nullptr) {
        return;  // Already showing
    }

    // Create fullscreen overlay
    ota_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(ota_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ota_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(ota_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ota_screen, 0, 0);
    lv_obj_center(ota_screen);
    
    // Title
    lv_obj_t* title = lv_label_create(ota_screen);
    lv_label_set_text(title, "Updating Firmware");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);
    
    // Version label
    ota_label = lv_label_create(ota_screen);
    std::string msg = "Version " + version;
    lv_label_set_text(ota_label, msg.c_str());
    lv_obj_set_style_text_color(ota_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(ota_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ota_label, LV_ALIGN_CENTER, 0, -20);
    
    // Progress bar
    ota_bar = lv_bar_create(ota_screen);
    lv_obj_set_size(ota_bar, 280, 20);
    lv_obj_align(ota_bar, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(ota_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ota_bar, lv_color_hex(0x00a8e8), LV_PART_INDICATOR);
    lv_bar_set_value(ota_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(ota_bar, 0, 100);
    
    // Percent label
    lv_obj_t* percent = lv_label_create(ota_screen);
    lv_label_set_text(percent, "0%");
    lv_obj_set_style_text_color(percent, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(percent, LV_ALIGN_CENTER, 0, 50);
    
    lv_obj_move_foreground(ota_screen);
}

void OTAUpdateManager::updateOtaProgress(uint8_t percent) {
    if (ota_bar != nullptr) {
        lv_bar_set_value(ota_bar, percent, LV_ANIM_OFF);
        
        // Update percent text
        lv_obj_t* percent_label = lv_obj_get_child(ota_screen, 3);
        if (percent_label) {
            std::string text = std::to_string(percent) + "%";
            lv_label_set_text(percent_label, text.c_str());
        }
    }
}

void OTAUpdateManager::hideOtaScreen() {
    if (ota_screen != nullptr) {
        lv_obj_del(ota_screen);
        ota_screen = nullptr;
        ota_bar = nullptr;
        ota_label = nullptr;
    }
}
