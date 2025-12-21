// Reconstructed after clock removal; this implementation focuses on building
// the navigation UI, header branding, and device info modal without time/clock logic.

#include "ui_builder.h"

#include <Arduino.h>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <cctype>

#include <ESP_Panel_Library.h>

#include "can_manager.h"
#include "config_manager.h"
#include "ota_manager.h"
#include "icon_library.h"
#include "ui_theme.h"
#include "assets/images.h"
#include "version_auto.h"

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
    refreshNetworkStatusLabel();
}

void UIBuilder::createBaseScreen() {
    lv_disp_t* disp = lv_disp_get_default();
    lv_coord_t screen_w = disp ? lv_disp_get_hor_res(disp) : 800;
    lv_coord_t screen_h = disp ? lv_disp_get_ver_res(disp) : 480;

    // Root screen
    base_screen_ = lv_obj_create(nullptr);
    lv_obj_set_size(base_screen_, screen_w, screen_h);
    lv_obj_clear_flag(base_screen_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(base_screen_, 0, 0);

    // Apply shell background using the active page color so no border shows around the UI
    lv_color_t flush_bg = UITheme::COLOR_SURFACE;
    if (config_) {
        if (!config_->theme.page_bg_color.empty()) {
            flush_bg = colorFromHex(config_->theme.page_bg_color, flush_bg);
        } else if (!config_->theme.surface_color.empty()) {
            flush_bg = colorFromHex(config_->theme.surface_color, flush_bg);
        } else if (!config_->theme.bg_color.empty()) {
            flush_bg = colorFromHex(config_->theme.bg_color, flush_bg);
        }
    }
    lv_obj_set_style_bg_color(base_screen_, flush_bg, 0);
    lv_obj_set_style_bg_opa(base_screen_, LV_OPA_COVER, 0);

    lv_scr_load(base_screen_);

    // Shell container to center the card
    lv_obj_t* shell = lv_obj_create(base_screen_);
    lv_obj_remove_style_all(shell);
    lv_obj_set_size(shell, 800, 480);
    lv_obj_center(shell);
    lv_obj_set_style_bg_opa(shell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(shell, 0, 0);
    lv_obj_clear_flag(shell, LV_OBJ_FLAG_SCROLLABLE);

    // Main card - fullscreen
    lv_color_t card_bg_color = flush_bg;
    lv_obj_t* main_container = lv_obj_create(shell);
    lv_obj_set_size(main_container, screen_w, screen_h);
    lv_obj_set_pos(main_container, 0, 0);
    lv_obj_set_style_bg_color(main_container, card_bg_color, 0);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_style_pad_gap(main_container, 0, 0);
    lv_obj_set_style_shadow_width(main_container, 0, 0);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);

    // Header - scales with content, respects text alignment
    header_bar_ = lv_obj_create(main_container);
    lv_obj_remove_style_all(header_bar_);
    lv_obj_set_width(header_bar_, lv_pct(100));
    lv_obj_set_height(header_bar_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_left(header_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_right(header_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_top(header_bar_, UITheme::SPACE_XS, 0);
    lv_obj_set_style_pad_bottom(header_bar_, UITheme::SPACE_XS, 0);
    lv_obj_set_style_pad_gap(header_bar_, UITheme::SPACE_XS, 0);
    lv_obj_set_flex_flow(header_bar_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_bar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(header_bar_, LV_OBJ_FLAG_SCROLLABLE);

    const lv_coord_t header_text_width = screen_w - (UITheme::SPACE_SM * 2);

    header_brand_row_ = lv_obj_create(header_bar_);
    lv_obj_remove_style_all(header_brand_row_);
    lv_obj_set_width(header_brand_row_, lv_pct(100));
    lv_obj_set_style_pad_all(header_brand_row_, 0, 0);
    lv_obj_set_style_pad_gap(header_brand_row_, UITheme::SPACE_XS, 0);
    lv_obj_set_flex_flow(header_brand_row_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_brand_row_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(header_brand_row_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(header_brand_row_, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(header_brand_row_, 0, 0);
    lv_obj_set_style_border_color(header_brand_row_, lv_color_hex(0x000000), 0);

    // Optional header logo (hidden by default)
    header_logo_slot_ = lv_obj_create(header_brand_row_);
    lv_obj_remove_style_all(header_logo_slot_);
    lv_obj_set_width(header_logo_slot_, LV_SIZE_CONTENT);
    lv_obj_set_height(header_logo_slot_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(header_logo_slot_, 0, 0);
    lv_obj_set_style_bg_opa(header_logo_slot_, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(header_logo_slot_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_logo_slot_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_logo_slot_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(header_logo_slot_, LV_OBJ_FLAG_HIDDEN);

    header_logo_img_ = lv_img_create(header_logo_slot_);
    lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_center(header_logo_img_);

    header_text_container_ = lv_obj_create(header_brand_row_);
    lv_obj_remove_style_all(header_text_container_);
    lv_obj_set_width(header_text_container_, lv_pct(100));
    lv_obj_set_style_pad_all(header_text_container_, 0, 0);
    lv_obj_set_style_pad_gap(header_text_container_, UITheme::SPACE_XS, 0);
    lv_obj_set_flex_flow(header_text_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_text_container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(header_text_container_, 1);
    lv_obj_clear_flag(header_text_container_, LV_OBJ_FLAG_SCROLLABLE);

    // Title label - wraps text and scales with content
    header_title_label_ = lv_label_create(header_text_container_);
    const lv_font_t* title_font = fontFromName(config_ ? config_->header.title_font : "montserrat_12");
    lv_obj_set_style_text_font(header_title_label_, title_font, 0);
    lv_obj_set_style_text_color(header_title_label_, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
    lv_label_set_long_mode(header_title_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(header_title_label_, lv_pct(100));
    lv_obj_set_style_max_width(header_title_label_, header_text_width, 0);

    // Subtitle label - wraps text and scales with content
    header_subtitle_label_ = lv_label_create(header_text_container_);
    const lv_font_t* subtitle_font = fontFromName(config_ ? config_->header.subtitle_font : "montserrat_10");
    lv_obj_set_style_text_font(header_subtitle_label_, subtitle_font, 0);
    lv_obj_set_style_text_color(header_subtitle_label_, config_ ? colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY) : UITheme::COLOR_TEXT_SECONDARY, 0);
    lv_label_set_long_mode(header_subtitle_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(header_subtitle_label_, lv_pct(100));
    lv_obj_set_style_max_width(header_subtitle_label_, header_text_width, 0);

    // Navigation bar lives inside the header so the shell only has two sections (header + page)
    nav_bar_ = lv_obj_create(header_bar_);
    lv_obj_remove_style_all(nav_bar_);
    lv_obj_set_width(nav_bar_, lv_pct(100));
    lv_obj_set_height(nav_bar_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_left(nav_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_right(nav_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_top(nav_bar_, nav_base_pad_top_, 0);
    lv_obj_set_style_pad_bottom(nav_bar_, UITheme::SPACE_XS, 0);
    lv_obj_set_style_pad_gap(nav_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_flex_flow(nav_bar_, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(nav_bar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(nav_bar_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_bar_, 0, 0);
    lv_obj_set_style_border_opa(nav_bar_, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(nav_bar_, LV_OBJ_FLAG_SCROLLABLE);

    // Content root (below nav)
    content_root_ = lv_obj_create(main_container);
    lv_obj_remove_style_all(content_root_);
    lv_obj_set_width(content_root_, lv_pct(100));
    lv_obj_set_flex_grow(content_root_, 1);
    lv_obj_set_style_bg_color(content_root_, config_ ? colorFromHex(config_->theme.page_bg_color, UITheme::COLOR_SURFACE) : UITheme::COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(content_root_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(content_root_, 0, 0);
    lv_obj_set_style_shadow_width(content_root_, 0, 0);
    lv_obj_set_style_pad_all(content_root_, UITheme::SPACE_MD, 0);
    lv_obj_set_style_pad_gap(content_root_, UITheme::SPACE_SM, 0);
    lv_obj_clear_flag(content_root_, LV_OBJ_FLAG_SCROLLABLE);

    // Status panel (kept hidden but retained for future use) - on base_screen not content_root
    status_panel_ = lv_obj_create(base_screen_);
    lv_obj_remove_style_all(status_panel_);
    lv_obj_set_width(status_panel_, screen_w);
    lv_obj_set_height(status_panel_, LV_SIZE_CONTENT);
    lv_obj_add_flag(status_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(status_panel_, 0, 0);

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

    // Invisible settings hotspot in the upper-right corner
    header_overlay_ = lv_btn_create(base_screen_);
    lv_obj_remove_style_all(header_overlay_);
    lv_obj_set_size(header_overlay_, 120, 80);
    lv_obj_set_style_bg_opa(header_overlay_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header_overlay_, 0, 0);
    lv_obj_set_pos(header_overlay_, screen_w - 120, 0);
    lv_obj_add_flag(header_overlay_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(header_overlay_, settingsButtonEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_move_foreground(header_overlay_);

    // Use content_root directly as page container
    page_container_ = content_root_;

    applyHeaderNavSpacing();
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
        // Don't remove all styles - keep base rendering intact for child labels
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        // Use page-specific inactive color, fall back to theme nav_button_color
        lv_color_t inactive_color = !page.nav_inactive_color.empty() ?
            colorFromHex(page.nav_inactive_color, UITheme::COLOR_SURFACE) :
            colorFromHex(config_->theme.nav_button_color, UITheme::COLOR_SURFACE);

        lv_obj_set_style_bg_color(btn, inactive_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        // Use page-specific active color, fall back to theme nav_button_active_color
        lv_color_t active_color = !page.nav_color.empty() ?
            colorFromHex(page.nav_color, UITheme::COLOR_ACCENT) :
            colorFromHex(config_->theme.nav_button_active_color, UITheme::COLOR_ACCENT);
        lv_obj_set_style_bg_color(btn, active_color, LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);

        lv_color_t border_color = colorFromHex(config_->theme.border_color, UITheme::COLOR_BORDER);
        lv_obj_set_style_border_width(btn, config_->theme.border_width, 0);
        lv_obj_set_style_border_color(btn, border_color, 0);
        lv_obj_set_style_border_width(btn, config_->theme.border_width, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, border_color, LV_STATE_CHECKED);
        const bool page_has_nav_radius = page.nav_button_radius >= 0 && page.nav_button_radius <= 50;
        const std::uint8_t nav_radius = page_has_nav_radius
            ? static_cast<std::uint8_t>(page.nav_button_radius)
            : ((config_->theme.nav_button_radius || config_->theme.nav_button_radius == 0)
                ? config_->theme.nav_button_radius
                : (config_->theme.button_radius ? config_->theme.button_radius : 20));
        lv_obj_set_style_radius(btn, nav_radius, 0);
        lv_obj_set_style_radius(btn, nav_radius, LV_STATE_CHECKED);
        lv_obj_set_style_pad_left(btn, UITheme::SPACE_MD, 0);
        lv_obj_set_style_pad_right(btn, UITheme::SPACE_MD, 0);
        lv_obj_set_style_pad_top(btn, UITheme::SPACE_SM, 0);
        lv_obj_set_style_pad_bottom(btn, UITheme::SPACE_SM, 0);
        lv_obj_set_style_min_width(btn, 140, 0);
        lv_obj_set_style_max_width(btn, 320, 0);
        lv_obj_set_height(btn, 46);
        lv_obj_set_style_shadow_width(btn, 12, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
        lv_obj_set_style_shadow_width(btn, 12, LV_STATE_CHECKED);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_STATE_CHECKED);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_STATE_CHECKED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn, navButtonEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(index)));

        // Create label - keep it simple like the working test
        lv_obj_t* label = lv_label_create(btn);
        
        // Get label text
        std::string nav_label_text;
        if (!page.nav_text.empty()) {
            nav_label_text = page.nav_text;
        } else if (!page.name.empty()) {
            nav_label_text = page.name;
        } else if (!page.id.empty()) {
            nav_label_text = page.id;
        } else {
            nav_label_text = "Page " + std::to_string(index + 1);
        }
        lv_label_set_text(label, nav_label_text.c_str());
        
        // Get text color: p.nav_text_color || theme.nav_button_text_color || theme.text_primary || '#f2f4f8'
        lv_color_t nav_text_color = lv_color_hex(0xf2f4f8);  // Default
        if (!page.nav_text_color.empty()) {
            nav_text_color = colorFromHex(page.nav_text_color, nav_text_color);
        } else if (!config_->theme.nav_button_text_color.empty()) {
            nav_text_color = colorFromHex(config_->theme.nav_button_text_color, nav_text_color);
        } else if (!config_->theme.text_primary.empty()) {
            nav_text_color = colorFromHex(config_->theme.text_primary, nav_text_color);
        }
        
        lv_obj_set_style_text_color(label, nav_text_color, 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_center(label);

        nav_buttons_.push_back(btn);
        ++index;
    }

    updateNavSelection();
}

void UIBuilder::buildEmptyState() {
    if (!page_container_) {
        return;
    }

    lv_obj_clean(page_container_);
    lv_obj_remove_style_all(page_container_);
    lv_color_t bg = config_ ? colorFromHex(config_->theme.page_bg_color, UITheme::COLOR_SURFACE) : UITheme::COLOR_SURFACE;
    lv_obj_set_width(page_container_, lv_pct(100));
    lv_obj_set_flex_grow(page_container_, 1);
    lv_obj_set_style_bg_color(page_container_, bg, 0);
    lv_obj_set_style_bg_opa(page_container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(page_container_, 0, 0);
    lv_obj_set_style_pad_all(page_container_, UITheme::SPACE_MD, 0);
    lv_obj_set_style_shadow_width(page_container_, 0, 0);
    lv_obj_clear_flag(page_container_, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(page_container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page_container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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

    if (!page_container_) {
        return;
    }

    active_page_ = index;
    const PageConfig& page = config_->pages[index];

    const std::string page_bg_hex = !page.bg_color.empty()
        ? page.bg_color
        : (config_ ? config_->theme.page_bg_color : "#0F0F0F");

    lv_obj_clean(page_container_);
    lv_obj_remove_style_all(page_container_);
    lv_obj_set_width(page_container_, lv_pct(100));
    lv_obj_set_flex_grow(page_container_, 1);
    lv_obj_set_style_bg_color(page_container_, colorFromHex(page_bg_hex, UITheme::COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(page_container_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(page_container_, 0, 0);
    lv_obj_set_style_pad_all(page_container_, UITheme::SPACE_MD, 0);
    lv_obj_set_style_border_width(page_container_, 0, 0);
    lv_obj_set_style_shadow_width(page_container_, 0, 0);
    lv_obj_clear_flag(page_container_, LV_OBJ_FLAG_SCROLLABLE);

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

    if (page.buttons.empty()) {
        // No grid layout - just center the message
        lv_obj_set_layout(page_container_, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(page_container_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(page_container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* label = lv_label_create(page_container_);
        lv_label_set_text(label, "This page has no buttons yet.");
        lv_obj_set_style_text_font(label, UITheme::FONT_BODY, 0);
        lv_color_t secondary = config_ ? colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY) : UITheme::COLOR_TEXT_SECONDARY;
        lv_obj_set_style_text_color(label, secondary, 0);
        updateNavSelection();
        return;
    }

    // Set up grid layout for buttons
    lv_obj_set_layout(page_container_, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_gap(page_container_, UITheme::SPACE_SM, 0);
    lv_obj_set_grid_dsc_array(page_container_, grid_cols_.data(), grid_rows_.data());

    for (const auto& button : page.buttons) {
        lv_obj_t* btn = lv_btn_create(page_container_);
        lv_obj_remove_style_all(btn);

        // Apply per-button styling; fall back to theme only when button fields are empty
        lv_obj_set_style_radius(btn, button.corner_radius, 0);
        lv_obj_set_style_border_width(btn, button.border_width, 0);
        const lv_color_t border_color = !button.border_color.empty()
            ? colorFromHex(button.border_color, UITheme::COLOR_BORDER)
            : (config_ ? colorFromHex(config_->theme.border_color, UITheme::COLOR_BORDER) : UITheme::COLOR_BORDER);
        lv_obj_set_style_border_color(btn, border_color, 0);
        lv_obj_set_style_border_opa(btn, button.border_width > 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

        const std::string button_color_hex = !button.color.empty()
            ? button.color
            : (config_ ? config_->theme.accent_color : "#FFA500");
        lv_color_t btn_color = colorFromHex(button_color_hex, UITheme::COLOR_ACCENT);
        lv_obj_set_style_bg_color(btn, btn_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        // Pressed state color - use button override or derive from base color
        const std::string pressed_hex = !button.pressed_color.empty() ? button.pressed_color : "";
        lv_color_t pressed_color = pressed_hex.empty()
            ? lv_color_darken(btn_color, LV_OPA_40)
            : colorFromHex(pressed_hex, lv_color_darken(btn_color, LV_OPA_40));
        lv_obj_set_style_bg_color(btn, pressed_color, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

        lv_obj_set_style_pad_all(btn, UITheme::SPACE_MD, 0);
        lv_obj_set_style_min_height(btn, 88, 0);
        lv_obj_set_style_shadow_width(btn, 14, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
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
        // Button text color priority: per-button > page override > theme default
        const lv_color_t theme_text_fallback = config_
            ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY)
            : UITheme::COLOR_TEXT_PRIMARY;
        const lv_color_t page_text_fallback = !page.text_color.empty()
            ? colorFromHex(page.text_color, theme_text_fallback)
            : theme_text_fallback;
        lv_color_t label_color = page_text_fallback;
        if (!button.text_color.empty()) {
            label_color = colorFromHex(button.text_color, page_text_fallback);
        }
        lv_obj_set_style_text_color(title, label_color, 0);

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

        // Apply text alignment - set label to full button width for text alignment to work
        lv_obj_set_width(title, lv_pct(100));
        lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
        
        lv_align_t align = LV_ALIGN_CENTER;
        lv_text_align_t text_align = LV_TEXT_ALIGN_CENTER;
        if (button.text_align == "top-left") { align = LV_ALIGN_TOP_LEFT; text_align = LV_TEXT_ALIGN_LEFT; }
        else if (button.text_align == "top-center") { align = LV_ALIGN_TOP_MID; text_align = LV_TEXT_ALIGN_CENTER; }
        else if (button.text_align == "top-right") { align = LV_ALIGN_TOP_RIGHT; text_align = LV_TEXT_ALIGN_RIGHT; }
        else if (button.text_align == "center") { align = LV_ALIGN_CENTER; text_align = LV_TEXT_ALIGN_CENTER; }
        else if (button.text_align == "bottom-left") { align = LV_ALIGN_BOTTOM_LEFT; text_align = LV_TEXT_ALIGN_LEFT; }
        else if (button.text_align == "bottom-center") { align = LV_ALIGN_BOTTOM_MID; text_align = LV_TEXT_ALIGN_CENTER; }
        else if (button.text_align == "bottom-right") { align = LV_ALIGN_BOTTOM_RIGHT; text_align = LV_TEXT_ALIGN_RIGHT; }
        lv_obj_align(title, align, 0, 0);
        lv_obj_set_style_text_align(title, text_align, 0);
    }

    updateNavSelection();
}

void UIBuilder::updateNavSelection() {
    for (std::size_t i = 0; i < nav_buttons_.size(); ++i) {
        lv_obj_t* btn = nav_buttons_[i];
        if (!btn) {
            continue;
        }
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        if (i == active_page_) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
            if (label) {
                lv_obj_add_state(label, LV_STATE_CHECKED);
            }
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
            if (label) {
                lv_obj_clear_state(label, LV_STATE_CHECKED);
            }
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
    std::string base64_data;
    if (pos == std::string::npos) {
        base64_data = data_uri;
    } else {
        base64_data = data_uri.substr(pos + prefix.length());
    }
    
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

bool UIBuilder::loadImageDescriptor(const std::string& data_uri,
                                    std::vector<uint8_t>& pixel_buffer,
                                    lv_img_dsc_t& descriptor,
                                    bool scrub_white_background) {
    if (data_uri.rfind("lvimg:", 0) != 0) {
        Serial.println("[UI] Unsupported image payload (missing lvimg: prefix)");
        return false;
    }

    const size_t fmt_sep = data_uri.find(':', 6);
    if (fmt_sep == std::string::npos) {
        Serial.println("[UI] Malformed lvimg payload (format separator missing)");
        return false;
    }
    const std::string format = data_uri.substr(6, fmt_sep - 6);

    const size_t size_sep = data_uri.find(':', fmt_sep + 1);
    if (size_sep == std::string::npos) {
        Serial.println("[UI] Malformed lvimg payload (size separator missing)");
        return false;
    }
    const std::string size_part = data_uri.substr(fmt_sep + 1, size_sep - (fmt_sep + 1));
    const size_t x_pos = size_part.find('x');
    if (x_pos == std::string::npos) {
        Serial.println("[UI] Malformed lvimg payload (widthxheight missing)");
        return false;
    }

    const uint16_t width = static_cast<uint16_t>(std::atoi(size_part.substr(0, x_pos).c_str()));
    const uint16_t height = static_cast<uint16_t>(std::atoi(size_part.substr(x_pos + 1).c_str()));
    if (width == 0 || height == 0) {
        Serial.println("[UI] Invalid lvimg dimensions");
        return false;
    }

    const std::string base64_data = data_uri.substr(size_sep + 1);
    pixel_buffer = decodeBase64Logo(base64_data);
    if (pixel_buffer.empty()) {
        Serial.println("[UI] Failed to decode lvimg base64 payload");
        return false;
    }

    descriptor = lv_img_dsc_t{};
    descriptor.header.always_zero = 0;
    descriptor.header.w = width;
    descriptor.header.h = height;

    if (format == "rgb565a") {
        const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
        if (pixel_buffer.size() != expected) {
            Serial.printf("[UI] lvimg buffer mismatch (%u vs %u)\n",
                          static_cast<unsigned>(pixel_buffer.size()),
                          static_cast<unsigned>(expected));
            return false;
        }
        if (scrub_white_background) {
            const uint8_t tolerance = 28;
            const uint8_t threshold = static_cast<uint8_t>(255 - tolerance);
            std::size_t stripped = 0;
            for (std::size_t i = 0; i + 2 < pixel_buffer.size(); i += 3) {
                const uint16_t color = static_cast<uint16_t>(pixel_buffer[i]) |
                                       (static_cast<uint16_t>(pixel_buffer[i + 1]) << 8);
                uint8_t r5 = static_cast<uint8_t>((color >> 11) & 0x1F);
                uint8_t g6 = static_cast<uint8_t>((color >> 5) & 0x3F);
                uint8_t b5 = static_cast<uint8_t>(color & 0x1F);
                uint8_t r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
                uint8_t g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
                uint8_t b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
                uint8_t maxc = std::max({r, g, b});
                uint8_t minc = std::min({r, g, b});
                if (maxc >= threshold && (maxc - minc) <= tolerance && pixel_buffer[i + 2] > 0) {
                    pixel_buffer[i + 2] = 0;
                    ++stripped;
                }
            }
            if (stripped > 0) {
                Serial.printf("[UI] Cleared %u near-white logo pixels to enforce transparency\n", static_cast<unsigned>(stripped));
            }
        }
        descriptor.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    } else if (format == "rgb565") {
        const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 2;
        if (pixel_buffer.size() != expected) {
            Serial.println("[UI] lvimg rgb565 buffer mismatch");
            return false;
        }
        descriptor.header.cf = LV_IMG_CF_TRUE_COLOR;
    } else {
        Serial.printf("[UI] Unsupported lvimg format: %s\n", format.c_str());
        return false;
    }

    descriptor.data_size = pixel_buffer.size();
    descriptor.data = pixel_buffer.data();
    return true;
}

void UIBuilder::applyHeaderNavSpacing() {
    if (!header_brand_row_) {
        return;
    }
    lv_coord_t spacing = UITheme::SPACE_SM;
    if (config_) {
        spacing = std::min<lv_coord_t>(60, std::max<lv_coord_t>(0, config_->header.nav_spacing));
    }

    const uint8_t header_border_width = config_ ? config_->theme.header_border_width : 0;
    if (header_border_width > 0) {
        lv_color_t header_border_color = colorFromHex(config_->theme.header_border_color, UITheme::COLOR_BORDER);
        lv_obj_set_style_border_width(header_brand_row_, header_border_width, 0);
        lv_obj_set_style_border_color(header_brand_row_, header_border_color, 0);
        lv_obj_set_style_border_opa(header_brand_row_, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_border_width(header_brand_row_, 0, 0);
    }

    lv_obj_set_style_pad_bottom(header_brand_row_, spacing, 0);

    nav_base_pad_top_ = spacing > 0
        ? std::max<lv_coord_t>(UITheme::SPACE_XS, spacing / 2)
        : UITheme::SPACE_XS;
    if (nav_bar_) {
        lv_obj_set_style_pad_top(nav_bar_, nav_base_pad_top_, 0);
    }
}

void UIBuilder::applyHeaderLogoSizing(uint16_t src_width,
                                      uint16_t src_height,
                                      bool inline_layout) {
    if (!header_logo_img_ || !header_logo_slot_) {
        return;
    }

    constexpr uint16_t kZoomBase = LV_IMG_ZOOM_NONE;  // 256
    constexpr uint16_t kZoomMin = LV_IMG_ZOOM_NONE / 4;   // 0.25x shrink cap
    constexpr uint16_t kZoomMax = LV_IMG_ZOOM_NONE * 4;   // 4x enlarge cap
    uint16_t configured_height = 64;
    if (config_) {
        const uint16_t raw_height = config_->header.logo_target_height;
        configured_height = std::min<uint16_t>(120, std::max<uint16_t>(24, raw_height));
    }
    const lv_coord_t target_height = configured_height;
    const lv_coord_t width_budget = inline_layout
        ? static_cast<lv_coord_t>(configured_height * 2.5f)
        : static_cast<lv_coord_t>(configured_height * 3.0f);

    uint32_t zoom = kZoomBase;
    if (src_height > 0) {
        const uint32_t scaled = static_cast<uint32_t>(target_height) * kZoomBase;
        zoom = scaled / src_height;
        if (zoom == 0) {
            zoom = kZoomMin;
        }
        if (src_width > 0) {
            const uint32_t width_scaled = static_cast<uint32_t>(width_budget) * kZoomBase;
            uint32_t width_zoom = width_scaled / src_width;
            if (width_zoom == 0) {
                width_zoom = kZoomMin;
            }
            zoom = std::min<uint32_t>(zoom, width_zoom);
        }
    }

    zoom = std::max<uint32_t>(kZoomMin, std::min<uint32_t>(kZoomMax, zoom));
    lv_img_set_zoom(header_logo_img_, static_cast<uint16_t>(zoom));

    lv_coord_t display_width = target_height;
    if (src_width > 0) {
        display_width = static_cast<lv_coord_t>((static_cast<uint64_t>(src_width) * zoom + (kZoomBase / 2)) / kZoomBase);
    }
    display_width = std::max<lv_coord_t>(target_height, display_width);
    display_width = std::min<lv_coord_t>(width_budget, display_width);

    lv_obj_set_size(header_logo_slot_, display_width, target_height);
    lv_obj_set_style_min_width(header_logo_slot_, display_width, 0);
    lv_obj_set_style_min_height(header_logo_slot_, target_height, 0);
    lv_obj_set_style_max_width(header_logo_slot_, width_budget, 0);
    lv_obj_set_style_max_height(header_logo_slot_, target_height, 0);
    lv_obj_set_style_pad_all(header_logo_slot_, 0, 0);
    lv_obj_set_style_pad_gap(header_logo_slot_, 0, 0);
    lv_obj_set_style_align(header_logo_img_, LV_ALIGN_CENTER, 0);
}

void UIBuilder::updateHeaderBranding() {
    if (!config_ || !header_title_label_ || !header_bar_) {
        return;
    }

    applyHeaderNavSpacing();

    lv_color_t header_bg = colorFromHex(config_->theme.surface_color, UITheme::COLOR_SURFACE);
    lv_obj_set_style_bg_color(header_bar_, header_bg, 0);
    lv_obj_set_style_bg_opa(header_bar_, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(header_bar_, 0, 0);
    lv_obj_set_style_border_opa(header_bar_, LV_OPA_TRANSP, 0);

    if (nav_bar_) {
        lv_obj_set_style_bg_opa(nav_bar_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(nav_bar_, 0, 0);
        lv_obj_set_style_border_opa(nav_bar_, LV_OPA_TRANSP, 0);
    }

    const auto hideLogoArea = [this]() {
        if (header_logo_img_) {
            lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
        }
        if (header_logo_slot_) {
            lv_obj_set_size(header_logo_slot_, 0, 0);
            lv_obj_add_flag(header_logo_slot_, LV_OBJ_FLAG_HIDDEN);
        }
    };

    const auto showLogoArea = [this]() {
        if (header_logo_slot_) {
            lv_obj_clear_flag(header_logo_slot_, LV_OBJ_FLAG_HIDDEN);
        }
        if (header_logo_img_) {
            lv_obj_clear_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
        }
    };

    // Update title text and font
    lv_label_set_text(header_title_label_, config_->header.title.c_str());
    const lv_font_t* title_font = fontFromName(config_->header.title_font);
    lv_obj_set_style_text_font(header_title_label_, title_font, 0);
    lv_color_t title_color = colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY);
    lv_obj_set_style_text_color(header_title_label_, title_color, 0);
    
    Serial.printf("[UI] Title alignment from config: '%s' (len=%d)\n", 
                  config_->header.title_align.c_str(), 
                  config_->header.title_align.length());
    Serial.printf("[UI] Comparing: left='%s', center='%s', right='%s'\n",
                  config_->header.title_align == "left" ? "MATCH" : "NO",
                  config_->header.title_align == "center" ? "MATCH" : "NO",
                  config_->header.title_align == "right" ? "MATCH" : "NO");
    
    // Apply text alignment and flex positioning
    lv_text_align_t text_align = LV_TEXT_ALIGN_CENTER;
    lv_flex_align_t cross_align = LV_FLEX_ALIGN_CENTER;
    
    if (config_->header.title_align == "left") {
        text_align = LV_TEXT_ALIGN_LEFT;
        cross_align = LV_FLEX_ALIGN_START;
        Serial.println("[UI] Setting text alignment to LEFT");
    } else if (config_->header.title_align == "right") {
        text_align = LV_TEXT_ALIGN_RIGHT;
        cross_align = LV_FLEX_ALIGN_END;
        Serial.println("[UI] Setting text alignment to RIGHT");
    } else {
        text_align = LV_TEXT_ALIGN_CENTER;
        cross_align = LV_FLEX_ALIGN_CENTER;
        Serial.println("[UI] Setting text alignment to CENTER");
    }
    
    // Update header flex alignment to match text alignment
    lv_obj_set_flex_align(header_bar_, LV_FLEX_ALIGN_START, cross_align, LV_FLEX_ALIGN_START);
    
    lv_obj_set_style_text_align(header_title_label_, text_align, 0);

    // Update subtitle text and font
    if (header_subtitle_label_) {
        if (config_->header.subtitle.empty()) {
            lv_obj_add_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(header_subtitle_label_, config_->header.subtitle.c_str());
            const lv_font_t* subtitle_font = fontFromName(config_->header.subtitle_font);
            lv_obj_set_style_text_font(header_subtitle_label_, subtitle_font, 0);
            lv_color_t subtitle_color = colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY);
            lv_obj_set_style_text_color(header_subtitle_label_, subtitle_color, 0);
            lv_obj_set_style_text_align(header_subtitle_label_, text_align, 0);
            
            lv_obj_clear_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (!header_logo_img_ || !header_brand_row_) {
        return;
    }

    header_logo_ready_ = false;
    lv_img_set_zoom(header_logo_img_, LV_IMG_ZOOM_NONE);

    Serial.printf("[UI] Logo config - show_logo: %d, logo_variant: '%s'\n", 
                  config_->header.show_logo, 
                  config_->header.logo_variant.c_str());
    
    const std::string logo_position = config_->header.logo_position.empty() ? "stacked" : config_->header.logo_position;
    const bool inline_layout = (logo_position == "inline-left" || logo_position == "inline-right");

    if (inline_layout) {
        lv_obj_set_flex_flow(header_brand_row_, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(header_brand_row_, UITheme::SPACE_SM, 0);
        lv_obj_set_flex_align(header_brand_row_, LV_FLEX_ALIGN_START, cross_align, LV_FLEX_ALIGN_CENTER);
        if (logo_position == "inline-left") {
            if (header_logo_slot_) lv_obj_move_background(header_logo_slot_);
            if (header_text_container_) lv_obj_move_foreground(header_text_container_);
        } else {
            if (header_text_container_) lv_obj_move_background(header_text_container_);
            if (header_logo_slot_) lv_obj_move_foreground(header_logo_slot_);
        }
    } else {
        lv_obj_set_flex_flow(header_brand_row_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_gap(header_brand_row_, UITheme::SPACE_XS, 0);
        lv_obj_set_flex_align(header_brand_row_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        if (header_logo_slot_) lv_obj_move_background(header_logo_slot_);
        if (header_text_container_) lv_obj_move_foreground(header_text_container_);
    }

    if (config_->header.show_logo) {
        // Priority 1: Custom uploaded header logo
        if (!config_->images.header_logo.empty()) {
            Serial.printf("[UI] Custom header logo found, length=%d\n", config_->images.header_logo.length());
            Serial.printf("[UI] Data URL prefix: %.50s...\n", config_->images.header_logo.c_str());
            
            if (loadImageDescriptor(config_->images.header_logo, logo_buffer_, header_logo_dsc_, true)) {
                header_logo_ready_ = true;
                lv_img_set_src(header_logo_img_, &header_logo_dsc_);
                showLogoArea();
                lv_obj_invalidate(header_logo_img_);

                lv_img_header_t header;
                lv_res_t res = lv_img_decoder_get_info(&header_logo_dsc_, &header);
                if (res == LV_RES_OK) {
                    Serial.printf("[UI] Image decoded: %dx%d, format=%d\n", header.w, header.h, header.cf);
                    applyHeaderLogoSizing(header.w, header.h, inline_layout);
                } else {
                    Serial.printf("[UI] WARN: Decoder couldn't preflight custom logo, res=%d\n", res);
                    applyHeaderLogoSizing(0, 0, inline_layout);
                }
            } else {
                Serial.println("[UI] ERROR: Failed to prepare custom header logo");
                hideLogoArea();
            }
            return;
        }
        
        // Priority 2: Legacy custom logo from header.logo_base64
        if (!config_->header.logo_base64.empty()) {
            Serial.println("[UI] Using legacy custom header logo");
            lv_img_set_src(header_logo_img_, config_->header.logo_base64.c_str());
            showLogoArea();
            lv_obj_invalidate(header_logo_img_);

            lv_img_header_t header;
            if (lv_img_decoder_get_info(config_->header.logo_base64.c_str(), &header) == LV_RES_OK) {
                applyHeaderLogoSizing(header.w, header.h, inline_layout);
            } else {
                applyHeaderLogoSizing(0, 0, inline_layout);
            }
            return;
        }
        
        // Priority 3: Built-in logo from logo_variant
        if (const lv_img_dsc_t* logo = iconForId(config_->header.logo_variant)) {
            Serial.printf("[UI] Using built-in logo variant: %s\n", config_->header.logo_variant.c_str());
            lv_img_set_src(header_logo_img_, logo);
            showLogoArea();
            lv_obj_invalidate(header_logo_img_);
            applyHeaderLogoSizing(logo->header.w, logo->header.h, inline_layout);
        } else {
            Serial.printf("[UI] No logo found for variant: %s\n", config_->header.logo_variant.c_str());
            hideLogoArea();
        }
    } else {
        Serial.println("[UI] Logo disabled in config");
        hideLogoArea();
    }
}

const lv_img_dsc_t* UIBuilder::iconForId(const std::string& id) const {
    // No built-in logos - all logos must be custom uploaded
    // This function kept for future expansion if built-in logos are added
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
    lv_obj_set_size(info_modal_, 520, 420);
    lv_obj_center(info_modal_);
    lv_obj_set_style_bg_color(info_modal_, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(info_modal_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(info_modal_, 3, 0);
    lv_obj_set_style_border_color(info_modal_, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_border_opa(info_modal_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(info_modal_, 16, 0);
    lv_obj_set_style_pad_all(info_modal_, UITheme::SPACE_MD, 0);
    lv_obj_set_flex_flow(info_modal_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_modal_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(info_modal_, UITheme::SPACE_SM, 0);
    lv_obj_add_flag(info_modal_, LV_OBJ_FLAG_CLICKABLE);  // Block clicks from reaching background
    lv_obj_clear_flag(info_modal_, LV_OBJ_FLAG_SCROLLABLE);  // Modal content shouldn't close on click

    // Title
    lv_obj_t* title = lv_label_create(info_modal_);
    lv_label_set_text(title, "Device Info");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(title, LV_OPA_COVER, 0);

    // Compact stat cards keep layout stable
    lv_obj_t* stats_row = lv_obj_create(info_modal_);
    lv_obj_remove_style_all(stats_row);
    lv_obj_set_width(stats_row, 500);
    lv_obj_set_flex_flow(stats_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(stats_row, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_all(stats_row, 0, 0);

    auto createStatCard = [&](const char* label_text, const char* default_value, lv_obj_t** value_slot) {
        lv_obj_t* card = lv_obj_create(stats_row);
        lv_obj_remove_style_all(card);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1c1c1c), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, UITheme::RADIUS_MD, 0);
        lv_obj_set_style_pad_all(card, UITheme::SPACE_SM, 0);
        lv_obj_set_style_min_width(card, 150, 0);
        lv_obj_set_style_max_width(card, 240, 0);
        lv_obj_set_style_min_height(card, 74, 0);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_grow(card, 1);

        lv_obj_t* heading = lv_label_create(card);
        lv_obj_set_style_text_font(heading, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(heading, UITheme::COLOR_TEXT_SECONDARY, 0);
        lv_label_set_text(heading, label_text);

        lv_obj_t* value = lv_label_create(card);
        lv_obj_set_style_text_font(value, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(value, UITheme::COLOR_TEXT_PRIMARY, 0);
        lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(value, lv_pct(100));
        lv_label_set_text(value, default_value);
        if (value_slot) {
            *value_slot = value;
        }
    };

    createStatCard("Connectivity", "Checking...", &network_status_label_);
    createStatCard("IP Address", "Not connected", &info_ip_label_);
    const char* version_default = (APP_VERSION && APP_VERSION[0]) ? APP_VERSION : "--";
    createStatCard("Firmware", version_default, &version_label_);
    refreshVersionLabel();

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

    // OTA controls
    lv_obj_t* ota_section = lv_obj_create(info_modal_);
    lv_obj_remove_style_all(ota_section);
    lv_obj_set_flex_flow(ota_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ota_section, 0, 0);
    lv_obj_set_style_pad_gap(ota_section, UITheme::SPACE_XS, 0);
    lv_obj_set_width(ota_section, 500);

    lv_obj_t* ota_header = lv_obj_create(ota_section);
    lv_obj_remove_style_all(ota_header);
    lv_obj_set_width(ota_header, lv_pct(100));
    lv_obj_set_flex_flow(ota_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ota_header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ota_header, 0, 0);
    lv_obj_set_style_pad_gap(ota_header, UITheme::SPACE_SM, 0);

    lv_obj_t* ota_heading = lv_label_create(ota_header);
    lv_label_set_text(ota_heading, "Software Updates");
    lv_obj_set_style_text_font(ota_heading, UITheme::FONT_BODY, 0);
    lv_obj_set_style_text_color(ota_heading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_flex_grow(ota_heading, 1, 0);

    ota_update_btn_ = lv_btn_create(ota_header);
    lv_obj_set_size(ota_update_btn_, 160, 40);
    lv_obj_set_style_bg_color(ota_update_btn_, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_radius(ota_update_btn_, UITheme::RADIUS_MD, 0);
    lv_obj_add_event_cb(ota_update_btn_, otaCheckButtonEvent, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* ota_btn_label = lv_label_create(ota_update_btn_);
    lv_label_set_text(ota_btn_label, "Check Now");
    lv_obj_set_style_text_font(ota_btn_label, UITheme::FONT_BODY, 0);
    lv_obj_center(ota_btn_label);

    ota_status_label_ = lv_label_create(ota_section);
    lv_label_set_long_mode(ota_status_label_, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(ota_status_label_, lv_pct(100));
    lv_obj_set_style_text_font(ota_status_label_, UITheme::FONT_BODY, 0);
    lv_obj_set_style_text_color(ota_status_label_, lv_color_hex(0xFFFFFF), 0);

    ota_status_text_ = OTAUpdateManager::instance().lastStatus();
    refreshOtaStatusLabel();

    // Close button
    lv_obj_t* close_row = lv_obj_create(info_modal_);
    lv_obj_remove_style_all(close_row);
    lv_obj_set_width(close_row, 500);
    lv_obj_set_flex_flow(close_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(close_row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(close_row, 0, 0);

    lv_obj_t* close_btn = lv_btn_create(close_row);
    lv_obj_set_size(close_btn, 120, 40);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_radius(close_btn, UITheme::RADIUS_MD, 0);
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

    updateOtaStatus(OTAUpdateManager::instance().lastStatus());
    refreshNetworkStatusLabel();
    refreshVersionLabel();

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

void UIBuilder::updateOtaStatus(const std::string& status) {
    ota_status_text_ = status.empty() ? "idle" : status;
    refreshOtaStatusLabel();
}

void UIBuilder::refreshOtaStatusLabel() {
    if (!ota_status_label_) {
        return;
    }
    const std::string friendly = humanizeOtaStatus(ota_status_text_);
    lv_label_set_text(ota_status_label_, friendly.c_str());
    lv_obj_set_style_text_color(ota_status_label_, colorForOtaStatus(ota_status_text_), 0);
}

std::string UIBuilder::humanizeOtaStatus(const std::string& status) const {
    if (status.empty()) {
        return "Idle";
    }

    auto startsWith = [](const std::string& value, const char* prefix) {
        return value.rfind(prefix, 0) == 0;
    };

    if (status == "disabled") return "OTA disabled";
    if (status == "waiting-for-wifi") return "Waiting for Wi-Fi";
    if (status == "wifi-ready") return "Wi-Fi connected";
    if (status == "manual-check-requested") return "Checking for updates...";
    if (status == "up-to-date") return "You're up to date";
    if (status == "manifest-url-empty" || status == "missing-manifest-url") return "Manifest URL missing";
    if (status == "manifest-channel-mismatch") return "No update available on this channel";
    if (startsWith(status, "update-available-")) return std::string("Update available: ") + status.substr(18);
    if (startsWith(status, "updated-to-")) return std::string("Updated to ") + status.substr(11);
    if (startsWith(status, "downloading-")) return std::string("Downloading ") + status.substr(12);
    if (startsWith(status, "manifest-http-")) return std::string("Manifest download failed (HTTP ") + status.substr(14) + ")";
    if (startsWith(status, "firmware-http-")) return std::string("Firmware download failed (HTTP ") + status.substr(14) + ")";
    if (startsWith(status, "manifest-parse-")) return std::string("Manifest parse error: ") + status.substr(15);
    if (status == "manifest-begin-failed") return "Unable to reach manifest";
    if (status == "firmware-begin-failed") return "Unable to reach firmware file";
    if (status == "update-begin-failed") return "Updater failed to start";
    if (status == "update-end-failed") return "Updater failed to finish";
    if (status == "md5-invalid") return "Firmware checksum mismatch";
    if (status == "firmware-empty") return "Firmware payload empty";
    if (status == "manifest-missing-fields") return "Manifest missing firmware info";

    std::string friendly = status;
    std::replace(friendly.begin(), friendly.end(), '-', ' ');
    if (!friendly.empty()) {
        friendly[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(friendly[0])));
    }
    return friendly;
}

lv_color_t UIBuilder::colorForOtaStatus(const std::string& status) const {
    auto startsWith = [](const std::string& value, const char* prefix) {
        return value.rfind(prefix, 0) == 0;
    };

    if (status == "up-to-date" || startsWith(status, "updated-to-")) {
        return UITheme::COLOR_SUCCESS;
    }
    if (status == "manual-check-requested" || startsWith(status, "downloading-") ||
        startsWith(status, "update-available-") || status == "wifi-ready") {
        return UITheme::COLOR_ACCENT;
    }
    if (status == "waiting-for-wifi" || status == "disabled") {
        return UITheme::COLOR_TEXT_SECONDARY;
    }
    if (status == "manifest-url-empty" || status == "missing-manifest-url" ||
        status == "manifest-begin-failed" || status == "firmware-begin-failed" ||
        status == "update-begin-failed" || status == "update-end-failed" ||
        status == "md5-invalid" || status == "firmware-empty" ||
        status == "manifest-missing-fields" || startsWith(status, "manifest-http-") ||
        startsWith(status, "firmware-http-") || startsWith(status, "manifest-parse-")) {
        return UITheme::COLOR_ERROR;
    }
    return UITheme::COLOR_TEXT_PRIMARY;
}

void UIBuilder::refreshNetworkStatusLabel() {
    if (info_ip_label_) {
        const char* ip_value = "Not connected";
        if (last_sta_connected_ && !last_sta_ip_.empty() && last_sta_ip_ != "0.0.0.0") {
            ip_value = last_sta_ip_.c_str();
        } else if (!last_ap_ip_.empty() && last_ap_ip_ != "0.0.0.0") {
            ip_value = last_ap_ip_.c_str();
        }
        lv_label_set_text(info_ip_label_, ip_value);
    }

    if (network_status_label_) {
        const std::string status_text = connectionStatusText();
        lv_label_set_text(network_status_label_, status_text.c_str());
        lv_obj_set_style_text_color(network_status_label_, connectionStatusColor(), 0);
    }
}

std::string UIBuilder::connectionStatusText() const {
    if (last_sta_connected_ && !last_sta_ip_.empty() && last_sta_ip_ != "0.0.0.0") {
        return "Wi-Fi Online";
    }
    if (!last_ap_ip_.empty() && last_ap_ip_ != "0.0.0.0") {
        return "Hosting AP";
    }
    return "Offline";
}

lv_color_t UIBuilder::connectionStatusColor() const {
    if (last_sta_connected_) {
        return UITheme::COLOR_SUCCESS;
    }
    if (!last_ap_ip_.empty() && last_ap_ip_ != "0.0.0.0") {
        return UITheme::COLOR_ACCENT;
    }
    return UITheme::COLOR_ERROR;
}

void UIBuilder::refreshVersionLabel() {
    if (!version_label_) {
        return;
    }
    const char* version_text = (APP_VERSION && APP_VERSION[0]) ? APP_VERSION : "--";
    lv_label_set_text(version_label_, version_text);
}

void UIBuilder::settingsButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    Serial.println("[UI] Settings button clicked - showing settings modal");
    UIBuilder::instance().showInfoModal();
}

void UIBuilder::otaCheckButtonEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    Serial.println("[UI] Manual OTA check requested");
    OTAUpdateManager::instance().triggerImmediateCheck();
    UIBuilder::instance().updateOtaStatus("manual-check-requested");
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
    sleep_logo_ready_ = false;

    if (!config_) {
        return;
    }

    std::string data_url;

    // Priority: new images.sleep_logo > legacy display.sleep_icon_base64
    if (!config_->images.sleep_logo.empty()) {
        data_url = config_->images.sleep_logo;
    } else if (!config_->display.sleep_icon_base64.empty()) {
        data_url = config_->display.sleep_icon_base64;
    }

    if (!data_url.empty()) {
        if (loadImageDescriptor(data_url, sleep_icon_buffer_, sleep_logo_dsc_)) {
            sleep_logo_ready_ = true;
        } else {
            Serial.println("[UI] Failed to decode sleep icon");
        }
    }
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
    if (sleep_logo_ready_) {
        Serial.printf("[UI] Showing sleep icon (%u bytes)\n", static_cast<unsigned>(sleep_icon_buffer_.size()));
        lv_img_set_src(sleep_image_, &sleep_logo_dsc_);
        lv_obj_clear_flag(sleep_image_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_center(sleep_image_);
        
        // Check decode result
        lv_img_header_t header;
        lv_res_t res = lv_img_decoder_get_info(&sleep_logo_dsc_, &header);
        if (res == LV_RES_OK) {
            Serial.printf("[UI] Sleep icon decoded: %dx%d\n", header.w, header.h);
        } else {
            Serial.printf("[UI] ERROR: Sleep icon decode failed, res=%d\n", res);
        }
    } else {
        // No image configured - hide the image object
        Serial.println("[UI] No sleep icon configured");
        lv_obj_add_flag(sleep_image_, LV_OBJ_FLAG_HIDDEN);
    }
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
    if (name == "montserrat_34") return &lv_font_montserrat_34;
    if (name == "montserrat_36") return &lv_font_montserrat_36;
    if (name == "montserrat_38") return &lv_font_montserrat_38;
    if (name == "montserrat_40") return &lv_font_montserrat_40;
    if (name == "montserrat_42") return &lv_font_montserrat_42;
    if (name == "montserrat_44") return &lv_font_montserrat_44;
    if (name == "montserrat_46") return &lv_font_montserrat_46;
    if (name == "montserrat_48") return &lv_font_montserrat_48;
    
    // Special fonts
    if (name == "dejavu_16") return &lv_font_dejavu_16_persian_hebrew;
    if (name == "simsun_16") return &lv_font_simsun_16_cjk;
    
    // UNSCII monospace fonts
    if (name == "unscii_8") return &lv_font_unscii_8;
    if (name == "unscii_16") return &lv_font_unscii_16;
    
    // Default fallback
    return &lv_font_montserrat_16;
}

const lv_font_t* UIBuilder::navLabelFontForText(const std::string& text) const {
    bool has_cjk = false;
    bool has_rtl = false;
    bool has_extended = false;

    std::size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp = nextUtf8Codepoint(text, idx);
        if (cp == 0) {
            continue;
        }

        // Common CJK ranges (Han, Kana, Hangul, full-width)
        if ((cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0x3040 && cp <= 0x30FF) ||
            (cp >= 0x31F0 && cp <= 0x31FF) ||
            (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xFF01 && cp <= 0xFF60) ||
            (cp >= 0xFFE0 && cp <= 0xFFE6)) {
            has_cjk = true;
            break;
        }

        // Hebrew, Arabic, Persian, Urdu ranges (including presentation forms)
        if ((cp >= 0x0590 && cp <= 0x08FF) ||
            (cp >= 0xFB50 && cp <= 0xFDFF) ||
            (cp >= 0xFE70 && cp <= 0xFEFF)) {
            has_rtl = true;
        } else if (cp > 0x7F) {
            has_extended = true;
        }
    }

    if (has_cjk) {
        return &lv_font_simsun_16_cjk;
    }
    if (has_rtl || has_extended) {
        return &lv_font_dejavu_16_persian_hebrew;
    }
    return UITheme::FONT_BODY;
}

uint32_t UIBuilder::nextUtf8Codepoint(const std::string& text, std::size_t& index) const {
    if (index >= text.size()) {
        return 0;
    }

    auto readContinuation = [&](std::size_t pos) -> int {
        if (pos >= text.size()) {
            return -1;
        }
        const uint8_t byte = static_cast<uint8_t>(text[pos]);
        if ((byte & 0xC0) != 0x80) {
            return -1;
        }
        return byte & 0x3F;
    };

    const uint8_t first = static_cast<uint8_t>(text[index]);
    if ((first & 0x80) == 0) {
        ++index;
        return first;
    }

    if ((first & 0xE0) == 0xC0) {
        int b1 = readContinuation(index + 1);
        if (b1 < 0) {
            ++index;
            return 0;
        }
        uint32_t cp = ((first & 0x1F) << 6) | static_cast<uint32_t>(b1);
        index += 2;
        return cp;
    }

    if ((first & 0xF0) == 0xE0) {
        int b1 = readContinuation(index + 1);
        int b2 = readContinuation(index + 2);
        if (b1 < 0 || b2 < 0) {
            ++index;
            return 0;
        }
        uint32_t cp = ((first & 0x0F) << 12) |
                       (static_cast<uint32_t>(b1) << 6) |
                       static_cast<uint32_t>(b2);
        index += 3;
        return cp;
    }

    if ((first & 0xF8) == 0xF0) {
        int b1 = readContinuation(index + 1);
        int b2 = readContinuation(index + 2);
        int b3 = readContinuation(index + 3);
        if (b1 < 0 || b2 < 0 || b3 < 0) {
            ++index;
            return 0;
        }
        uint32_t cp = ((first & 0x07) << 18) |
                       (static_cast<uint32_t>(b1) << 12) |
                       (static_cast<uint32_t>(b2) << 6) |
                       static_cast<uint32_t>(b3);
        index += 4;
        return cp;
    }

    // Invalid leading byte, skip it
    ++index;
    return 0;
}
