#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Limits that align with the documentation
constexpr std::size_t MAX_PAGES = 20;
constexpr std::size_t MAX_BUTTONS_PER_PAGE = 12;

struct CanFrameConfig {
    bool enabled = false;
    std::uint32_t pgn = 0x00FF00;  // Default to proprietary B frame
    std::uint8_t priority = 3;
    std::uint8_t source_address = 0x80;
    std::uint8_t destination_address = 0xFF;  // Broadcast by default
    std::array<std::uint8_t, 8> data{};
};

struct ButtonConfig {
    std::string id = "button_0";
    std::string label = "Button";
    std::string color = "#FFA500";
    std::string pressed_color = "#FF8800";
    std::string icon = "";  // Reserved for future icon uploads
    std::uint8_t row = 0;
    std::uint8_t col = 0;
    std::uint8_t row_span = 1;
    std::uint8_t col_span = 1;
    bool momentary = false;
    std::uint8_t font_size = 24;
    std::string font_family = "montserrat";  // montserrat or unscii
    std::string text_align = "center";  // top-left, top-center, top-right, center, bottom-left, bottom-center, bottom-right
    CanFrameConfig can;
};

struct PageConfig {
    std::string id = "page_0";
    std::string name = "Home";
    std::string nav_color = "";
    std::uint8_t rows = 2;
    std::uint8_t cols = 2;
    std::vector<ButtonConfig> buttons;
};

struct WifiCredentials {
    bool enabled = true;
    std::string ssid = "BroncoControls";
    std::string password = "bronco123";
};

struct WifiConfig {
    WifiCredentials ap{};
    WifiCredentials sta{};

    WifiConfig() {
        sta.enabled = false;
        sta.ssid.clear();
        sta.password.clear();
    }
};

struct HeaderConfig {
    std::string title = "Bronco Controls";
    std::string subtitle = "Web Configurator";
    bool show_logo = true;
    std::string logo_variant = "bronco";  // Matches icon registry ids
};

struct ThemeConfig {
    std::string bg_color = "#1A1A1A";
    std::string surface_color = "#2A2A2A";
    std::string page_bg_color = "#0F0F0F";
    std::string accent_color = "#FFA500";
    std::string text_primary = "#FFFFFF";
    std::string text_secondary = "#AAAAAA";
    std::string border_color = "#3A3A3A";
    std::string header_border_color = "#FFA500";
    std::string nav_button_color = "#3A3A3A";
    std::string nav_button_active_color = "#FFA500";
    std::uint8_t button_radius = 12;
    std::uint8_t border_width = 2;
    std::uint8_t header_border_width = 0;
};

struct CanMessage {
    std::string id = "msg_0";
    std::string name = "Unnamed";
    std::uint32_t pgn = 0x00FF00;
    std::uint8_t priority = 3;
    std::uint8_t source_address = 0x80;
    std::uint8_t destination_address = 0xFF;
    std::array<std::uint8_t, 8> data{};
    std::string description = "";
};

struct DeviceConfig {
    std::string version = "1.0.0";
    WifiConfig wifi{};
    HeaderConfig header{};
    ThemeConfig theme{};
    std::vector<PageConfig> pages;
    std::vector<CanMessage> can_library;
};
