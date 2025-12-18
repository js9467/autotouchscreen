#include "ui_builder.h"

#include <Arduino.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "assets/images.h"
#include "can_manager.h"
#include "ui_theme.h"
#include "icon_library.h"

namespace {
struct IconEntry {
    const char* id;
    const lv_img_dsc_t* img;
};

constexpr IconEntry kIconEntries[] = {
    {"bronco", &img_bronco_logo},
};
}

UIBuilder& UIBuilder::instance() {
    static UIBuilder builder;
    return builder;
}

void UIBuilder::begin() {
    if (base_screen_) {
        return;
    }
    createBaseScreen();
    lv_scr_load(base_screen_);
}

void UIBuilder::applyConfig(const DeviceConfig& config) {
    config_ = &config;
    if (!base_screen_) {
        begin();
    }

    // Apply theme colors to base screen, header, and text
    if (base_screen_) {
        lv_color_t bg = colorFromHex(config_->theme.bg_color, UITheme::COLOR_BG);
        lv_obj_set_style_bg_color(base_screen_, bg, 0);
    }
    if (header_bar_) {
        lv_color_t surface = colorFromHex(config_->theme.surface_color, UITheme::COLOR_SURFACE);
        lv_obj_set_style_bg_color(header_bar_, surface, 0);
        uint8_t header_border_width = config_->theme.header_border_width;
        lv_obj_set_style_border_width(header_bar_, header_border_width, 0);
        if (header_border_width > 0) {
            lv_color_t header_border_color = colorFromHex(config_->theme.header_border_color, UITheme::COLOR_ACCENT);
            lv_obj_set_style_border_color(header_bar_, header_border_color, 0);
        }
    }
    if (page_container_) {
        lv_color_t page_bg = colorFromHex(config_->theme.page_bg_color, UITheme::COLOR_BG);
        lv_obj_set_style_bg_color(page_container_, page_bg, 0);
    }
    if (header_title_label_) {
        lv_color_t text_primary = colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_set_style_text_color(header_title_label_, text_primary, 0);
    }
    if (header_subtitle_label_) {
        lv_color_t text_secondary = colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY);
        lv_obj_set_style_text_color(header_subtitle_label_, text_secondary, 0);
    }
    if (status_ap_label_) {
        lv_color_t text_primary = colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_set_style_text_color(status_ap_label_, text_primary, 0);
    }
    if (status_sta_label_) {
        lv_color_t text_primary = colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_set_style_text_color(status_sta_label_, text_primary, 0);
    }

    if (!config_->pages.empty() && active_page_ >= config_->pages.size()) {
        active_page_ = 0;
    }

    buildNavigation();
    updateHeaderBranding();

    if (config_->pages.empty()) {
        buildEmptyState();
    } else {
        buildPage(active_page_);
    }

    dirty_ = false;
}

void UIBuilder::markDirty() {
    dirty_ = true;
}

bool UIBuilder::consumeDirtyFlag() {
    if (dirty_) {
        dirty_ = false;
        return true;
    }
    return false;
}

void UIBuilder::createBaseScreen() {
    base_screen_ = lv_obj_create(nullptr);
    
    // Apply theme background color from config
    lv_color_t bg = config_ ? colorFromHex(config_->theme.bg_color, UITheme::COLOR_BG) : UITheme::COLOR_BG;
    lv_obj_set_style_bg_color(base_screen_, bg, 0);
    lv_obj_set_style_bg_opa(base_screen_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(base_screen_, 0, 0);
    lv_obj_set_style_pad_all(base_screen_, 0, 0);

    // Create header bar without background image
    header_bar_ = lv_obj_create(base_screen_);
    lv_obj_set_size(header_bar_, 800, 70);
    lv_obj_set_pos(header_bar_, 0, 0);
    
    // Apply theme surface color if available
    lv_color_t surface_color = config_ ? colorFromHex(config_->theme.surface_color, UITheme::COLOR_SURFACE) : UITheme::COLOR_SURFACE;
    lv_obj_set_style_bg_color(header_bar_, surface_color, 0);
    lv_obj_set_style_bg_opa(header_bar_, LV_OPA_COVER, 0);
    uint8_t header_border_width = config_ ? config_->theme.header_border_width : 0;
    lv_obj_set_style_border_width(header_bar_, header_border_width, 0);
    if (header_border_width > 0) {
        lv_color_t header_border_color = config_ ? colorFromHex(config_->theme.header_border_color, UITheme::COLOR_ACCENT) : UITheme::COLOR_ACCENT;
        lv_obj_set_style_border_color(header_bar_, header_border_color, 0);
        lv_obj_set_style_border_side(header_bar_, LV_BORDER_SIDE_BOTTOM, 0);
    }
    lv_obj_set_style_radius(header_bar_, 0, 0);
    lv_obj_set_style_pad_all(header_bar_, 0, 0);

    header_overlay_ = lv_obj_create(header_bar_);
    lv_obj_remove_style_all(header_overlay_);
    lv_obj_set_size(header_overlay_, 800, 70);
    lv_obj_set_style_bg_opa(header_overlay_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header_overlay_, 0, 0);
    lv_obj_set_style_pad_all(header_overlay_, UITheme::SPACE_MD, 0);
    lv_obj_set_style_pad_gap(header_overlay_, UITheme::SPACE_MD, 0);
    lv_obj_set_flex_flow(header_overlay_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_overlay_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    header_logo_img_ = lv_img_create(header_overlay_);
    lv_obj_set_style_img_opa(header_logo_img_, LV_OPA_COVER, 0);

    lv_obj_t* header_text_column = lv_obj_create(header_overlay_);
    lv_obj_remove_style_all(header_text_column);
    lv_obj_set_flex_flow(header_text_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header_text_column, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(header_text_column, UITheme::SPACE_XS, 0);
    lv_obj_set_width(header_text_column, 600);

    header_title_label_ = lv_label_create(header_text_column);
    lv_obj_set_style_text_font(header_title_label_, UITheme::FONT_HEADING, 0);
    lv_obj_set_style_text_color(header_title_label_, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
    lv_label_set_text(header_title_label_, "Bronco Controls");
    lv_label_set_long_mode(header_title_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(header_title_label_, 600);

    header_subtitle_label_ = lv_label_create(header_text_column);
    lv_obj_set_style_text_font(header_subtitle_label_, UITheme::FONT_CAPTION, 0);
    lv_obj_set_style_text_color(header_subtitle_label_, config_ ? colorFromHex(config_->theme.text_secondary, UITheme::COLOR_TEXT_SECONDARY) : UITheme::COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(header_subtitle_label_, "Web Configurator");
    lv_label_set_long_mode(header_subtitle_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_width(header_subtitle_label_, 600);

    content_root_ = lv_obj_create(base_screen_);
    lv_obj_set_size(content_root_, 800, 410);
    lv_obj_set_pos(content_root_, 0, 70);
    lv_obj_set_style_bg_opa(content_root_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_root_, 0, 0);
    lv_obj_set_style_pad_all(content_root_, UITheme::SPACE_MD, 0);
    lv_obj_set_flex_flow(content_root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(content_root_, LV_OBJ_FLAG_SCROLLABLE);

    nav_bar_ = lv_obj_create(content_root_);
    lv_obj_set_size(nav_bar_, 760, 60);
    lv_obj_set_style_bg_opa(nav_bar_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_bar_, 0, 0);
    lv_obj_set_style_pad_gap(nav_bar_, UITheme::SPACE_SM, 0);
    lv_obj_set_style_pad_all(nav_bar_, 0, 0);
    lv_obj_set_flex_flow(nav_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    status_panel_ = lv_obj_create(content_root_);
    lv_obj_set_size(status_panel_, 760, 56);
    lv_obj_set_style_bg_opa(status_panel_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_panel_, 0, 0);
    lv_obj_set_style_pad_all(status_panel_, 0, 0);
    lv_obj_set_style_pad_gap(status_panel_, UITheme::SPACE_SM, 0);
    lv_obj_set_flex_flow(status_panel_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_panel_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    auto create_chip = [](lv_obj_t* parent, lv_color_t bg_color) {
        lv_obj_t* chip = lv_obj_create(parent);
        lv_obj_set_style_bg_color(chip, bg_color, 0);
        lv_obj_set_style_bg_opa(chip, LV_OPA_50, 0);
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

    page_container_ = lv_obj_create(content_root_);
    lv_obj_set_size(page_container_, 760, 320);
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
    lv_obj_set_style_pad_gap(page_container_, UITheme::SPACE_SM, 0);
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
        
        // Apply theme radius and border from config
        uint8_t radius = config_ ? config_->theme.button_radius : UITheme::RADIUS_LG;
        uint8_t border_width = config_ ? config_->theme.border_width : 0;
        lv_color_t border_color = config_ ? colorFromHex(config_->theme.border_color, UITheme::COLOR_BORDER) : UITheme::COLOR_BORDER;
        lv_obj_set_style_radius(btn, radius, 0);
        lv_obj_set_style_border_width(btn, border_width, 0);
        lv_obj_set_style_border_color(btn, border_color, 0);
        lv_obj_set_style_border_opa(btn, border_width > 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        
        // Normal state color
        lv_color_t btn_color = colorFromHex(button.color, UITheme::COLOR_ACCENT);
        lv_obj_set_style_bg_color(btn, btn_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        
        // Pressed state color - use custom if set, otherwise darken
        lv_color_t pressed_color = button.pressed_color.empty() ? 
            lv_color_darken(btn_color, LV_OPA_40) : 
            colorFromHex(button.pressed_color, lv_color_darken(btn_color, LV_OPA_40));
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
        lv_obj_set_style_text_color(title, config_ ? colorFromHex(config_->theme.text_primary, UITheme::COLOR_TEXT_PRIMARY) : UITheme::COLOR_TEXT_PRIMARY, 0);
        
        // Select closest available font size
        const lv_font_t* font = &lv_font_montserrat_16;
        if (button.font_size <= 13) {
            font = &lv_font_montserrat_12;
        } else if (button.font_size <= 15) {
            font = &lv_font_montserrat_14;
        } else if (button.font_size <= 17) {
            font = &lv_font_montserrat_16;
        } else if (button.font_size <= 19) {
            font = &lv_font_montserrat_18;
        } else if (button.font_size <= 21) {
            font = &lv_font_montserrat_20;
        } else if (button.font_size <= 23) {
            font = &lv_font_montserrat_22;
        } else if (button.font_size <= 26) {
            font = &lv_font_montserrat_24;
        } else if (button.font_size <= 30) {
            font = &lv_font_montserrat_28;
        } else {
            font = &lv_font_montserrat_32;
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

        lv_obj_t* subtitle = lv_label_create(btn);
        if (button.can.enabled || button.can.pgn != 0) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "PGN 0x%05lX%s",
                static_cast<unsigned long>(button.can.pgn),
                button.can.enabled ? "" : " (disabled)");
            lv_label_set_text(subtitle, buffer);
            UITheme::apply_label_style(subtitle, UITheme::FONT_CAPTION, UITheme::COLOR_BG);
        } else {
            lv_label_set_text(subtitle, "No CAN frame assigned");
            UITheme::apply_label_style(subtitle, UITheme::FONT_CAPTION, UITheme::COLOR_BG);
        }
        lv_obj_align(subtitle, LV_ALIGN_BOTTOM_LEFT, 0, 0);
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
            // Apply active color
            lv_color_t active_color = colorFromHex(config_->theme.nav_button_active_color, UITheme::COLOR_ACCENT);
            lv_obj_set_style_bg_color(btn, active_color, LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
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

void UIBuilder::updateHeaderBranding() {
    if (!config_ || !header_title_label_ || !header_overlay_) {
        return;
    }

    lv_label_set_text(header_title_label_, config_->header.title.c_str());

    if (header_subtitle_label_) {
        if (config_->header.subtitle.empty()) {
            lv_obj_add_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(header_subtitle_label_, config_->header.subtitle.c_str());
            lv_obj_clear_flag(header_subtitle_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (!header_logo_img_) {
        return;
    }

    if (config_->header.show_logo) {
        if (const lv_img_dsc_t* logo = iconForId(config_->header.logo_variant)) {
            lv_img_set_src(header_logo_img_, logo);
            lv_obj_clear_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(header_logo_img_, LV_OBJ_FLAG_HIDDEN);
    }
}

const lv_img_dsc_t* UIBuilder::iconForId(const std::string& id) const {
    if (id.empty() || id == "none") {
        return nullptr;
    }
    for (const auto& entry : kIconEntries) {
        if (id == entry.id) {
            return entry.img;
        }
    }
    return nullptr;
}

void UIBuilder::updateNetworkStatus(const std::string& ap_ip, const std::string& sta_ip, bool sta_connected) {
    last_ap_ip_ = ap_ip;
    last_sta_ip_ = sta_ip;
    last_sta_connected_ = sta_connected;

    if (!status_ap_label_ || !status_sta_label_ || !status_ap_chip_ || !status_sta_chip_ || !status_panel_) {
        return;
    }

    const bool ap_valid = !ap_ip.empty() && ap_ip != "0.0.0.0";
    const bool sta_valid = sta_connected && !sta_ip.empty() && sta_ip != "0.0.0.0";
    
    // Only show network status when WiFi is connected (either AP or STA)
    if (ap_valid || sta_valid) {
        lv_obj_clear_flag(status_panel_, LV_OBJ_FLAG_HIDDEN);
        
        if (ap_valid) {
            lv_label_set_text_fmt(status_ap_label_, "AP %s", ap_ip.c_str());
            lv_obj_clear_flag(status_ap_chip_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(status_ap_chip_, LV_OBJ_FLAG_HIDDEN);
        }
        
        if (sta_valid) {
            lv_label_set_text_fmt(status_sta_label_, "LAN %s", sta_ip.c_str());
            lv_obj_clear_flag(status_sta_chip_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(status_sta_chip_, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_style_bg_color(status_sta_chip_, sta_valid ? UITheme::COLOR_SUCCESS : UITheme::COLOR_SURFACE, 0);
        lv_obj_set_style_bg_opa(status_sta_chip_, sta_valid ? LV_OPA_80 : LV_OPA_40, 0);
        lv_obj_set_style_bg_color(status_ap_chip_, UITheme::COLOR_SURFACE, 0);
        lv_obj_set_style_bg_opa(status_ap_chip_, ap_valid ? LV_OPA_80 : LV_OPA_40, 0);
    } else {
        // Hide status panel when not connected to WiFi (vehicle mode)
        lv_obj_add_flag(status_panel_, LV_OBJ_FLAG_HIDDEN);
    }
}
