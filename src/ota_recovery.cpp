/**
 * OTA Recovery Firmware
 * Purpose: Minimal firmware to enable USB serial for subsequent uploads
 * - Enables USB serial (USB_SEL LOW)
 * - Maintains WiFi for OTA updates
 * - Minimal overhead
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <Update.h>
#include "ESP_IOExpander_Library.h"

// I2C configuration for IO Expander
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

// WiFi credentials - use existing ones
const char* ssid = "BroncoControls";
const char* password = "bronco2024";

// IO Expander for USB_SEL control
ESP_IOExpander_CH422G *expander = nullptr;
#define USB_SEL 3  // GPIO3 on CH422G controls USB vs CAN multiplexer

AsyncWebServer server(80);

void setupIOExpander() {
    Serial.println("Initializing IO Expander...");
    expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    expander->init();
    expander->begin();
    
    // CRITICAL: Set USB_SEL LOW to enable USB serial (disables CAN)
    expander->pinMode(USB_SEL, OUTPUT);
    expander->digitalWrite(USB_SEL, LOW);
    
    Serial.println("✓ USB Serial ENABLED (CAN disabled)");
}

void setupWiFi() {
    Serial.println("\nConnecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n✗ WiFi connection failed");
    }
}

void setupWebServer() {
    // Status endpoint
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<html><body>";
        html += "<h1>OTA Recovery Mode</h1>";
        html += "<p>USB Serial: ENABLED</p>";
        html += "<p>Ready for serial upload</p>";
        html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });
    
    // API status
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<256> doc;
        doc["mode"] = "recovery";
        doc["usb_serial"] = true;
        doc["ip"] = WiFi.localIP().toString();
        
        const esp_app_desc_t *app_desc = esp_ota_get_app_description();
        doc["version"] = app_desc->version;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.begin();
    Serial.println("✓ Web server started");
}

void setup() {
    // Initialize USB Serial first
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║   OTA RECOVERY FIRMWARE ACTIVE        ║");
    Serial.println("║   USB Serial: ENABLED                 ║");
    Serial.println("║   Ready for serial upload             ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    // Setup IO expander to enable USB serial
    setupIOExpander();
    
    // Connect to WiFi for OTA capability
    setupWiFi();
    
    // Start web server
    setupWebServer();
    
    Serial.println("\n✓ Recovery mode ready");
    Serial.println("You can now:");
    Serial.println("  1. Upload firmware via USB serial");
    Serial.println("  2. Access web interface at: http://" + WiFi.localIP().toString());
    Serial.println("  3. Send OTA update");
}

void loop() {
    // Just maintain WiFi connection and print status every 10 seconds
    static unsigned long lastPrint = 0;
    
    if (millis() - lastPrint > 10000) {
        lastPrint = millis();
        Serial.printf("Status: WiFi %s | IP: %s | USB Serial: ENABLED\n",
                     WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                     WiFi.localIP().toString().c_str());
    }
    
    delay(100);
}
