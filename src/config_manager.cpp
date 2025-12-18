#include "config_manager.h"

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include <algorithm>
#include <sstream>

namespace {
constexpr const char* kConfigPath = "/config.json";

template <typename T>
T clampValue(T value, T min_value, T max_value) {
    return std::min(max_value, std::max(min_value, value));
}

std::string safeString(JsonVariantConst value, const std::string& fallback) {
    if (value.is<const char*>()) {
        return std::string(value.as<const char*>());
    }
    if (value.is<std::string>()) {
        return value.as<std::string>();
    }
    if (value.is<long>()) {
        return std::to_string(value.as<long>());
    }
    if (value.is<unsigned long>()) {
        return std::to_string(value.as<unsigned long>());
    }
    return fallback;
}

std::string sanitizeColor(const std::string& hex) {
    if (hex.size() == 7 && hex[0] == '#') {
        return hex;
    }
    return "#FFA500";
}

std::string fallbackId(const char* prefix, std::size_t index) {
    std::ostringstream oss;
    oss << prefix << '_' << index;
    return oss.str();
}
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager mgr;
    return mgr;
}

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[ConfigManager] Failed to mount LittleFS");
        return false;
    }

    if (!LittleFS.exists(kConfigPath)) {
        config_ = buildDefaultConfig();
        return save();
    }

    if (!loadFromStorage()) {
        Serial.println("[ConfigManager] Failed to load config. Reverting to defaults.");
        config_ = buildDefaultConfig();
        return save();
    }

    return true;
}

bool ConfigManager::save() const {
    std::string json = toJson();
    return writeToStorage(json);
}

bool ConfigManager::resetToDefaults() {
    config_ = buildDefaultConfig();
    return save();
}

std::string ConfigManager::toJson() const {
    DynamicJsonDocument doc(16384);
    encodeConfig(config_, doc);

    std::string output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::updateFromJson(JsonVariantConst json, std::string& error) {
    DeviceConfig incoming;
    if (!decodeConfig(json, incoming, error)) {
        return false;
    }

    config_ = std::move(incoming);
    return true;
}

bool ConfigManager::loadFromStorage() {
    File file = LittleFS.open(kConfigPath, FILE_READ);
    if (!file) {
        Serial.println("[ConfigManager] Could not open config file");
        return false;
    }

    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.printf("[ConfigManager] JSON parse error: %s\n", err.c_str());
        return false;
    }

    std::string parse_error;
    if (!decodeConfig(doc.as<JsonVariantConst>(), config_, parse_error)) {
        Serial.printf("[ConfigManager] Decode error: %s\n", parse_error.c_str());
        return false;
    }

    return true;
}

bool ConfigManager::writeToStorage(const std::string& json) const {
    File file = LittleFS.open(kConfigPath, FILE_WRITE);
    if (!file) {
        Serial.println("[ConfigManager] Could not open config file for writing");
        return false;
    }

    size_t written = file.print(json.c_str());
    file.close();
    return written == json.size();
}

DeviceConfig ConfigManager::buildDefaultConfig() const {
    DeviceConfig cfg;
    cfg.version = "1.0.0";
    cfg.header.title = "Bronco Controls";
    cfg.header.subtitle = "Web Configurator";
    cfg.header.show_logo = true;
    cfg.header.logo_variant = "bronco";

    PageConfig home;
    home.id = "home";
    home.name = "Factory Home";
    home.rows = 2;
    home.cols = 2;

    ButtonConfig windows;
    windows.id = "windows";
    windows.label = "Windows";
    windows.color = "#FF8A00";
    windows.row = 0;
    windows.col = 0;

    ButtonConfig locks;
    locks.id = "locks";
    locks.label = "Locks";
    locks.color = "#1ABC9C";
    locks.row = 0;
    locks.col = 1;

    ButtonConfig running;
    running.id = "running";
    running.label = "Running Boards";
    running.color = "#2980B9";
    running.row = 1;
    running.col = 0;

    ButtonConfig aux;
    aux.id = "aux";
    aux.label = "Aux";
    aux.color = "#9B59B6";
    aux.row = 1;
    aux.col = 1;

    home.buttons = {windows, locks, running, aux};
    cfg.pages = {home};

    return cfg;
}

void ConfigManager::encodeConfig(const DeviceConfig& source, DynamicJsonDocument& doc) const {
    doc.clear();
    doc["version"] = source.version.c_str();

    JsonObject header = doc["header"].to<JsonObject>();
    header["title"] = source.header.title.c_str();
    header["subtitle"] = source.header.subtitle.c_str();
    header["show_logo"] = source.header.show_logo;
    header["logo_variant"] = source.header.logo_variant.c_str();

    JsonObject theme = doc["theme"].to<JsonObject>();
    theme["bg_color"] = source.theme.bg_color.c_str();
    theme["surface_color"] = source.theme.surface_color.c_str();
    theme["page_bg_color"] = source.theme.page_bg_color.c_str();
    theme["accent_color"] = source.theme.accent_color.c_str();
    theme["text_primary"] = source.theme.text_primary.c_str();
    theme["text_secondary"] = source.theme.text_secondary.c_str();
    theme["border_color"] = source.theme.border_color.c_str();
    theme["header_border_color"] = source.theme.header_border_color.c_str();
    theme["nav_button_color"] = source.theme.nav_button_color.c_str();
    theme["nav_button_active_color"] = source.theme.nav_button_active_color.c_str();
    theme["button_radius"] = source.theme.button_radius;
    theme["border_width"] = source.theme.border_width;
    theme["header_border_width"] = source.theme.header_border_width;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    JsonObject ap = wifi["ap"].to<JsonObject>();
    ap["enabled"] = source.wifi.ap.enabled;
    ap["ssid"] = source.wifi.ap.ssid.c_str();
    ap["password"] = source.wifi.ap.password.c_str();

    JsonObject sta = wifi["sta"].to<JsonObject>();
    sta["enabled"] = source.wifi.sta.enabled;
    sta["ssid"] = source.wifi.sta.ssid.c_str();
    sta["password"] = source.wifi.sta.password.c_str();

    JsonArray pages = doc["pages"].to<JsonArray>();
    for (const auto& page : source.pages) {
        JsonObject page_obj = pages.createNestedObject();
        page_obj["id"] = page.id.c_str();
        page_obj["name"] = page.name.c_str();
        page_obj["nav_color"] = page.nav_color.c_str();
        page_obj["rows"] = page.rows;
        page_obj["cols"] = page.cols;

        JsonArray buttons = page_obj["buttons"].to<JsonArray>();
        for (const auto& button : page.buttons) {
            JsonObject btn_obj = buttons.createNestedObject();
            btn_obj["id"] = button.id.c_str();
            btn_obj["label"] = button.label.c_str();
            btn_obj["color"] = button.color.c_str();
            btn_obj["pressed_color"] = button.pressed_color.c_str();
            btn_obj["icon"] = button.icon.c_str();
            btn_obj["row"] = button.row;
            btn_obj["col"] = button.col;
            btn_obj["row_span"] = button.row_span;
            btn_obj["col_span"] = button.col_span;
            btn_obj["momentary"] = button.momentary;
            btn_obj["font_size"] = button.font_size;
            btn_obj["font_family"] = button.font_family.c_str();
            btn_obj["text_align"] = button.text_align.c_str();
            btn_obj["font_size"] = button.font_size;

            JsonObject can_obj = btn_obj["can"].to<JsonObject>();
            can_obj["enabled"] = button.can.enabled;
            can_obj["pgn"] = button.can.pgn;
            can_obj["priority"] = button.can.priority;
            can_obj["source_address"] = button.can.source_address;
            can_obj["destination_address"] = button.can.destination_address;

            JsonArray data_arr = can_obj["data"].to<JsonArray>();
            for (std::uint8_t byte : button.can.data) {
                data_arr.add(byte);
            }
        }
    }

    JsonArray can_library = doc["can_library"].to<JsonArray>();
    for (const auto& msg : source.can_library) {
        JsonObject msg_obj = can_library.createNestedObject();
        msg_obj["id"] = msg.id.c_str();
        msg_obj["name"] = msg.name.c_str();
        msg_obj["pgn"] = msg.pgn;
        msg_obj["priority"] = msg.priority;
        msg_obj["source_address"] = msg.source_address;
        msg_obj["destination_address"] = msg.destination_address;
        msg_obj["description"] = msg.description.c_str();
        
        JsonArray data_arr = msg_obj["data"].to<JsonArray>();
        for (std::uint8_t byte : msg.data) {
            data_arr.add(byte);
        }
    }
}

bool ConfigManager::decodeConfig(JsonVariantConst json, DeviceConfig& target, std::string& error) const {
    if (json.isNull()) {
        error = "JSON payload is empty";
        return false;
    }

    target.version = safeString(json["version"], "1.0.0");

    JsonObjectConst header = json["header"];
    if (!header.isNull()) {
        target.header.title = safeString(header["title"], target.header.title);
        target.header.subtitle = safeString(header["subtitle"], target.header.subtitle);
        target.header.show_logo = header["show_logo"] | target.header.show_logo;
        target.header.logo_variant = safeString(header["logo_variant"], target.header.logo_variant);
    }

    JsonObjectConst theme = json["theme"];
    if (!theme.isNull()) {
        target.theme.bg_color = sanitizeColor(safeString(theme["bg_color"], target.theme.bg_color));
        target.theme.surface_color = sanitizeColor(safeString(theme["surface_color"], target.theme.surface_color));
        target.theme.page_bg_color = sanitizeColor(safeString(theme["page_bg_color"], target.theme.page_bg_color));
        target.theme.accent_color = sanitizeColor(safeString(theme["accent_color"], target.theme.accent_color));
        target.theme.text_primary = sanitizeColor(safeString(theme["text_primary"], target.theme.text_primary));
        target.theme.text_secondary = sanitizeColor(safeString(theme["text_secondary"], target.theme.text_secondary));
        target.theme.border_color = sanitizeColor(safeString(theme["border_color"], target.theme.border_color));
        target.theme.header_border_color = sanitizeColor(safeString(theme["header_border_color"], target.theme.header_border_color));
        target.theme.nav_button_color = sanitizeColor(safeString(theme["nav_button_color"], target.theme.nav_button_color));
        target.theme.nav_button_active_color = sanitizeColor(safeString(theme["nav_button_active_color"], target.theme.nav_button_active_color));
        target.theme.button_radius = clampValue<std::uint8_t>(theme["button_radius"] | target.theme.button_radius, 0u, 50u);
        target.theme.border_width = clampValue<std::uint8_t>(theme["border_width"] | target.theme.border_width, 0u, 10u);
        target.theme.header_border_width = clampValue<std::uint8_t>(theme["header_border_width"] | target.theme.header_border_width, 0u, 10u);
    }

    JsonObjectConst wifi = json["wifi"];
    if (!wifi.isNull()) {
        JsonObjectConst ap = wifi["ap"];
        if (!ap.isNull()) {
            target.wifi.ap.enabled = ap["enabled"] | true;
            target.wifi.ap.ssid = safeString(ap["ssid"], target.wifi.ap.ssid);
            target.wifi.ap.password = safeString(ap["password"], target.wifi.ap.password);
        }

        JsonObjectConst sta = wifi["sta"];
        if (!sta.isNull()) {
            target.wifi.sta.enabled = sta["enabled"] | false;
            target.wifi.sta.ssid = safeString(sta["ssid"], target.wifi.sta.ssid);
            target.wifi.sta.password = safeString(sta["password"], target.wifi.sta.password);
        }
    }

    target.pages.clear();
    JsonArrayConst pages = json["pages"].as<JsonArrayConst>();
    if (!pages.isNull()) {
        std::size_t page_index = 0;
        for (JsonObjectConst page_obj : pages) {
            if (page_index >= MAX_PAGES) {
                break;
            }

            PageConfig page;
            page.id = safeString(page_obj["id"], fallbackId("page", page_index));
            page.name = safeString(page_obj["name"], page.id);
            page.nav_color = sanitizeColor(safeString(page_obj["nav_color"], ""));
            page.rows = clampValue<std::uint8_t>(page_obj["rows"] | 2, 1, 4);
            page.cols = clampValue<std::uint8_t>(page_obj["cols"] | 2, 1, 4);

            JsonArrayConst buttons = page_obj["buttons"].as<JsonArrayConst>();
            if (!buttons.isNull()) {
                std::size_t button_index = 0;
                for (JsonObjectConst btn_obj : buttons) {
                    if (button_index >= MAX_BUTTONS_PER_PAGE) {
                        break;
                    }

                    ButtonConfig button;
                    button.id = safeString(btn_obj["id"], fallbackId("btn", button_index));
                    button.label = safeString(btn_obj["label"], button.id);
                    button.color = sanitizeColor(safeString(btn_obj["color"], button.color));
                    button.pressed_color = sanitizeColor(safeString(btn_obj["pressed_color"], button.pressed_color));
                    button.icon = safeString(btn_obj["icon"], "");
                    button.row = clampValue<std::uint8_t>(btn_obj["row"] | 0, 0, page.rows - 1);
                    button.col = clampValue<std::uint8_t>(btn_obj["col"] | 0, 0, page.cols - 1);
                    button.row_span = clampValue<std::uint8_t>(btn_obj["row_span"] | 1, 1, page.rows - button.row);
                    button.col_span = clampValue<std::uint8_t>(btn_obj["col_span"] | 1, 1, page.cols - button.col);
                    button.momentary = btn_obj["momentary"] | false;
                    button.font_size = clampValue<std::uint8_t>(btn_obj["font_size"] | 24, 8, 72);
                    button.font_family = safeString(btn_obj["font_family"], "montserrat");
                    button.text_align = safeString(btn_obj["text_align"], "center");

                    JsonObjectConst can_obj = btn_obj["can"];
                    if (!can_obj.isNull()) {
                        button.can.enabled = can_obj["enabled"] | false;
                        button.can.pgn = can_obj["pgn"] | button.can.pgn;
    target.can_library.clear();
    JsonArrayConst can_library = json["can_library"].as<JsonArrayConst>();
    if (!can_library.isNull()) {
        std::size_t msg_index = 0;
        for (JsonObjectConst msg_obj : can_library) {
            if (msg_index >= 50) {  // Reasonable limit for CAN library
                break;
            }

            CanMessage msg;
            msg.id = safeString(msg_obj["id"], fallbackId("can_msg", msg_index));
            msg.name = safeString(msg_obj["name"], msg.id);
            msg.pgn = msg_obj["pgn"] | 0;
            msg.priority = clampValue<std::uint8_t>(msg_obj["priority"] | 6, 0u, 7u);
            msg.source_address = msg_obj["source_address"] | 0xF9;
            msg.destination_address = msg_obj["destination_address"] | 0xFF;
            msg.description = safeString(msg_obj["description"], "");

            JsonArrayConst data_arr = msg_obj["data"].as<JsonArrayConst>();
            if (!data_arr.isNull()) {
                std::size_t i = 0;
                for (JsonVariantConst byte_val : data_arr) {
                    if (i >= msg.data.size()) {
                        break;
                    }
                    msg.data[i] = clampValue<std::uint8_t>(byte_val | 0, 0u, 255u);
                    ++i;
                }
            }

            target.can_library.push_back(std::move(msg));
            ++msg_index;
        }
    }

                        button.can.priority = clampValue<std::uint8_t>(can_obj["priority"] | button.can.priority, 0u, 7u);
                        button.can.source_address = can_obj["source_address"] | button.can.source_address;
                        button.can.destination_address = can_obj["destination_address"] | button.can.destination_address;

                        JsonArrayConst data_arr = can_obj["data"].as<JsonArrayConst>();
                        if (!data_arr.isNull()) {
                            std::size_t i = 0;
                            for (JsonVariantConst byte_val : data_arr) {
                                if (i >= button.can.data.size()) {
                                    break;
                                }
                                button.can.data[i] = clampValue<std::uint8_t>(byte_val | 0, 0u, 255u);
                                ++i;
                            }
                        }
                    }

                    page.buttons.push_back(std::move(button));
                    ++button_index;
                }
            }

            target.pages.push_back(std::move(page));
            ++page_index;
        }
    }

    if (target.pages.empty()) {
        target.pages = buildDefaultConfig().pages;
    }

    return true;
}
