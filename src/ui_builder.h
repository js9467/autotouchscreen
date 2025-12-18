#pragma once

#include <lvgl.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "config_types.h"

class UIBuilder {
public:
    static UIBuilder& instance();

    void begin();
    void applyConfig(const DeviceConfig& config);

    void markDirty();
    bool consumeDirtyFlag();
    void updateNetworkStatus(const std::string& ap_ip, const std::string& sta_ip, bool sta_connected);

private:
    UIBuilder() = default;

    void createBaseScreen();
    void buildNavigation();
    void buildEmptyState();
    void buildPage(std::size_t index);
    void updateNavSelection();
    void updateHeaderBranding();
    const lv_img_dsc_t* iconForId(const std::string& id) const;

    static void navButtonEvent(lv_event_t* e);
    static void actionButtonEvent(lv_event_t* e);
    static lv_color_t colorFromHex(const std::string& hex, lv_color_t fallback);

    const DeviceConfig* config_ = nullptr;
    lv_obj_t* base_screen_ = nullptr;
    lv_obj_t* header_bar_ = nullptr;
    lv_obj_t* header_overlay_ = nullptr;
    lv_obj_t* header_logo_img_ = nullptr;
    lv_obj_t* header_title_label_ = nullptr;
    lv_obj_t* header_subtitle_label_ = nullptr;
    lv_obj_t* content_root_ = nullptr;
    lv_obj_t* nav_bar_ = nullptr;
    lv_obj_t* status_panel_ = nullptr;
    lv_obj_t* status_ap_chip_ = nullptr;
    lv_obj_t* status_sta_chip_ = nullptr;
    lv_obj_t* status_ap_label_ = nullptr;
    lv_obj_t* status_sta_label_ = nullptr;
    lv_obj_t* page_container_ = nullptr;
    std::vector<lv_obj_t*> nav_buttons_;
    std::vector<lv_coord_t> grid_cols_;
    std::vector<lv_coord_t> grid_rows_;
    std::size_t active_page_ = 0;
    bool dirty_ = false;
    std::string last_ap_ip_ = "";
    std::string last_sta_ip_ = "";
    bool last_sta_connected_ = false;
};
