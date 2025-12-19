#include "web_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <cstddef>

#include "config_manager.h"
#include "ui_builder.h"
#include "web_interface.h"

namespace {
const IPAddress kApIp(192, 168, 4, 250);
const IPAddress kApGateway(192, 168, 4, 250);
const IPAddress kApMask(255, 255, 255, 0);
constexpr std::size_t kConfigJsonLimit = 524288;  // 512KB for config with base64 images (base64 adds ~33% overhead)
constexpr std::size_t kWifiConnectJsonLimit = 1024;

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
}

WebServerManager& WebServerManager::instance() {
    static WebServerManager server;
    return server;
}

WebServerManager::WebServerManager()
    : server_(80) {}

void WebServerManager::begin() {
    if (!events_registered_) {
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                sta_connected_ = true;
                sta_ip_ = WiFi.localIP();
                Serial.printf("[WebServer] Station connected: %s\n", sta_ip_.toString().c_str());
            } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                sta_connected_ = false;
                sta_ip_ = IPAddress(0, 0, 0, 0);
                Serial.println("[WebServer] Station disconnected");
            }
        });
        events_registered_ = true;
    }

    configureWifi();
    setupRoutes();
    server_.begin();
    
    // Start DNS server for captive portal
    dns_server_.start(53, "*", kApIp);
    
    Serial.println("[WebServer] HTTP server started on port 80");
    Serial.println("[WebServer] Captive portal DNS active");
}

void WebServerManager::loop() {
    // Process DNS requests for captive portal
    dns_server_.processNextRequest();
}

void WebServerManager::notifyConfigChanged() {
    configureWifi();
}

void WebServerManager::configureWifi() {
    auto& wifi = ConfigManager::instance().getConfig().wifi;
    WiFi.mode(WIFI_MODE_APSTA);

    // SAFETY: Always enable AP if it was disabled (prevents lockout)
    if (!wifi.ap.enabled) {
        Serial.println("[WebServer] WARNING: AP was disabled. Re-enabling to prevent lockout.");
        wifi.ap.enabled = true;
        if (wifi.ap.ssid.empty()) {
            wifi.ap.ssid = "CAN-Control";
        }
        if (wifi.ap.password.empty() || wifi.ap.password.length() < 8) {
            wifi.ap.password = "canbus123";
        }
        ConfigManager::instance().save();
    }

    if (wifi.ap.enabled) {
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
    } else {
        WiFi.softAPdisconnect(true);
        ap_ip_ = IPAddress(0, 0, 0, 0);
    }

    if (wifi.sta.enabled && !wifi.sta.ssid.empty()) {
        Serial.printf("[WebServer] Connecting to %s...\n", wifi.sta.ssid.c_str());
        sta_connected_ = false;
        WiFi.begin(wifi.sta.ssid.c_str(), wifi.sta.password.c_str());
    } else {
        WiFi.disconnect(true);
        sta_connected_ = false;
        sta_ip_ = IPAddress(0, 0, 0, 0);
    }
}

void WebServerManager::setupRoutes() {
    // Captive portal detection endpoints
    // iOS and macOS
    server_.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    // Android
    server_.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    // Windows
    server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    // Generic captive portal detection
    server_.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    
    // Main configuration page
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", WEB_INTERFACE_HTML);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        response->addHeader("ETag", String(millis()).c_str());  // Force fresh content
        request->send(response);
    });

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(256);
        doc["ap_ip"] = ap_ip_.toString();
        doc["sta_ip"] = sta_ip_.toString();
        doc["sta_connected"] = sta_connected_;
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
            std::string error;
            if (!ConfigManager::instance().updateFromJson(json.as<JsonVariantConst>(), error)) {
                DynamicJsonDocument doc(256);
                doc["status"] = "error";
                doc["message"] = error.c_str();
                String payload;
                serializeJson(doc, payload);
                request->send(400, "application/json", payload);
                return;
            }

            if (!ConfigManager::instance().save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist\"}");
                return;
            }

            UIBuilder::instance().markDirty();
            notifyConfigChanged();

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

            notifyConfigChanged();

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
                // Clear logo_variant when custom header is uploaded
                cfg.header.logo_variant = "";
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
        }, 262144);  // 256KB limit for single image upload
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

    server_.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

WifiStatusSnapshot WebServerManager::getStatusSnapshot() const {
    WifiStatusSnapshot snapshot;
    snapshot.ap_ip = ap_ip_;
    snapshot.sta_ip = sta_ip_;
    snapshot.sta_connected = sta_connected_;
    return snapshot;
}

void WebServerManager::disableAP() {
    Serial.println("[WebServer] Disabling Access Point");
    dns_server_.stop();
    WiFi.softAPdisconnect(true);
    ap_ip_ = IPAddress(0, 0, 0, 0);
}
