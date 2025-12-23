#pragma once

#include <lvgl.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "config_types.h"
#include "ui_theme.h"

class UIBuilder {
public:
    static UIBuilder& instance();

    void begin();
    void applyConfig(const DeviceConfig& config);
    void updateOtaStatus(const std::string& status);

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
    void createInfoModal();
    void showInfoModal();
    void hideInfoModal();
    void loadSleepIcon();
    void armSleepTimer();
    void resetSleepTimer();
    void showSleepOverlay();
    void hideSleepOverlay();
    void setBrightness(uint8_t percent);
    const lv_img_dsc_t* iconForId(const std::string& id) const;
    const lv_font_t* fontFromName(const std::string& name) const;
    const lv_font_t* navLabelFontForText(const std::string& text) const;
    uint32_t nextUtf8Codepoint(const std::string& text, std::size_t& index) const;
    std::vector<uint8_t> decodeBase64Logo(const std::string& data_uri);
    bool loadImageDescriptor(const std::string& data_uri, std::vector<uint8_t>& pixel_buffer, lv_img_dsc_t& descriptor, bool scrub_white_background = false);
    void applyHeaderNavSpacing();
    void applyHeaderLogoSizing(uint16_t src_width, uint16_t src_height, bool inline_layout);
    void refreshOtaStatusLabel();
    std::string humanizeOtaStatus(const std::string& status) const;
    lv_color_t colorForOtaStatus(const std::string& status) const;
    void refreshNetworkStatusIndicators();
    void refreshOtaStatusBar();
    void updateOtaActionState();
    static bool startsWith(const std::string& text, const char* prefix);
    bool isOtaStatusError(const std::string& status) const;
    uint8_t otaStatusProgress(const std::string& status) const;
    enum class DiagnosticsPriority : std::uint8_t { NORMAL = 0, WARNING = 1, ERROR = 2 };
    enum class OtaAction : std::uint8_t { INSTALL = 0, CHECK_ONLY = 1, BLOCKED = 2 };
    void setDiagnosticsMessage(const std::string& text, DiagnosticsPriority priority, bool force);
    void refreshNetworkStatusLabel();
    std::string connectionStatusText() const;
    lv_color_t connectionStatusColor() const;
    void refreshVersionLabel();

    static void navButtonEvent(lv_event_t* e);
    static void actionButtonEvent(lv_event_t* e);
    static void settingsButtonEvent(lv_event_t* e);
    static void otaUpdateButtonEvent(lv_event_t* e);
    static void infoModalCloseEvent(lv_event_t* e);
    static void infoModalBackdropEvent(lv_event_t* e);
    static void brightnessSliderEvent(lv_event_t* e);
    static void modalActivityEvent(lv_event_t* e);
    static lv_color_t colorFromHex(const std::string& hex, lv_color_t fallback);

    const DeviceConfig* config_ = nullptr;
    lv_obj_t* base_screen_ = nullptr;
    lv_obj_t* header_bar_ = nullptr;
    lv_obj_t* header_brand_row_ = nullptr;
    lv_obj_t* header_overlay_ = nullptr;
    lv_obj_t* header_logo_slot_ = nullptr;
    lv_obj_t* header_logo_img_ = nullptr;
    lv_obj_t* header_text_container_ = nullptr;
    std::vector<uint8_t> logo_buffer_;
    lv_img_dsc_t header_logo_dsc_{};
    bool header_logo_ready_ = false;
    lv_obj_t* header_title_label_ = nullptr;
    lv_obj_t* header_subtitle_label_ = nullptr;
    lv_obj_t* info_modal_ = nullptr;
    lv_obj_t* info_modal_bg_ = nullptr;
    lv_obj_t* brightness_value_label_ = nullptr;
    lv_obj_t* brightness_slider_ = nullptr;
    lv_obj_t* network_status_label_ = nullptr;
    lv_obj_t* version_label_ = nullptr;
    lv_obj_t* ota_status_label_ = nullptr;
    lv_obj_t* ota_primary_button_ = nullptr;
    lv_obj_t* ota_primary_button_label_ = nullptr;
    lv_obj_t* info_ip_label_ = nullptr;
    lv_obj_t* network_status_bar_ = nullptr;
    lv_obj_t* ota_status_bar_ = nullptr;
    lv_obj_t* diagnostics_label_ = nullptr;
    lv_obj_t* sleep_overlay_ = nullptr;
    lv_obj_t* sleep_image_ = nullptr;
    lv_timer_t* sleep_timer_ = nullptr;
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
    lv_coord_t nav_base_pad_top_ = UITheme::SPACE_XS;
    std::string last_ap_ip_ = "";
    std::string last_sta_ip_ = "";
    bool last_sta_connected_ = false;
    std::vector<uint8_t> sleep_icon_buffer_;  // Kept for compatibility
    lv_img_dsc_t sleep_logo_dsc_{};
    bool sleep_logo_ready_ = false;
    std::string ota_status_text_ = "idle";
    OtaAction ota_primary_action_ = OtaAction::INSTALL;
    DiagnosticsPriority diag_priority_ = DiagnosticsPriority::NORMAL;
    bool info_modal_visible_ = false;
};
