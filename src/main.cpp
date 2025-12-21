/**
 * @file main.cpp
 * Bronco Controls - Web-configurable automotive HMI
 *
 * Boots the LVGL runtime, loads configuration from LittleFS,
 * and exposes a WiFi + web interface for live customization.
 */

#include <Arduino.h>
#include <ESP_IOExpander_Library.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include <WiFi.h>
#include <string>

#include "can_manager.h"
#include "config_manager.h"
#include "ui_builder.h"
#include "ui_theme.h"
#include "web_server.h"
#include "ota_manager.h"

// IO pin definitions for the Waveshare ESP32-S3-Touch-LCD-4.3 board
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5

// I2C definitions for the external IO expander (CH422G)
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

// LVGL port configuration
#define LVGL_TICK_PERIOD_MS     (2)
#define LVGL_TASK_MAX_DELAY_MS  (500)
#define LVGL_TASK_MIN_DELAY_MS  (1)
#define LVGL_TASK_STACK_SIZE    (6 * 1024)
#define LVGL_TASK_PRIORITY      (2)
#define LVGL_BUF_SIZE           (ESP_PANEL_LCD_H_RES * 40)

// Globals
ESP_Panel* panel = nullptr;
SemaphoreHandle_t lvgl_mux = nullptr;

// Forward declarations for LVGL helpers
void lvgl_port_lock(int timeout_ms);
void lvgl_port_unlock();

#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);
}
#else
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}

bool notify_lvgl_flush_ready(void* user_ctx) {
    lv_disp_drv_t* disp_driver = static_cast<lv_disp_drv_t*>(user_ctx);
    lv_disp_flush_ready(disp_driver);
    return false;
}
#endif

#if ESP_PANEL_USE_LCD_TOUCH
void lvgl_port_tp_read(lv_indev_drv_t* indev, lv_indev_data_t* data) {
    panel->getLcdTouch()->readData();

    bool touched = panel->getLcdTouch()->getTouchState();
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    TouchPoint point = panel->getLcdTouch()->getPoint();
    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;
}
#endif

void lvgl_port_lock(int timeout_ms) {
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks);
}

void lvgl_port_unlock() {
    xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_port_task(void* arg) {
    Serial.println("[LVGL] Task started");

    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (true) {
        lvgl_port_lock(-1);
        task_delay_ms = lv_timer_handler();
        lvgl_port_unlock();

        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=================================");
    Serial.println(" Bronco Controls - Web Config ");
    Serial.println("=================================");

    // Initialize LVGL core
    lv_init();

    // Initialize display panel
    panel = new ESP_Panel();

    // Configure IO expander
    ESP_IOExpander* expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
    expander->init();
    expander->begin();
    expander->multiPinMode(TP_RST | LCD_BL | LCD_RST | SD_CS | USB_SEL, OUTPUT);
    expander->multiDigitalWrite(TP_RST | LCD_BL | LCD_RST | SD_CS, HIGH);
    expander->digitalWrite(USB_SEL, LOW);
    panel->addIOExpander(expander);

    // LVGL draw buffers in PSRAM
    static lv_disp_draw_buf_t draw_buf;
    uint8_t* buf = static_cast<uint8_t*>(heap_caps_calloc(1, LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
    if (!buf) {
        Serial.println("[Error] Unable to allocate LVGL buffer in PSRAM");
        while (true) {
            delay(1000);
        }
    }
    lv_disp_draw_buf_init(&draw_buf, buf, nullptr, LVGL_BUF_SIZE);

    // Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ESP_PANEL_LCD_H_RES;
    disp_drv.ver_res = ESP_PANEL_LCD_V_RES;
    disp_drv.flush_cb = lvgl_port_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

#if ESP_PANEL_USE_LCD_TOUCH
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_port_tp_read;
    lv_indev_drv_register(&indev_drv);
#endif

    panel->init();
#if ESP_PANEL_LCD_BUS_TYPE != ESP_PANEL_BUS_TYPE_RGB
    panel->getLcd()->setCallback(notify_lvgl_flush_ready, &disp_drv);
#endif
    panel->begin();

    // Start LVGL background task
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    xTaskCreate(lvgl_port_task, "lvgl", LVGL_TASK_STACK_SIZE, nullptr, LVGL_TASK_PRIORITY, nullptr);

    // Load configuration from flash
    if (!ConfigManager::instance().begin()) {
        Serial.println("[Config] Failed to mount LittleFS; factory defaults applied.");
    }

    // Ready CAN (TWAI) bus
    CanManager::instance().begin();

    // Build the themed UI once before networking spins up
    lvgl_port_lock(-1);
    UITheme::init();
    UIBuilder::instance().begin();
    UIBuilder::instance().applyConfig(ConfigManager::instance().getConfig());
    lvgl_port_unlock();

    // Launch WiFi access point + web server
    WebServerManager::instance().begin();
    OTAUpdateManager::instance().begin();

    Serial.println("=================================");
    Serial.println(" Touch the screen or open http://192.168.4.250 ");
    Serial.println("=================================");
}

void loop() {
    static uint32_t last_network_push_ms = 0;
    static uint32_t ap_start_ms = 0;
    static bool ap_shutdown_complete = false;
    static std::string last_ota_status_pushed;
    
    // Record AP start time on first loop
    if (ap_start_ms == 0) {
        ap_start_ms = millis();
    }
    
    // Shutdown AP after 90 seconds
    if (!ap_shutdown_complete && (millis() - ap_start_ms >= 90000)) {
        WebServerManager::instance().disableAP();
        ap_shutdown_complete = true;
        Serial.println("[WiFi] AP disabled after 90 seconds");
    }

    if (UIBuilder::instance().consumeDirtyFlag()) {
        lvgl_port_lock(-1);
        UIBuilder::instance().applyConfig(ConfigManager::instance().getConfig());
        lvgl_port_unlock();
    }

    const uint32_t now = millis();
    WifiStatusSnapshot snapshot = WebServerManager::instance().getStatusSnapshot();
    if (now - last_network_push_ms >= 1000) {
        std::string ap_ip = snapshot.ap_ip.toString().c_str();
        std::string sta_ip = snapshot.sta_ip.toString().c_str();
        lvgl_port_lock(-1);
        UIBuilder::instance().updateNetworkStatus(ap_ip, sta_ip, snapshot.sta_connected);
        lvgl_port_unlock();
        last_network_push_ms = now;
    }

    OTAUpdateManager& ota = OTAUpdateManager::instance();
    ota.loop(snapshot);

    const std::string& ota_status = ota.lastStatus();
    if (ota_status != last_ota_status_pushed) {
        lvgl_port_lock(-1);
        UIBuilder::instance().updateOtaStatus(ota_status);
        lvgl_port_unlock();
        last_ota_status_pushed = ota_status;
    }

    WebServerManager::instance().loop();
    vTaskDelay(pdMS_TO_TICKS(50));
}
