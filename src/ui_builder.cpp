// Reconstructed after clock removal; this implementation focuses on building
// the navigation UI, header branding, and device info modal without time/clock logic.

#include "ui_builder.h"

#include <Arduino.h>
#include <cstdlib>
#include <algorithm>

#include <ESP_Panel_Library.h>

#include "can_manager.h"
#include "config_manager.h"
#include "icon_library.h"
#include "ui_theme.h"
#include "assets/images.h"

extern ESP_Panel* panel;

UIBuilder& UIBuilder::instance() {
    static UIBuilder inst;
    return inst;
}

void UIBuilder::begin() {
    config_ = &ConfigManager::instance().getConfig();

    // Apply display settings before constructing UI
    loadSleepIcon();
    setBrightness(config_->display.brightness);
    createBaseScreen();
    createInfoModal();

    if (config_ && !config_->pages.empty()) {
        buildNavigation();
        buildPage(0);
    } else {
        buildNavigation();
        buildEmptyState();
    }

    updateHeaderBranding();
}

void UIBuilder::applyConfig(const DeviceConfig& config) {
    config_ = &config;

    loadSleepIcon();
    setBrightness(config.display.brightness);

    buildNavigation();
    if (config_->pages.empty()) {
        buildEmptyState();
    } else {
        if (active_page_ >= config_->pages.size()) {
            active_page_ = 0;
        }
        buildPage(active_page_);
    }

    updateHeaderBranding();
}

void UIBuilder::markDirty() {
    dirty_ = true;
}

bool UIBuilder::consumeDirtyFlag() {
    const bool was_dirty = dirty_;
    dirty_ = false;
    return was_dirty;
}

void UIBuilder::updateNetworkStatus(const std::string& ap_ip, const std::string& sta_ip, bool sta_connected) {
    last_ap_ip_ = ap_ip;
    last_sta_ip_ = sta_ip;
    last_sta_connected_ = sta_connected;
}

void UIBuilder::createBaseScreen() {
    // Root screen
    base_screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(base_screen_, 800, 480);
    lv_obj_clear_flag(base_screen_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(base_screen_, 0, 0);

    // Apply theme colors if available
    lv_color_t page_bg_color = config_ ? colorFromHex(config_->theme.page_bg_color, UITheme::COLOR_BG) : UITheme::COLOR_BG;
    lv_obj_set_style_bg_color(base_screen_, page_bg_color, 0);
    lv_obj_set_style_bg_opa(base_screen_, LV_OPA_COVER, 0);

    lv_scr_load(base_screen_);

    // Header bar
    header_bar_ = lv_obj_create(base_screen_);
    lv_obj_set_size(header_bar_, 800, 110); // Increase header height to match web and prevent title overflow
    lv_obj_set_style_bg_color(header_bar_, colorFromHex(config_ ? config_->theme.surface_color : "#2A2A2A", UITheme::COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(header_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_bar_, config_ ? config_->theme.header_border_width : 0, 0);
    lv_obj_set_style_border_color(header_bar_, colorFromHex(config_ ? config_->theme.header_border_color : "#FFA500", UITheme::COLOR_ACCENT), 0);
    lv_obj_set_style_border_side(header_bar_, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_opa(header_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header_bar_, 0, 0);
    lv_obj_set_style_pad_all(header_bar_, UITheme::SPACE_SM, 0); // Reduce padding
    lv_obj_set_flex_flow(header_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_bar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_bar_, LV_OBJ_FLAG_SCROLLABLE);

    // Header logo
    header_logo_img_ = lv_img_create(header_bar_);
    lv_obj_set_style_pad_right(header_logo_img_, UITheme::SPACE_MD, 0);
    lv_obj_set_style_border_width(header_logo_img_, 0, 0);

    // Text column
    lv_obj_t* header_text_column = lv_obj_create(header_bar_);
    lv_obj_remove_style_all(header_text_column);
    lv_obj_set_flex_flow(header_text_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(header_text_column, UITheme::SPACE_XL, 0);   // 32px top padding for more space
    lv_obj_set_style_pad_bottom(header_text_column, UITheme::SPACE_SM, 0); // 8px bottom padding
    lv_obj_set_style_pad_left(header_text_column, 0, 0);
    lv_obj_set_style_pad_right(header_text_column, 0, 0);
    lv_obj_set_style_pad_gap(header_text_column, UITheme::SPACE_SM, 0);    // 8px gap between title/subtitle
    lv_obj_clear_flag(header_text_column, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(header_text_column, 1);

    header_title_label_ = lv_label_create(header_text_column);
    // Clamp title font size to max 20px
    const lv_font_t* title_font = fontFromName(config_ ? config_->header.title_font : "montserrat_20");
    lv_obj_set_style_text_font(header_title_label_, title_font, 0);
    lv_obj_set_style_text_color(header_title_label_, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);

    header_subtitle_label_ = lv_label_create(header_text_column);
    // Clamp subtitle font size to max 14px
    const lv_font_t* subtitle_font = fontFromName(config_ ? config_->header.subtitle_font : "montserrat_14");
    lv_obj_set_style_text_font(header_subtitle_label_, subtitle_font, 0);
    lv_obj_set_style_text_color(header_subtitle_label_, config_ ? colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY) : UITheme::COLOR_TEXT_SECONDARY, 0);

    // Settings/info button on the right
    lv_obj_t* header_spacer = lv_obj_create(header_bar_);
    lv_obj_remove_style_all(header_spacer);
    lv_obj_set_size(header_spacer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(header_spacer, 1);

    lv_obj_t* settings_btn = lv_btn_create(header_bar_);
    lv_obj_remove_style_all(settings_btn);
    lv_obj_set_size(settings_btn, 44, 44);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_btn, 0, 0);
    lv_obj_set_style_radius(settings_btn, 0, 0);
    lv_obj_add_event_cb(settings_btn, settingsButtonEvent, LV_EVENT_CLICKED, nullptr);

    // Content root (below header)
    content_root_ = lv_obj_create(base_screen_);
    lv_obj_set_size(content_root_, 800, 400);
    lv_obj_set_style_bg_color(content_root_, page_bg_color, 0);
    lv_obj_set_style_bg_opa(content_root_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(content_root_, UITheme::SPACE_MD, 0);
    lv_obj_set_flex_flow(content_root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(content_root_, UITheme::SPACE_MD, 0);
    lv_obj_set_pos(content_root_, 0, 80);
    lv_obj_clear_flag(content_root_, LV_OBJ_FLAG_SCROLLABLE);

    // Navigation bar
    nav_bar_ = lv_obj_create(content_root_);
    lv_obj_set_width(nav_bar_, 800);
    lv_obj_set_height(nav_bar_, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(nav_bar_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_bar_, 0, 0);
    lv_obj_set_style_pad_all(nav_bar_, UITheme::SPACE_XS, 0);
    lv_obj_set_style_pad_gap(nav_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_flex_flow(nav_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(nav_bar_, LV_OBJ_FLAG_SCROLLABLE);

    // Status panel (kept hidden but retained for future use)
    status_panel_ = lv_obj_create(content_root_);
    lv_obj_remove_style_all(status_panel_);
    lv_obj_set_width(status_panel_, 800);
    lv_obj_set_height(status_panel_, LV_SIZE_CONTENT);
    lv_obj_add_flag(status_panel_, LV_OBJ_FLAG_HIDDEN);

    // Helper for chip creation (used only if status panel is shown later)
    auto create_chip = [&](lv_obj_t* parent, lv_color_t bg) {
        lv_obj_t* chip = lv_obj_create(parent);
        lv_obj_set_style_bg_color(chip, bg, 0);
        lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(chip, 0, 0);
        lv_obj_set_style_radius(chip, UITheme::RADIUS_LG, 0);
        lv_obj_set_style_pad_all(chip, UITheme::SPACE_MD, 0);
        lv_obj_set_style_min_width(chip, 180, 0);
        lv_obj_set_style_max_width(chip, 360, 0);
        lv_obj_set_size(chip, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        return chip;
    };

    status_ap_chip_ = create_chip(status_panel_, UITheme::COLOR_SURFACE);
    status_ap_label_ = lv_label_create(status_ap_chip_);
    lv_obj_set_style_text_font(status_ap_label_, UITheme::FONT_BODY, 0);
    lv_obj_set_style_text_color(status_ap_label_, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
    lv_label_set_text(status_ap_label_, "AP —");

    status_sta_chip_ = create_chip(status_panel_, UITheme::COLOR_SURFACE);
    status_sta_label_ = lv_label_create(status_sta_chip_);
    lv_obj_set_style_text_font(status_sta_label_, UITheme::FONT_BODY, 0);
    lv_obj_set_style_text_color(status_sta_label_, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
    lv_label_set_text(status_sta_label_, "LAN waiting...");

    // Page container
    page_container_ = lv_obj_create(content_root_);
    lv_obj_set_size(page_container_, 760, 300);
    lv_color_t page_bg = config_ ? colorFromHex(config_->theme.page_bg_color, UITheme::COLOR_BG) : UITheme::COLOR_BG;
    lv_obj_set_style_bg_color(page_container_, page_bg, 0);
    lv_obj_set_style_bg_opa(page_container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page_container_, 0, 0);
    lv_obj_set_style_pad_all(page_container_, UITheme::SPACE_SM, 0);
    lv_obj_clear_flag(page_container_, LV_OBJ_FLAG_SCROLLABLE);
}

void UIBuilder::buildNavigation() {
    lv_obj_clean(nav_bar_);
    nav_buttons_.clear();

    if (!config_ || config_->pages.empty()) {
        return;
    }

    std::size_t index = 0;
    nav_buttons_.reserve(config_->pages.size());

    for (const auto& page : config_->pages) {
        lv_obj_t* btn = lv_btn_create(nav_bar_);

        // Use page-specific color or fall back to theme default
        lv_color_t nav_color = page.nav_color.empty() ?
            colorFromHex(config_->theme.nav_button_color, UITheme::COLOR_SURFACE) :
            colorFromHex(page.nav_color, UITheme::COLOR_SURFACE);

        lv_obj_set_style_bg_color(btn, nav_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        // Set active state color
        lv_color_t active_color = colorFromHex(config_->theme.nav_button_active_color, UITheme::COLOR_ACCENT);
        lv_obj_set_style_bg_color(btn, active_color, LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);

        lv_obj_set_style_border_width(btn, config_->theme.border_width, 0);
        lv_obj_set_style_border_color(btn, colorFromHex(config_->theme.border_color, UITheme::COLOR_BORDER), 0);
        lv_obj_set_style_radius(btn, config_->theme.button_radius, 0);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, 50);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn, navButtonEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(index)));

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, page.name.c_str());
        lv_obj_set_style_text_font(label, UITheme::FONT_BODY, 0);
        lv_obj_set_style_text_color(label, colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY), 0);
        lv_obj_center(label);

        nav_buttons_.push_back(btn);
        ++index;
    }

    updateNavSelection();
}

void UIBuilder::buildEmptyState() {
    lv_obj_clean(page_container_);
    lv_obj_t* label = lv_label_create(page_container_);
    lv_label_set_text(label, "No pages configured. Use the web interface to add controls.");
    lv_obj_set_style_text_font(label, UITheme::FONT_BODY, 0);
    lv_obj_set_style_text_color(label, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void UIBuilder::buildPage(std::size_t index) {
    if (!config_ || index >= config_->pages.size()) {
        buildEmptyState();
        return;
    }

    active_page_ = index;
    const PageConfig& page = config_->pages[index];

    const bool page_style_active = !page.bg_color.empty() || !page.text_color.empty() ||
                                   !page.button_color.empty() || !page.button_pressed_color.empty() ||
                                   !page.button_border_color.empty() ||
                                   page.button_border_width > 0 || page.button_radius > 0;

    const std::string page_bg_hex = !page.bg_color.empty()
        ? page.bg_color
        : (config_ ? config_->theme.page_bg_color : "#0F0F0F");
    lv_obj_set_style_bg_color(page_container_, colorFromHex(page_bg_hex, UITheme::COLOR_BG), 0);

    lv_obj_clean(page_container_);

    grid_cols_.assign(page.cols + 1, LV_GRID_TEMPLATE_LAST);
    grid_rows_.assign(page.rows + 1, LV_GRID_TEMPLATE_LAST);
    for (std::size_t i = 0; i < page.cols; ++i) {
        grid_cols_[i] = LV_GRID_FR(1);
    }
    grid_cols_.back() = LV_GRID_TEMPLATE_LAST;
    for (std::size_t i = 0; i < page.rows; ++i) {
        grid_rows_[i] = LV_GRID_FR(1);
    }
    grid_rows_.back() = LV_GRID_TEMPLATE_LAST;

    lv_obj_set_layout(page_container_, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_gap(page_container_, UITheme::SPACE_XS, 0);
    lv_obj_set_grid_dsc_array(page_container_, grid_cols_.data(), grid_rows_.data());

    if (page.buttons.empty()) {
        lv_obj_t* label = lv_label_create(page_container_);
        lv_label_set_text(label, "This page has no buttons yet.");
        lv_obj_set_style_text_font(label, UITheme::FONT_BODY, 0);
        lv_obj_set_style_text_color(label, colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        updateNavSelection();
        return;
    }

    for (const auto& button : page.buttons) {
        lv_obj_t* btn = lv_btn_create(page_container_);
        lv_obj_remove_style_all(btn);

        // Apply per-page overrides when provided
        const uint8_t radius = page_style_active
            ? (page.button_radius > 0 ? page.button_radius : (config_ ? config_->theme.button_radius : UITheme::RADIUS_LG))
            : (config_ ? config_->theme.button_radius : UITheme::RADIUS_LG);
        const uint8_t border_width = page_style_active
            ? page.button_border_width
            : (config_ ? config_->theme.border_width : 0);
        const lv_color_t border_color = page_style_active && !page.button_border_color.empty()
            ? colorFromHex(page.button_border_color, UITheme::COLOR_BORDER)
            : (config_ ? colorFromHex(config_->theme.border_color, UITheme::COLOR_BORDER) : UITheme::COLOR_BORDER);
        lv_obj_set_style_radius(btn, radius, 0);
        lv_obj_set_style_border_width(btn, border_width, 0);
        lv_obj_set_style_border_color(btn, border_color, 0);
        lv_obj_set_style_border_opa(btn, border_width > 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

        // Normal state color
        const std::string button_color_hex = page_style_active && !page.button_color.empty()
            ? page.button_color
            : button.color;
        lv_color_t btn_color = colorFromHex(button_color_hex, UITheme::COLOR_ACCENT);
        lv_obj_set_style_bg_color(btn, btn_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        // Pressed state color - use custom if set, otherwise darken
        const std::string pressed_hex = page_style_active && !page.button_pressed_color.empty()
            ? page.button_pressed_color
            : button.pressed_color;
        lv_color_t pressed_color = pressed_hex.empty()
            ? lv_color_darken(btn_color, LV_OPA_40)
            : colorFromHex(pressed_hex, lv_color_darken(btn_color, LV_OPA_40));
        lv_obj_set_style_bg_color(btn, pressed_color, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

        lv_obj_set_style_pad_all(btn, UITheme::SPACE_MD, 0);
        lv_obj_set_grid_cell(btn,
                             LV_GRID_ALIGN_STRETCH, button.col, button.col_span,
                             LV_GRID_ALIGN_STRETCH, button.row, button.row_span);
        lv_obj_add_event_cb(btn, actionButtonEvent, LV_EVENT_CLICKED, (void*)&button);

        // Create icon if specified
        if (!button.icon.empty() && button.icon != "none") {
            IconType icon_type = IconLibrary::icon_from_string(button.icon);
            if (icon_type != IconType::NONE) {
                lv_obj_t* icon_obj = IconLibrary::create_icon(btn, icon_type, btn_color);
                if (icon_obj) {
                    lv_obj_align(icon_obj, LV_ALIGN_TOP_RIGHT, -UITheme::SPACE_SM, UITheme::SPACE_SM);
                }
            }
        }

        lv_obj_t* title = lv_label_create(btn);
        lv_label_set_text(title, button.label.c_str());
        const std::string text_hex = page_style_active && !page.text_color.empty()
            ? page.text_color
            : (config_ ? config_->theme.text_primary : "#FFFFFF");
        lv_obj_set_style_text_color(title, colorFromHex(text_hex, UITheme::COLOR_TEXT_PRIMARY), 0);

        // Use font_name if specified, otherwise use font_family + font_size
        const lv_font_t* font;
        if (!button.font_name.empty() && button.font_name != "montserrat_16") {
            font = fontFromName(button.font_name);
        } else if (!button.font_family.empty() && button.font_family != "montserrat") {
            // Use font family with size
            std::string fontKey = button.font_family + "_" + std::to_string(button.font_size);
            font = fontFromName(fontKey);
        } else {
            // Default montserrat with size mapping
            if (button.font_size <= 13) font = &lv_font_montserrat_12;
            else if (button.font_size <= 15) font = &lv_font_montserrat_14;
            else if (button.font_size <= 17) font = &lv_font_montserrat_16;
            else if (button.font_size <= 19) font = &lv_font_montserrat_18;
            else if (button.font_size <= 21) font = &lv_font_montserrat_20;
            else if (button.font_size <= 23) font = &lv_font_montserrat_22;
            else if (button.font_size <= 25) font = &lv_font_montserrat_24;
            else if (button.font_size <= 27) font = &lv_font_montserrat_26;
            else if (button.font_size <= 29) font = &lv_font_montserrat_28;
            else if (button.font_size <= 31) font = &lv_font_montserrat_30;
            else font = &lv_font_montserrat_32;
        }
        lv_obj_set_style_text_font(title, font, 0);

        // Apply text alignment
        lv_align_t align = LV_ALIGN_CENTER;
        if (button.text_align == "top-left") align = LV_ALIGN_TOP_LEFT;
        else if (button.text_align == "top-center") align = LV_ALIGN_TOP_MID;
        else if (button.text_align == "top-right") align = LV_ALIGN_TOP_RIGHT;
        else if (button.text_align == "center") align = LV_ALIGN_CENTER;
        else if (button.text_align == "bottom-left") align = LV_ALIGN_BOTTOM_LEFT;
        else if (button.text_align == "bottom-center") align = LV_ALIGN_BOTTOM_MID;
        else if (button.text_align == "bottom-right") align = LV_ALIGN_BOTTOM_RIGHT;
        lv_obj_align(title, align, 0, 0);
    }

    updateNavSelection();
}

void UIBuilder::updateNavSelection() {
    for (std::size_t i = 0; i < nav_buttons_.size(); ++i) {
        lv_obj_t* btn = nav_buttons_[i];
        if (!btn) {
            continue;
        }
        if (i == active_page_) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    }
}

void UIBuilder::navButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    std::size_t index = static_cast<std::size_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    UIBuilder::instance().buildPage(index);
}

void UIBuilder::actionButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    auto* config = static_cast<const ButtonConfig*>(lv_event_get_user_data(e));
    if (!config) {
        return;
    }
    CanManager::instance().sendButtonAction(*config);
}

lv_color_t UIBuilder::colorFromHex(const std::string& hex, lv_color_t fallback) {
    if (hex.size() != 7 || hex[0] != '#') {
        return fallback;
    }
    char* end_ptr = nullptr;
    unsigned long value = strtoul(hex.c_str() + 1, &end_ptr, 16);
    if (end_ptr == nullptr || *end_ptr != '\0') {
        return fallback;
    }
    return lv_color_hex(static_cast<uint32_t>(value));
}

// Base64 decoder for logo images
std::vector<uint8_t> UIBuilder::decodeBase64Logo(const std::string& data_uri) {
    std::vector<uint8_t> result;
    
    // Find the base64 data after "base64,"
    const std::string prefix = "base64,";
    size_t pos = data_uri.find(prefix);
    if (pos == std::string::npos) {
        return result;
    }
    
    std::string base64_data = data_uri.substr(pos + prefix.length());
    
    // Base64 decode table
    static const uint8_t decode_table[256] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
        52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
    };
    
    // Reserve approximate size (3 bytes per 4 characters)
    result.reserve(base64_data.length() * 3 / 4);
    
    // Decode
    uint32_t val = 0;
    int valb = -8;
    for (uint8_t c : base64_data) {
        if (decode_table[c] == 64) break;
        val = (val << 6) + decode_table[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    
    return result;
}

void UIBuilder::updateHeaderBranding() {
    if (!config_ || !header_title_label_ || !header_bar_) {
        return;
    }

    // Update title text and font
    lv_label_set_text(header_title_label_, config_->header.title.c_str());
    const lv_font_t* title_font = fontFromName(config_->header.title_font);
    lv_obj_set_style_text_font(header_title_label_, title_font, 0);

    // Update subtitle text and font
    if (header_subtitle_label_) {
        if (config_->header.subtitle.empty()) {
            lv_obj_add_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(header_subtitle_label_, config_->header.subtitle.c_str());
            const lv_font_t* subtitle_font = fontFromName(config_->header.subtitle_font);
            lv_obj_set_style_text_font(header_subtitle_label_, subtitle_font, 0);
            lv_obj_clear_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (!header_logo_img_) {
        return;
    }

    Serial.printf("[UI] Logo config - show_logo: %d, logo_variant: '%s'\n", 
                  config_->header.show_logo, 
                  config_->header.logo_variant.c_str());
    
    if (config_->header.show_logo) {
        // Use built-in logo from logo_variant
        if (const lv_img_dsc_t* logo = iconForId(config_->header.logo_variant)) {
            Serial.printf("[UI] Setting logo image source for variant: %s\n", config_->header.logo_variant.c_str());
            lv_img_set_src(header_logo_img_, logo);
            lv_obj_clear_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(header_logo_img_);  // Force redraw
        } else {
            Serial.printf("[UI] No logo found for variant: %s\n", config_->header.logo_variant.c_str());
            lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        Serial.println("[UI] Logo disabled in config");
        lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
    }
}

const lv_img_dsc_t* UIBuilder::iconForId(const std::string& id) const {
    // Map known logo IDs to compiled image descriptors; default to Bronco logo
    if (id == "bronco" || id.empty()) {
        return &img_bronco_logo;
    }
    return nullptr;
}

void UIBuilder::createInfoModal() {
    // Modal background (covers entire screen, dark overlay)
    info_modal_bg_ = lv_obj_create(base_screen_);
    lv_obj_set_size(info_modal_bg_, 800, 480);
    lv_obj_set_pos(info_modal_bg_, 0, 0);
    lv_obj_set_style_bg_color(info_modal_bg_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(info_modal_bg_, LV_OPA_70, 0);
    lv_obj_set_style_border_width(info_modal_bg_, 0, 0);
    lv_obj_add_flag(info_modal_bg_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(info_modal_bg_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(info_modal_bg_, infoModalCloseEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_move_foreground(info_modal_bg_);  // Ensure modal is on top

    // Modal content box - larger to fit time controls
    info_modal_ = lv_obj_create(info_modal_bg_);
    lv_obj_set_size(info_modal_, 550, 480);  // Increased height for AM/PM button
    lv_obj_center(info_modal_);
    lv_obj_set_style_bg_color(info_modal_, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(info_modal_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(info_modal_, 3, 0);
    lv_obj_set_style_border_color(info_modal_, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_border_opa(info_modal_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info_modal_, 16, 0);
    lv_obj_set_style_pad_all(info_modal_, UITheme::SPACE_LG, 0);
    lv_obj_set_flex_flow(info_modal_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_modal_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(info_modal_, UITheme::SPACE_MD, 0);
    lv_obj_add_flag(info_modal_, LV_OBJ_FLAG_CLICKABLE);  // Block clicks from reaching background
    lv_obj_clear_flag(info_modal_, LV_OBJ_FLAG_SCROLLABLE);  // Modal content shouldn't close on click

    // Title
    lv_obj_t* title = lv_label_create(info_modal_);
    lv_label_set_text(title, "Device Info");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(title, LV_OPA_COVER, 0);

    // IP address label
    lv_obj_t* ip_label = lv_label_create(info_modal_);
    lv_label_set_text(ip_label, "IP: Not connected");
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ip_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(ip_label, LV_OPA_COVER, 0);
    lv_label_set_long_mode(ip_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ip_label, 500);
    lv_obj_set_user_data(info_modal_, ip_label);

    // Brightness controls
    lv_obj_t* brightness_row = lv_obj_create(info_modal_);
    lv_obj_remove_style_all(brightness_row);
    lv_obj_set_flex_flow(brightness_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(brightness_row, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_all(brightness_row, 0, 0);
    lv_obj_set_width(brightness_row, 500);

    lv_obj_t* brightness_label = lv_label_create(brightness_row);
    lv_label_set_text(brightness_label, "Brightness");
    lv_obj_set_style_text_font(brightness_label, UITheme::FONT_BODY, 0);

    brightness_slider_ = lv_slider_create(brightness_row);
    lv_slider_set_range(brightness_slider_, 0, 100);
    lv_slider_set_value(brightness_slider_, config_ ? config_->display.brightness : 100, LV_ANIM_OFF);
    lv_obj_set_width(brightness_slider_, 260);
    lv_obj_add_event_cb(brightness_slider_, brightnessSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(brightness_slider_, brightnessSliderEvent, LV_EVENT_RELEASED, nullptr);

    brightness_value_label_ = lv_label_create(brightness_row);
    char pct_buf[8];
    snprintf(pct_buf, sizeof(pct_buf), "%u%%", static_cast<unsigned>(config_ ? config_->display.brightness : 100));
    lv_label_set_text(brightness_value_label_, pct_buf);
    lv_obj_set_style_text_font(brightness_value_label_, UITheme::FONT_BODY, 0);

    // Time controls removed; this modal now only shows device info.

    // Close button
    lv_obj_t* close_btn = lv_btn_create(info_modal_);
    lv_obj_set_size(close_btn, 120, 40);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_pad_top(close_btn, UITheme::SPACE_SM, 0);
    lv_obj_add_event_cb(close_btn, infoModalCloseEvent, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_set_style_text_font(close_label, UITheme::FONT_BODY, 0);
    lv_obj_center(close_label);

    // Sleep overlay (hidden until timeout)
    sleep_overlay_ = lv_obj_create(info_modal_bg_);
    lv_obj_set_size(sleep_overlay_, 800, 480);
    lv_obj_set_pos(sleep_overlay_, 0, 0);
    lv_obj_set_style_bg_color(sleep_overlay_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(sleep_overlay_, LV_OPA_90, 0);
    lv_obj_set_style_border_width(sleep_overlay_, 0, 0);
    lv_obj_add_flag(sleep_overlay_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(sleep_overlay_, modalActivityEvent, LV_EVENT_CLICKED, nullptr);

    sleep_image_ = lv_img_create(sleep_overlay_);
    lv_obj_center(sleep_image_);

    // Reset sleep timer on any interaction within modal
    lv_obj_add_event_cb(info_modal_, modalActivityEvent, LV_EVENT_ALL, nullptr);
}

void UIBuilder::showInfoModal() {
    if (!info_modal_bg_ || !info_modal_) {
        return;
    }

    // Update IP address in modal
    lv_obj_t* ip_label = static_cast<lv_obj_t*>(lv_obj_get_user_data(info_modal_));
    if (ip_label) {
        std::string ip_text = "IP: ";
        if (last_sta_connected_ && !last_sta_ip_.empty() && last_sta_ip_ != "0.0.0.0") {
            ip_text += last_sta_ip_;
        } else if (!last_ap_ip_.empty() && last_ap_ip_ != "0.0.0.0") {
            ip_text += last_ap_ip_;
        } else {
            ip_text += "Not connected";
        }
        lv_label_set_text(ip_label, ip_text.c_str());
    }

    lv_obj_move_foreground(info_modal_bg_);  // Ensure modal is on top layer
    lv_obj_clear_flag(info_modal_bg_, LV_OBJ_FLAG_HIDDEN);

    resetSleepTimer();
    armSleepTimer();
}

void UIBuilder::hideInfoModal() {
    if (info_modal_bg_) {
        lv_obj_add_flag(info_modal_bg_, LV_OBJ_FLAG_HIDDEN);
    }
    if (sleep_timer_) {
        lv_timer_del(sleep_timer_);
        sleep_timer_ = nullptr;
    }
    hideSleepOverlay();
}

void UIBuilder::settingsButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    Serial.println("[UI] Settings button clicked - showing settings modal");
    UIBuilder::instance().showInfoModal();
}
void UIBuilder::infoModalCloseEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    UIBuilder::instance().hideInfoModal();
}

void UIBuilder::brightnessSliderEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED && lv_event_get_code(e) != LV_EVENT_RELEASED) {
        return;
    }
    lv_obj_t* slider = lv_event_get_target(e);
    uint8_t value = static_cast<uint8_t>(lv_slider_get_value(slider));
    UIBuilder::instance().setBrightness(value);
    UIBuilder::instance().resetSleepTimer();
}

void UIBuilder::modalActivityEvent(lv_event_t* e) {
    UIBuilder::instance().resetSleepTimer();
    if (UIBuilder::instance().sleep_overlay_) {
        lv_obj_add_flag(UIBuilder::instance().sleep_overlay_, LV_OBJ_FLAG_HIDDEN);
    }
}

void UIBuilder::setBrightness(uint8_t percent) {
    percent = std::min<uint8_t>(100, percent);
    DeviceConfig& cfg = ConfigManager::instance().getConfig();
    const bool changed = cfg.display.brightness != percent;
    cfg.display.brightness = percent;

    if (panel && panel->getBacklight()) {
        panel->getBacklight()->setBrightness(percent);
    }

    if (brightness_slider_ && lv_slider_get_value(brightness_slider_) != percent) {
        lv_slider_set_value(brightness_slider_, percent, LV_ANIM_OFF);
    }
    if (brightness_value_label_) {
        char pct_buf[8];
        snprintf(pct_buf, sizeof(pct_buf), "%u%%", static_cast<unsigned>(percent));
        lv_label_set_text(brightness_value_label_, pct_buf);
    }

    if (changed) {
        ConfigManager::instance().save();
    }
}

void UIBuilder::loadSleepIcon() {
    sleep_icon_buffer_.clear();

    if (!config_ || config_->display.sleep_icon_base64.empty()) {
        return;
    }

    sleep_icon_buffer_ = decodeBase64Logo(config_->display.sleep_icon_base64);
}

void UIBuilder::armSleepTimer() {
    if (!config_ || !config_->display.sleep_enabled) {
        return;
    }
    if (sleep_timer_) {
        lv_timer_del(sleep_timer_);
        sleep_timer_ = nullptr;
    }
    uint32_t period_ms = static_cast<uint32_t>(config_->display.sleep_timeout_seconds) * 1000U;
    sleep_timer_ = lv_timer_create([](lv_timer_t*) {
        UIBuilder::instance().showSleepOverlay();
    }, period_ms, nullptr);
}

void UIBuilder::resetSleepTimer() {
    if (sleep_timer_) {
        lv_timer_reset(sleep_timer_);
    } else if (config_ && config_->display.sleep_enabled) {
        armSleepTimer();
    }
    hideSleepOverlay();
}

void UIBuilder::showSleepOverlay() {
    if (!sleep_overlay_) {
        return;
    }
    if (!sleep_icon_buffer_.empty()) {
        lv_img_set_src(sleep_image_, sleep_icon_buffer_.data());
    } else {
        lv_img_set_src(sleep_image_, &img_bronco_logo);
    }
    lv_obj_center(sleep_image_);
    lv_obj_clear_flag(sleep_overlay_, LV_OBJ_FLAG_HIDDEN);
}

void UIBuilder::hideSleepOverlay() {
    if (sleep_overlay_) {
        lv_obj_add_flag(sleep_overlay_, LV_OBJ_FLAG_HIDDEN);
    }
}

const lv_font_t* UIBuilder::fontFromName(const std::string& name) const {
    // Map font names to actual LVGL fonts - Montserrat
    if (name == "montserrat_12") return &lv_font_montserrat_12;
    if (name == "montserrat_14") return &lv_font_montserrat_14;
    if (name == "montserrat_16") return &lv_font_montserrat_16;
    if (name == "montserrat_18") return &lv_font_montserrat_18;
    if (name == "montserrat_20") return &lv_font_montserrat_20;
    if (name == "montserrat_22") return &lv_font_montserrat_22;
    if (name == "montserrat_24") return &lv_font_montserrat_24;
    if (name == "montserrat_26") return &lv_font_montserrat_26;
    if (name == "montserrat_28") return &lv_font_montserrat_28;
    if (name == "montserrat_30") return &lv_font_montserrat_30;
    if (name == "montserrat_32") return &lv_font_montserrat_32;
    
    // UNSCII monospace fonts
    if (name == "unscii_8") return &lv_font_unscii_8;
    if (name == "unscii_16") return &lv_font_unscii_16;
    
    // Default fallback
    return &lv_font_montserrat_16;
}
