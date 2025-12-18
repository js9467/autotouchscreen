#pragma once

#include <ESPAsyncWebServer.h>
#include <WiFi.h>

struct WifiStatusSnapshot {
    IPAddress ap_ip;
    IPAddress sta_ip;
    bool sta_connected = false;
};

class WebServerManager {
public:
    static WebServerManager& instance();

    void begin();
    void loop();
    void notifyConfigChanged();
    void disableAP();
    WifiStatusSnapshot getStatusSnapshot() const;

private:
    WebServerManager();

    void setupRoutes();
    void configureWifi();

    AsyncWebServer server_;
    bool sta_connected_ = false;
    bool events_registered_ = false;
    IPAddress ap_ip_{0, 0, 0, 0};
    IPAddress sta_ip_{0, 0, 0, 0};
};
