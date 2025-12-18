/**
 * @file main.cpp
 * Bronco Controls - Main entry point
 * 
 * Initializes LVGL, display panel, touch, and creates UI screens
 */

#include <Arduino.h>
#include <lvgl.h>
#include <ESP_Panel_Library.h>
#include <ESP_IOExpander_Library.h>

#include "ui_theme.h"
#include "app_state.h"
#include "assets/images.h"

// Extend IO Pin definitions for Waveshare board
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5

// I2C Pin definitions
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

// LVGL porting configurations
#define LVGL_TICK_PERIOD_MS     (2)
#define LVGL_TASK_MAX_DELAY_MS  (500)
#define LVGL_TASK_MIN_DELAY_MS  (1)
#define LVGL_TASK_STACK_SIZE    (6 * 1024)  // Increased for automotive UI
#define LVGL_TASK_PRIORITY      (2)
#define LVGL_BUF_SIZE           (ESP_PANEL_LCD_H_RES * 40)  // 40 lines

// Global objects
ESP_Panel *panel = NULL;
SemaphoreHandle_t lvgl_mux = NULL;
lv_obj_t *current_screen_obj = NULL;

// Forward declarations - Page builders
void build_home_page(lv_obj_t* content);
void build_windows_page(lv_obj_t* content);
void build_locks_page(lv_obj_t* content);
void build_running_boards_page(lv_obj_t* content);
void handle_navigation(Screen screen);

// UI Components
static lv_obj_t* base_screen = nullptr;
static lv_obj_t* content_container = nullptr;
static lv_obj_t* header_bar = nullptr;

// ========== LVGL DISPLAY CALLBACKS ==========

#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
/* RGB display flush */
void lvgl_port_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);
}
#else
/* Non-RGB display flush */
void lvgl_port_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}

bool notify_lvgl_flush_ready(void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}
#endif

// ========== TOUCH INPUT CALLBACK ==========

#if ESP_PANEL_USE_LCD_TOUCH
void lvgl_port_tp_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    panel->getLcdTouch()->readData();

    bool touched = panel->getLcdTouch()->getTouchState();
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        TouchPoint point = panel->getLcdTouch()->getPoint();

        data->state = LV_INDEV_STATE_PR;
        data->point.x = point.x;
        data->point.y = point.y;

        // Debug touch coordinates
        // Serial.printf("Touch: x=%d, y=%d\n", point.x, point.y);
    }
}
#endif

// ========== LVGL MUTEX ==========

void lvgl_port_lock(int timeout_ms) {
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks);
}

void lvgl_port_unlock(void) {
    xSemaphoreGiveRecursive(lvgl_mux);
}

// ========== LVGL TASK ==========

void lvgl_port_task(void *arg) {
    Serial.println("Starting LVGL task");

    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        // Lock mutex for LVGL API calls
        lvgl_port_lock(-1);
        
        task_delay_ms = lv_timer_handler();
        
        // Update app state uptime
        AppState::getInstance().updateUptime(millis());
        
        // Release mutex
        lvgl_port_unlock();

        // Limit task delay
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// ========== WINDOW HOLD CONTROLS ==========

static lv_timer_t* window_timer = nullptr;
static int window_hold_id = -1;
static int window_hold_dir = 0;

static void window_timer_cb(lv_timer_t*) {
    // TODO: Send CAN/J1939 commands here
    Serial.printf("WINDOW HOLD id=%d dir=%d\n", window_hold_id, window_hold_dir);
}

static void start_window_hold(int id, int dir) {
    window_hold_id = id;
    window_hold_dir = dir;
    if (!window_timer) window_timer = lv_timer_create(window_timer_cb, 80, nullptr);
}

static void stop_window_hold() {
    if (window_timer) { lv_timer_del(window_timer); window_timer = nullptr; }
    window_hold_id = -1;
    window_hold_dir = 0;
}

static void window_hold_btn_event(lv_event_t* e) {
    int packed = (int)(intptr_t)lv_event_get_user_data(e);
    int id = (packed >> 8) & 0xFF;
    int dir = (packed & 0xFF) ? +1 : -1;

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        start_window_hold(id, dir);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        stop_window_hold();
    }
}

// ========== LOCK TOGGLE STATE ==========

static bool ui_locked = false;
static lv_obj_t* lock_icon = nullptr;
static lv_obj_t* lock_label = nullptr;

static void update_lock_ui() {
    if (!lock_icon || !lock_label) return;
    lv_label_set_text(lock_icon, ui_locked ? "\xEF\x80\x82" : "\xEF\x81\xBD");
    lv_label_set_text(lock_label, ui_locked ? "LOCKED" : "UNLOCKED");
    lv_obj_set_style_text_color(lock_icon,
        ui_locked ? UITheme::COLOR_ACCENT : UITheme::COLOR_TEXT_SECONDARY, 0);
}

static void lock_toggle_event(lv_event_t*) {
    ui_locked = !ui_locked;
    update_lock_ui();
    if (ui_locked) AppState::getInstance().lockAll();
    else AppState::getInstance().unlockAll();
}

// ========== RUNNING BOARDS STATE ==========

enum class RBState { DEPLOYED, RETRACTED, UNKNOWN };
static RBState rb_state = RBState::UNKNOWN;
static lv_obj_t* rb_status_label = nullptr;

static void update_rb_ui() {
    if (!rb_status_label) return;
    const char* text = (rb_state == RBState::DEPLOYED) ? "STATE: DEPLOYED" :
                       (rb_state == RBState::RETRACTED) ? "STATE: RETRACTED" : "STATE: UNKNOWN";
    lv_label_set_text(rb_status_label, text);
}

static void rb_deploy_event(lv_event_t*) {
    rb_state = RBState::DEPLOYED;
    update_rb_ui();
    // TODO: Send CAN command
    Serial.println("Running boards: DEPLOY");
}

static void rb_retract_event(lv_event_t*) {
    rb_state = RBState::RETRACTED;
    update_rb_ui();
    // TODO: Send CAN command
    Serial.println("Running boards: RETRACT");
}

// ========== NAV RAIL HELPERS ==========

// Static screen targets for callbacks
static Screen S_HOME = Screen::HOME;
static Screen S_WINDOWS = Screen::WINDOWS;
static Screen S_LOCKS = Screen::LOCKS;
static Screen S_RB = Screen::RUNNING_BOARDS;

static void nav_btn_event(lv_event_t* e) {
    Screen tgt = *(Screen*)lv_event_get_user_data(e);
    AppState::getInstance().navigateToScreen(tgt);
}

// Helper to create back button (bottom-right white circular arrow)
static void add_back_button(lv_obj_t* parent) {
    lv_obj_t* back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 70, 70);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
    lv_obj_set_style_radius(back_btn, 35, 0);  // Circular
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_width(back_btn, 2, 0);
    lv_obj_set_style_border_color(back_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_shadow_width(back_btn, 10, 0);
    lv_obj_add_event_cb(back_btn, nav_btn_event, LV_EVENT_CLICKED, &S_HOME);
    
    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " ");
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_32, 0);
    lv_obj_center(back_lbl);
}

// ========== BASE SCREEN CREATION ==========

void create_base_screen() {
    base_screen = lv_obj_create(NULL);
    UITheme::apply_screen_style(base_screen);

    // Header image (800×70) - 675KB
    header_bar = lv_img_create(base_screen);
    lv_img_set_src(header_bar, &img_header);
    lv_obj_set_pos(header_bar, 0, 0);

    // Content container (full width below header) - 800×410px
    content_container = lv_obj_create(base_screen);
    lv_obj_set_size(content_container, 800, 410);
    lv_obj_set_pos(content_container, 0, 70);
    lv_obj_set_style_bg_opa(content_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_container, 0, 0);
    lv_obj_set_style_pad_all(content_container, UITheme::SPACE_LG, 0);

    lv_scr_load(base_screen);
}

// ========== HOME PAGE (Full-Screen 2×2 Grid) ==========

void build_home_page(lv_obj_t* content) {
    // 2×2 grid layout
    lv_obj_set_layout(content, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_gap(content, 0, 0);

    auto make_tile = [&](const lv_img_dsc_t* icon_dsc, const char* label_text, Screen* target_scr, uint8_t col, uint8_t row) {
        lv_obj_t* tile = lv_btn_create(content);
        lv_obj_set_size(tile, LV_PCT(100), LV_PCT(100));
        lv_obj_set_grid_cell(tile, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_style_bg_opa(tile, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_radius(tile, 0, 0);
        lv_obj_add_event_cb(tile, nav_btn_event, LV_EVENT_CLICKED, target_scr);

        // Icon centered
        lv_obj_t* icon = lv_img_create(tile);
        lv_img_set_src(icon, icon_dsc);
        lv_img_set_antialias(icon, true);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -20);

        // Label below icon
        lv_obj_t* lbl = lv_label_create(tile);
        lv_label_set_text(lbl, label_text);
        UITheme::apply_label_style(lbl, UITheme::FONT_HEADING, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 70);
    };

    // 2×2 grid: Windows (top-left), Locks (top-right), Running Boards (bottom-left), Home (bottom-right)
    make_tile(&img_windows_icon, "WINDOWS", &S_WINDOWS, 0, 0);
    make_tile(&img_locks_icon, "LOCKS", &S_LOCKS, 1, 0);
    make_tile(&img_runningboards_icon, "RUNNING BOARDS", &S_RB, 0, 1);
    make_tile(&img_home_icon, "HOME", &S_HOME, 1, 1);
}

// ========== WINDOWS PAGE ==========

void build_windows_page(lv_obj_t* content) {
    // Main content area
    lv_obj_t* main_content = lv_obj_create(content);
    lv_obj_set_size(main_content, 760, 320);
    lv_obj_set_pos(main_content, 20, 10);
    lv_obj_set_style_bg_opa(main_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_content, 0, 0);
    lv_obj_set_flex_flow(main_content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    auto make_column = [&](const char* name, int id) {
        lv_obj_t* col = lv_obj_create(main_content);
        lv_obj_set_size(col, 320, LV_PCT(95));
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl = lv_label_create(col);
        lv_label_set_text(lbl, name);
        UITheme::apply_label_style(lbl, UITheme::FONT_HEADING, UITheme::COLOR_TEXT_PRIMARY);

        // UP button
        lv_obj_t* up = lv_btn_create(col);
        UITheme::apply_button_style(up, true);
        lv_obj_set_size(up, 280, 100);
        lv_obj_add_event_cb(up, window_hold_btn_event, LV_EVENT_PRESSED,  (void*)(intptr_t)((id<<8) | 1));
        lv_obj_add_event_cb(up, window_hold_btn_event, LV_EVENT_RELEASED, (void*)(intptr_t)((id<<8) | 1));
        lv_obj_add_event_cb(up, window_hold_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)((id<<8) | 1));
        lv_obj_t* up_lbl = lv_label_create(up);
        lv_label_set_text(up_lbl, "UP (HOLD)");
        UITheme::apply_label_style(up_lbl, UITheme::FONT_BODY, UITheme::COLOR_BG);
        lv_obj_center(up_lbl);

        // DOWN button
        lv_obj_t* down = lv_btn_create(col);
        UITheme::apply_button_style(down, false);
        lv_obj_set_size(down, 280, 100);
        lv_obj_add_event_cb(down, window_hold_btn_event, LV_EVENT_PRESSED,  (void*)(intptr_t)((id<<8) | 0));
        lv_obj_add_event_cb(down, window_hold_btn_event, LV_EVENT_RELEASED, (void*)(intptr_t)((id<<8) | 0));
        lv_obj_add_event_cb(down, window_hold_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)((id<<8) | 0));
        lv_obj_t* dn_lbl = lv_label_create(down);
        lv_label_set_text(dn_lbl, "DOWN (HOLD)");
        UITheme::apply_label_style(dn_lbl, UITheme::FONT_BODY, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_center(dn_lbl);
    };

    make_column("DRIVER", 0);
    make_column("PASSENGER", 1);
    
    // Back button
    add_back_button(content);
}

// ========== LOCKS PAGE ==========

void build_locks_page(lv_obj_t* content) {
    // Main content area
    lv_obj_t* main_content = lv_obj_create(content);
    lv_obj_set_size(main_content, 760, 380);
    lv_obj_set_pos(main_content, 20, 10);
    lv_obj_set_style_bg_opa(main_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_content, 0, 0);
    lv_obj_set_flex_flow(main_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* card = lv_obj_create(main_content);
    lv_obj_set_size(card, LV_PCT(85), LV_PCT(75));
    UITheme::apply_card_style(card);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, lock_toggle_event, LV_EVENT_CLICKED, nullptr);

    lock_icon = lv_label_create(card);
    UITheme::apply_label_style(lock_icon, &lv_font_montserrat_32, UITheme::COLOR_ACCENT);
    lv_obj_align(lock_icon, LV_ALIGN_CENTER, 0, -30);

    lock_label = lv_label_create(card);
    UITheme::apply_label_style(lock_label, UITheme::FONT_HEADING, UITheme::COLOR_TEXT_PRIMARY);
    lv_obj_align(lock_label, LV_ALIGN_CENTER, 0, 40);

    // Sync initial state
    ui_locked = (AppState::getInstance().getLockState(0) == LockState::LOCKED);
    update_lock_ui();
    
    // Back button
    add_back_button(content);
}

// ========== RUNNING BOARDS PAGE ==========

void build_running_boards_page(lv_obj_t* content) {
    // Main content area
    lv_obj_t* main_content = lv_obj_create(content);
    lv_obj_set_size(main_content, 760, 380);
    lv_obj_set_pos(main_content, 20, 10);
    lv_obj_set_style_bg_opa(main_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(main_content, 0, 0);
    lv_obj_set_flex_flow(main_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Icon at top
    lv_obj_t* icon = lv_img_create(main_content);
    lv_img_set_src(icon, &img_runningboards_icon);
    lv_img_set_antialias(icon, true);
    lv_obj_set_style_img_recolor(icon, UITheme::COLOR_ACCENT, 0);
    lv_obj_set_style_img_recolor_opa(icon, 80, 0);

    // DEPLOY button
    lv_obj_t* deploy_btn = lv_btn_create(main_content);
    UITheme::apply_button_style(deploy_btn, true);
    lv_obj_set_size(deploy_btn, 350, 100);
    lv_obj_add_event_cb(deploy_btn, rb_deploy_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* deploy_lbl = lv_label_create(deploy_btn);
    lv_label_set_text(deploy_lbl, "DEPLOY");
    UITheme::apply_label_style(deploy_lbl, UITheme::FONT_HEADING, UITheme::COLOR_BG);
    lv_obj_center(deploy_lbl);

    // RETRACT button
    lv_obj_t* retract_btn = lv_btn_create(content);
    UITheme::apply_button_style(retract_btn, false);
    lv_obj_set_size(retract_btn, 350, 100);
    lv_obj_add_event_cb(retract_btn, rb_retract_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* retract_lbl = lv_label_create(retract_btn);
    lv_label_set_text(retract_lbl, "RETRACT");
    UITheme::apply_label_style(retract_lbl, UITheme::FONT_HEADING, UITheme::COLOR_TEXT_PRIMARY);
    lv_obj_center(retract_lbl);

    // Status label
    rb_status_label = lv_label_create(main_content);
    UITheme::apply_label_style(rb_status_label, UITheme::FONT_BODY, UITheme::COLOR_TEXT_SECONDARY);
    update_rb_ui();
    
    // Back button
    add_back_button(content);
}

// ========== NAVIGATION HANDLER (content swap, no flicker) ==========

void handle_navigation(Screen screen) {
    Serial.printf("Navigate to screen: %d\n", (int)screen);

    lvgl_port_lock(-1);
    
    // Clean content container (removes all children)
    lv_obj_clean(content_container);

    // Build the appropriate page content
    switch (screen) {
        case Screen::HOME:           build_home_page(content_container);           break;
        case Screen::WINDOWS:        build_windows_page(content_container);        break;
        case Screen::LOCKS:          build_locks_page(content_container);          break;
        case Screen::RUNNING_BOARDS: build_running_boards_page(content_container); break;
        default:                     build_home_page(content_container);           break;
    }
    
    lvgl_port_unlock();
}

// ========== MAIN SETUP ==========

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=================================");
    Serial.println("Bronco Controls - Automotive HMI");
    Serial.println("=================================\n");

    // ========== Initialize LVGL ==========
    Serial.println("Initializing LVGL...");
    lv_init();

    // ========== Initialize Display Panel ==========
    Serial.println("Initializing display panel...");
    panel = new ESP_Panel();

    // ========== Initialize IO Expander (CH422G) ==========
    Serial.println("Initializing IO expander...");
    ESP_IOExpander *expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
    expander->init();
    expander->begin();
    expander->multiPinMode(TP_RST | LCD_BL | LCD_RST | SD_CS | USB_SEL, OUTPUT);
    expander->multiDigitalWrite(TP_RST | LCD_BL | LCD_RST | SD_CS, HIGH);
    expander->digitalWrite(USB_SEL, LOW);
    panel->addIOExpander(expander);

    // ========== Initialize LVGL Buffers (use PSRAM) ==========
    Serial.println("Allocating LVGL buffers in PSRAM...");
    static lv_disp_draw_buf_t draw_buf;
    uint8_t *buf = (uint8_t *)heap_caps_calloc(1, LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!buf) {
        Serial.println("ERROR: Failed to allocate LVGL buffer in PSRAM!");
        while (1) { delay(1000); }
    }
    Serial.printf("Buffer allocated: %d bytes\n", LVGL_BUF_SIZE * sizeof(lv_color_t));
    
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_BUF_SIZE);

    // ========== Initialize Display Driver ==========
    Serial.println("Registering display driver...");
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ESP_PANEL_LCD_H_RES;
    disp_drv.ver_res = ESP_PANEL_LCD_V_RES;
    disp_drv.flush_cb = lvgl_port_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // ========== Initialize Touch Driver ==========
#if ESP_PANEL_USE_LCD_TOUCH
    Serial.println("Registering touch driver...");
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_port_tp_read;
    lv_indev_drv_register(&indev_drv);
#endif

    // ========== Initialize Panel Hardware ==========
    Serial.println("Starting panel...");
    panel->init();
    
#if ESP_PANEL_LCD_BUS_TYPE != ESP_PANEL_BUS_TYPE_RGB
    panel->getLcd()->setCallback(notify_lvgl_flush_ready, &disp_drv);
#endif
    
    panel->begin();

    // ========== Start LVGL Task ==========
    Serial.println("Creating LVGL task...");
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    xTaskCreate(lvgl_port_task, "lvgl", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);

    // ========== Initialize Theme ==========
    lvgl_port_lock(-1);
    Serial.println("Initializing UI theme...");
    UITheme::init();

    // ========== Initialize App State ==========
    Serial.println("Initializing app state...");
    AppState::getInstance().setScreenChangeCallback(handle_navigation);

    // ========== Create Base Screen (once, with nav rail) ==========
    Serial.println("Creating base screen with nav rail...");
    create_base_screen();

    // ========== Load Home Page ==========
    Serial.println("Loading home page...");
    build_home_page(content_container);

    lvgl_port_unlock();

    Serial.println("\n=================================");
    Serial.println("Setup complete! Touch to interact");
    Serial.println("=================================\n");
}

// ========== MAIN LOOP ==========

void loop() {
    // LVGL runs in its own FreeRTOS task
    // Main loop can be used for CAN/J1939 processing in the future
    vTaskDelay(pdMS_TO_TICKS(1000));
}
