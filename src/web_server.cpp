#include "web_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <cstddef>

#include "config_manager.h"
#include "ui_builder.h"
#include "version_auto.h"
#include "web_interface.h"

namespace {
const IPAddress kApIp(192, 168, 4, 250);
const IPAddress kApGateway(192, 168, 4, 250);
const IPAddress kApMask(255, 255, 255, 0);
constexpr std::size_t kConfigJsonLimit = 524288;  // 512KB for config with base64 images (base64 adds ~33% overhead)
constexpr std::size_t kWifiConnectJsonLimit = 1024;
constexpr std::size_t kImageUploadJsonLimit = 1048576;  // 1MB limit for header/base64 payloads
constexpr std::size_t kImageUploadContentLimit = 1048576;
constexpr std::uint32_t kWifiReconfigureDelayMs = 750;  // Allow HTTP responses to finish before toggling radios

const char* AuthModeToString(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "wep";
        case WIFI_AUTH_WPA_PSK: return "wpa";
        case WIFI_AUTH_WPA2_PSK: return "wpa2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "wpa_wpa2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2_enterprise";
        case WIFI_AUTH_WPA3_PSK: return "wpa3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa2_wpa3";
        case WIFI_AUTH_WAPI_PSK: return "wapi";
        default: return "unknown";
    }
}

bool WifiConfigEquals(const WifiConfig& lhs, const WifiConfig& rhs) {
    const auto creds_equal = [](const WifiCredentials& a, const WifiCredentials& b) {
        return a.enabled == b.enabled && a.ssid == b.ssid && a.password == b.password;
    };
    return creds_equal(lhs.ap, rhs.ap) && creds_equal(lhs.sta, rhs.sta);
}
}

WebServerManager& WebServerManager::instance() {
    static WebServerManager server;
    return server;
}

WebServerManager::WebServerManager()
    : server_(80) {}

void WebServerManager::begin() {
    static bool dns_configured = false;
    if (!events_registered_) {
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                sta_connected_ = true;
                sta_ip_ = WiFi.localIP();
                sta_ssid_ = WiFi.SSID().c_str();
                // Set DNS servers AFTER getting IP to prevent DHCP from overwriting
                // Only do this once to avoid triggering more events
                if (!dns_configured) {
                    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), 
                               IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1));
                    dns_configured = true;
                    Serial.println("[WiFi] DNS configured to 8.8.8.8 (primary) and 1.1.1.1 (secondary)");
                }
                Serial.printf("[WebServer] Station connected: %s\n", sta_ip_.toString().c_str());
            } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                sta_connected_ = false;
                sta_ip_ = IPAddress(0, 0, 0, 0);
                sta_ssid_.clear();
                Serial.println("[WebServer] Station disconnected");
            }
        });
        events_registered_ = true;
    }

    configureWifi();
    setupRoutes();
    server_.begin();
    Serial.println("[WebServer] HTTP server started on port 80");
}

void WebServerManager::loop() {
    // Process DNS requests for captive portal
    if (dns_active_) {
        dns_server_.processNextRequest();
    }

    if (wifi_reconfigure_pending_) {
        const std::uint32_t now = millis();
        if (now - wifi_reconfigure_request_ms_ >= kWifiReconfigureDelayMs) {
            wifi_reconfigure_pending_ = false;
            configureWifi();
        }
    }
}

void WebServerManager::notifyConfigChanged() {
    wifi_reconfigure_pending_ = true;
    wifi_reconfigure_request_ms_ = millis();
}

void WebServerManager::configureWifi() {
    auto& wifi = ConfigManager::instance().getConfig().wifi;
    WiFi.mode(WIFI_MODE_APSTA);

    const bool sta_configured = wifi.sta.enabled && !wifi.sta.ssid.empty();
    if ((!wifi.ap.enabled || ap_suppressed_) && !sta_configured) {
        Serial.println("[WebServer] WARNING: Station credentials missing. Enabling fallback AP.");
        wifi.ap.enabled = true;
        ap_suppressed_ = false;
        if (wifi.ap.ssid.empty()) {
            wifi.ap.ssid = "CAN-Control";
        }
        ConfigManager::instance().save();
    }

    if (ap_suppressed_ && !sta_connected_) {
        Serial.println("[WebServer] STA disconnected. Re-enabling AP for recovery.");
        ap_suppressed_ = false;
    }

    if (wifi.ap.enabled && !ap_suppressed_) {
        const char* password = nullptr;
        if (wifi.ap.password.length() >= 8) {
            password = wifi.ap.password.c_str();
        }
        WiFi.softAPdisconnect(true);
        if (!WiFi.softAPConfig(kApIp, kApGateway, kApMask)) {
            Serial.println("[WebServer] Failed to set AP IP config");
        }
        
        Serial.printf("[WebServer] Starting AP - SSID: %s, Password: %s\n", 
                     wifi.ap.ssid.c_str(), password ? password : "(none - open network)");
        
        if (!WiFi.softAP(wifi.ap.ssid.c_str(), password)) {
            Serial.println("[WebServer] Failed to start access point");
        }
        ap_ip_ = WiFi.softAPIP();
        Serial.printf("[WebServer] AP ready at %s\n", ap_ip_.toString().c_str());
        if (!dns_active_) {
            dns_server_.start(53, "*", kApIp);
            dns_active_ = true;
            Serial.println("[WebServer] Captive portal DNS active");
        }
    } else {
        WiFi.softAPdisconnect(true);
        ap_ip_ = IPAddress(0, 0, 0, 0);
        if (dns_active_) {
            dns_server_.stop();
            dns_active_ = false;
            Serial.println("[WebServer] Captive portal DNS stopped");
        }
    }

    if (wifi.sta.enabled && !wifi.sta.ssid.empty()) {
        Serial.printf("[WebServer] Connecting to %s...\n", wifi.sta.ssid.c_str());
        sta_connected_ = false;
        sta_ssid_.clear();
        WiFi.begin(wifi.sta.ssid.c_str(), wifi.sta.password.c_str());
    } else {
        WiFi.disconnect(true);
        sta_connected_ = false;
        sta_ip_ = IPAddress(0, 0, 0, 0);
        sta_ssid_.clear();
    }
}

void WebServerManager::setupRoutes() {
    // Captive portal detection endpoints - return wrong content to trigger portal
    // iOS and macOS - expects "Success" but we return wrong content to trigger portal
    server_.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=http://192.168.4.250/'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
    server_.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=http://192.168.4.250/'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    });
    // Android - expects 204 No Content, we return different to trigger portal
    server_.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    // Windows connectivity tests
    server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    // Additional Microsoft connectivity endpoints
    server_.on("/connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/microsoft-connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    
    // Main configuration page
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Generate HTML with version embedded
        String html = FPSTR(WEB_INTERFACE_HTML);
        html.replace("{{VERSION}}", APP_VERSION);
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html.c_str());
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        response->addHeader("ETag", String(millis()).c_str());  // Force fresh content
        request->send(response);
    });

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(384);
        doc["firmware_version"] = APP_VERSION;
        doc["ap_ip"] = ap_ip_.toString();
        doc["sta_ip"] = sta_ip_.toString();
        doc["sta_connected"] = sta_connected_;

        std::string device_ip;
        if (sta_connected_ && sta_ip_ != IPAddress(0, 0, 0, 0)) {
            device_ip = sta_ip_.toString().c_str();
        } else if (ap_ip_ != IPAddress(0, 0, 0, 0)) {
            device_ip = ap_ip_.toString().c_str();
        }
        doc["device_ip"] = device_ip.c_str();

        std::string connected_network;
        if (sta_connected_) {
            if (!sta_ssid_.empty()) {
                connected_network = sta_ssid_;
            } else {
                const auto& cfg = ConfigManager::instance().getConfig().wifi;
                connected_network = cfg.sta.ssid.empty() ? "Hidden network" : cfg.sta.ssid;
            }
        } else if (ap_ip_ != IPAddress(0, 0, 0, 0)) {
            const auto& cfg = ConfigManager::instance().getConfig().wifi;
            std::string ap_ssid = cfg.ap.ssid.empty() ? "CAN-Control" : cfg.ap.ssid;
            connected_network = "AP: " + ap_ssid;
        }
        doc["connected_network"] = connected_network.c_str();

        doc["uptime_ms"] = millis();
        doc["heap"] = ESP.getFreeHeap();
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    server_.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        std::string json = ConfigManager::instance().toJson();
        String payload(json.c_str());
        request->send(200, "application/json", payload);
    });

    auto* handler = new AsyncCallbackJsonWebHandler("/api/config",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            auto& config_mgr = ConfigManager::instance();
            WifiConfig previous_wifi = config_mgr.getConfig().wifi;
            std::string error;
            if (!config_mgr.updateFromJson(json.as<JsonVariantConst>(), error)) {
                DynamicJsonDocument doc(256);
                doc["status"] = "error";
                doc["message"] = error.c_str();
                String payload;
                serializeJson(doc, payload);
                request->send(400, "application/json", payload);
                return;
            }

            const bool wifi_changed = !WifiConfigEquals(previous_wifi, config_mgr.getConfig().wifi);

            if (!config_mgr.save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist\"}");
                return;
            }

            UIBuilder::instance().markDirty();

            if (wifi_changed) {
                request->onDisconnect([this]() {
                    notifyConfigChanged();
                });
            }

            DynamicJsonDocument doc(64);
            doc["status"] = "ok";
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kConfigJsonLimit);
    server_.addHandler(handler);

    auto* wifi_handler = new AsyncCallbackJsonWebHandler("/api/wifi/connect",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            String ssid = json["ssid"] | "";
            String password = json["password"] | "";
            bool persist = json["persist"] | true;

            if (ssid.isEmpty()) {
                DynamicJsonDocument doc(128);
                doc["status"] = "error";
                doc["message"] = "SSID is required";
                String payload;
                serializeJson(doc, payload);
                request->send(400, "application/json", payload);
                return;
            }

            auto& cfg = ConfigManager::instance().getConfig();
            cfg.wifi.sta.enabled = true;
            cfg.wifi.sta.ssid = ssid.c_str();
            cfg.wifi.sta.password = password.c_str();

            if (persist && !ConfigManager::instance().save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist\"}");
                return;
            }

            request->onDisconnect([this]() {
                notifyConfigChanged();
            });

            DynamicJsonDocument doc(160);
            doc["status"] = "connecting";
            doc["ssid"] = cfg.wifi.sta.ssid.c_str();
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kWifiConnectJsonLimit);
    server_.addHandler(wifi_handler);

    // Dedicated image upload endpoints (separate from main config to avoid size limits)
    auto* image_handler = new AsyncCallbackJsonWebHandler("/api/image/upload",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            String imageType = json["type"] | "";
            String imageData = json["data"] | "";
            
            if (imageType.isEmpty()) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing type\"}");
                return;
            }
            
            // Allow empty data for clearing images
            Serial.printf("[WebServer] Image upload: type=%s, data_length=%d\n", imageType.c_str(), imageData.length());
            
            auto& cfg = ConfigManager::instance().getConfig();
            
            // Store image in appropriate field
            if (imageType == "header") {
                cfg.images.header_logo = imageData.c_str();
                // Toggle logo display based on whether we have data
                cfg.header.show_logo = !imageData.isEmpty();
                // Clear logo_variant when custom header is uploaded
                if (!imageData.isEmpty()) {
                    cfg.header.logo_variant = "";
                }
            } else if (imageType == "splash") {
                cfg.images.splash_logo = imageData.c_str();
            } else if (imageType == "background") {
                cfg.images.background_image = imageData.c_str();
            } else if (imageType == "sleep") {
                cfg.images.sleep_logo = imageData.c_str();
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid image type\"}");
                return;
            }
            
            if (!ConfigManager::instance().save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save\"}");
                return;
            }
            
            UIBuilder::instance().markDirty();
            
            DynamicJsonDocument doc(64);
            doc["status"] = "ok";
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kImageUploadJsonLimit);
    image_handler->setMaxContentLength(kImageUploadContentLimit);
    server_.addHandler(image_handler);

    server_.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        const int16_t count = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
        if (count < 0) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Scan failed\"}");
            return;
        }

        DynamicJsonDocument doc(4096);
        doc["status"] = "ok";
        doc["count"] = count;
        JsonArray networks = doc.createNestedArray("networks");
        for (int16_t i = 0; i < count; ++i) {
            const String ssid = WiFi.SSID(i);
            JsonObject entry = networks.createNestedObject();
            entry["ssid"] = ssid;
            entry["rssi"] = WiFi.RSSI(i);
            entry["channel"] = WiFi.channel(i);
            entry["bssid"] = WiFi.BSSIDstr(i);
            const wifi_auth_mode_t auth = WiFi.encryptionType(i);
            entry["secure"] = auth != WIFI_AUTH_OPEN;
            entry["auth"] = AuthModeToString(auth);
            entry["hidden"] = ssid.isEmpty();
        }
        WiFi.scanDelete();

        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // Captive portal - redirect all unknown requests to main page
    server_.onNotFound([](AsyncWebServerRequest* request) {
        // For API calls, return 404
        if (request->url().startsWith("/api/")) {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        } else {
            // For all other requests, redirect to captive portal with 302 redirect
            request->redirect("http://192.168.4.250/");
        }
    });
}

WifiStatusSnapshot WebServerManager::getStatusSnapshot() const {
    WifiStatusSnapshot snapshot;
    snapshot.ap_ip = ap_ip_;
    snapshot.sta_ip = sta_ip_;
    snapshot.sta_connected = sta_connected_ || (WiFi.status() == WL_CONNECTED);
    if (!sta_ssid_.empty()) {
        snapshot.sta_ssid = sta_ssid_;
    } else if (snapshot.sta_connected) {
        snapshot.sta_ssid = WiFi.SSID().c_str();
    }
    return snapshot;
}

void WebServerManager::disableAP() {
    Serial.println("[WebServer] Disabling Access Point");
    dns_server_.stop();
    dns_active_ = false;
    ap_suppressed_ = true;
    WiFi.softAPdisconnect(true);
    ap_ip_ = IPAddress(0, 0, 0, 0);
}
