# Component and Screen Expansion Guide

This guide shows you how to extend the Bronco Controls HMI with new components and complete screen implementations.

## Current Project Status

✅ **COMPLETED:**
- PlatformIO configuration
- ESP32 panel hardware configuration
- LVGL configuration (v8.3.x)
- Design system with Bronco theme
- AppState management singleton
- Basic main.cpp with working home screen
- Complete documentation

📋 **TO BE IMPLEMENTED** (examples provided below):
- Detailed Windows screen with press-and-hold controls
- Detailed Locks screen with toggle switches
- Reusable components (TopBar, TileButton, Toggle, Slider, Card)
- Additional screens (Settings, Gauges, etc.)

## How to Add Components

### Example: Creating a Toggle Component

Create `src/components/toggle.h`:

```cpp
#ifndef TOGGLE_H
#define TOGGLE_H

#include <lvgl.h>
#include <functional>

class Toggle {
public:
    using StateCallback = std::function<void(bool)>;

    static lv_obj_t* create(lv_obj_t* parent, const char* label, bool initial_state = false);
    static void setState(lv_obj_t* toggle, bool state);
    static bool getState(lv_obj_t* toggle);
    static void setCallback(lv_obj_t* toggle, StateCallback callback);

private:
    static void toggle_event_cb(lv_event_t* e);
};

#endif
```

Create `src/components/toggle.cpp`:

```cpp
#include "toggle.h"
#include "../ui_theme.h"

lv_obj_t* Toggle::create(lv_obj_t* parent, const char* label_text, bool initial_state) {
    // Container for toggle + label
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_SIZE_CONTENT, UITheme::TOGGLE_HEIGHT + 10);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container, UITheme::SPACE_MD, 0);

    // Toggle switch
    lv_obj_t* sw = lv_switch_create(container);
    lv_obj_set_size(sw, UITheme::TOGGLE_WIDTH, UITheme::TOGGLE_HEIGHT);
    
    // Apply theme colors
    lv_obj_set_style_bg_color(sw, UITheme::COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, UITheme::COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, UITheme::COLOR_TEXT_PRIMARY, LV_PART_KNOB);
    
    if (initial_state) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    
    lv_obj_add_event_cb(sw, toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Label
    if (label_text) {
        lv_obj_t* label = lv_label_create(container);
        lv_label_set_text(label, label_text);
        UITheme::apply_label_style(label, UITheme::FONT_BODY, UITheme::COLOR_TEXT_PRIMARY);
    }

    return container;  // Return container for positioning
}

void Toggle::setState(lv_obj_t* toggle, bool state) {
    // Find the switch child
    lv_obj_t* sw = lv_obj_get_child(toggle, 0);
    if (sw && lv_obj_check_type(sw, &lv_switch_class)) {
        if (state) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
        }
    }
}

bool Toggle::getState(lv_obj_t* toggle) {
    lv_obj_t* sw = lv_obj_get_child(toggle, 0);
    if (sw && lv_obj_check_type(sw, &lv_switch_class)) {
        return lv_obj_has_state(sw, LV_STATE_CHECKED);
    }
    return false;
}

void Toggle::setCallback(lv_obj_t* toggle, StateCallback callback) {
    lv_obj_t* sw = lv_obj_get_child(toggle, 0);
    if (sw) {
        // Store callback in user data (need to heap-allocate)
        StateCallback* cb_ptr = new StateCallback(callback);
        lv_obj_set_user_data(sw, cb_ptr);
    }
}

void Toggle::toggle_event_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    
    StateCallback* callback = static_cast<StateCallback*>(lv_obj_get_user_data(sw));
    if (callback) {
        bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
        (*callback)(state);
    }
}
```

## How to Add Complete Screens

### Example: Windows Screen Implementation

Create `src/screens/windows_screen.h`:

```cpp
#ifndef WINDOWS_SCREEN_H
#define WINDOWS_SCREEN_H

#include <lvgl.h>

class WindowsScreen {
public:
    static lv_obj_t* create(lv_obj_t* parent = NULL);
    static void update();  // Call periodically to update UI from state
    static void destroy();

private:
    static lv_obj_t* screen;
    static lv_obj_t* window_buttons[4];
    
    static void back_button_cb(lv_event_t* e);
    static void window_button_cb(lv_event_t* e);
    static void update_button_state(uint8_t window_id);
};

#endif
```

Create `src/screens/windows_screen.cpp`:

```cpp
#include "windows_screen.h"
#include "../ui_theme.h"
#include "../app_state.h"

lv_obj_t* WindowsScreen::screen = nullptr;
lv_obj_t* WindowsScreen::window_buttons[4] = {nullptr};

lv_obj_t* WindowsScreen::create(lv_obj_t* parent) {
    // Create screen
    screen = lv_obj_create(parent);
    UITheme::apply_screen_style(screen);

    // Top bar with back button
    lv_obj_t* top_bar = lv_obj_create(screen);
    lv_obj_set_size(top_bar, LV_PCT(100), UITheme::TOP_BAR_HEIGHT);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    UITheme::apply_card_style(top_bar);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 60, 40);
    UITheme::apply_button_style(back_btn, true);  // Accent color
    lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Title
    lv_obj_t* title = lv_label_create(top_bar);
    lv_label_set_text(title, "WINDOWS");
    UITheme::apply_label_style(title, UITheme::FONT_HEADING, UITheme::COLOR_TEXT_PRIMARY);

    // Content area
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_size(content, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 20);
    
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(content, UITheme::SPACE_LG, 0);

    // Window buttons (press-and-hold style)
    const char* window_names[] = {"Driver", "Passenger", "Rear Left", "Rear Right"};
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_btn_create(content);
        lv_obj_set_size(btn, LV_PCT(100), 80);
        UITheme::apply_button_style(btn, false);
        lv_obj_add_event_cb(btn, window_button_cb, LV_EVENT_PRESSED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, window_button_cb, LV_EVENT_RELEASED, (void*)(intptr_t)i);
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%s\nPress and Hold", window_names[i]);
        UITheme::apply_label_style(label, UITheme::FONT_BODY, UITheme::COLOR_TEXT_PRIMARY);
        lv_obj_center(label);
        
        window_buttons[i] = btn;
        update_button_state(i);
    }

    return screen;
}

void WindowsScreen::back_button_cb(lv_event_t* e) {
    AppState::getInstance().navigateToScreen(Screen::HOME);
}

void WindowsScreen::window_button_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t window_id = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_PRESSED) {
        // Start opening window
        AppState::getInstance().openWindow(window_id);
        update_button_state(window_id);
    } else if (code == LV_EVENT_RELEASED) {
        // Stop window movement (in real impl, send stop command)
        update_button_state(window_id);
    }
}

void WindowsScreen::update_button_state(uint8_t window_id) {
    if (window_id >= 4 || !window_buttons[window_id]) return;
    
    WindowState state = AppState::getInstance().getWindowState(window_id);
    lv_obj_t* btn = window_buttons[window_id];
    
    // Update button appearance based on state
    switch (state) {
        case WindowState::OPEN:
            lv_obj_set_style_bg_color(btn, UITheme::COLOR_SUCCESS, 0);
            break;
        case WindowState::OPENING:
            lv_obj_set_style_bg_color(btn, UITheme::COLOR_ACCENT, 0);
            break;
        default:
            lv_obj_set_style_bg_color(btn, UITheme::COLOR_SURFACE, 0);
            break;
    }
}

void WindowsScreen::update() {
    for (int i = 0; i < 4; i++) {
        update_button_state(i);
    }
}

void WindowsScreen::destroy() {
    if (screen) {
        lv_obj_del(screen);
        screen = nullptr;
    }
}
```

## Integration into main.cpp

Update the `handle_navigation` function in main.cpp:

```cpp
#include "screens/windows_screen.h"
#include "screens/locks_screen.h"

void handle_navigation(Screen screen) {
    lvgl_port_lock(-1);
    
    switch (screen) {
        case Screen::HOME:
            // Already on home, do nothing
            break;
            
        case Screen::WINDOWS:
            if (current_screen_obj) lv_obj_del(current_screen_obj);
            current_screen_obj = WindowsScreen::create();
            lv_scr_load(current_screen_obj);
            break;
            
        case Screen::LOCKS:
            if (current_screen_obj) lv_obj_del(current_screen_obj);
            current_screen_obj = LocksScreen::create();
            lv_scr_load(current_screen_obj);
            break;
            
        default:
            break;
    }
    
    lvgl_port_unlock();
}
```

## File Organization Checklist

When you add new components and screens, maintain this structure:

```
src/
├── main.cpp                      # ✓ Created
├── ui_theme.h/cpp                # ✓ Created
├── app_state.h/cpp               # ✓ Created
├── components/                   # Create this directory
│   ├── top_bar.h/cpp            # Follow Toggle example
│   ├── tile_button.h/cpp        # Follow Toggle example
│   ├── toggle.h/cpp             # Example provided above
│   ├── slider.h/cpp             # Similar to Toggle
│   └── card.h/cpp               # Simple wrapper component
└── screens/                      # Create this directory
    ├── home_screen.h/cpp        # Already in main.cpp, can extract
    ├── windows_screen.h/cpp     # Example provided above
    ├── locks_screen.h/cpp       # Similar to Windows
    └── settings_screen.h/cpp    # Future expansion
```

## Build After Adding Files

After adding new `.cpp` files:

1. PlatformIO will automatically detect them
2. Just run **Build** again
3. No need to modify `platformio.ini`

## Common Patterns

### State Binding
```cpp
// In screen creation:
AppState::getInstance().setWindowStateCallback([]() {
    WindowsScreen::update();  // Update UI when state changes
});
```

### Flex Layouts (Preferred)
```cpp
lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_EVENLY, 
                      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
```

### Grid Layouts (For Regular Grids)
```cpp
static lv_coord_t col_dsc[] = {160, 160, 160, LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {120, 120, LV_GRID_TEMPLATE_LAST};
lv_obj_set_grid_dsc_array(container, col_dsc, row_dsc);
```

## Next Steps

1. **Implement Windows Screen**: Use the example above
2. **Implement Locks Screen**: Similar pattern with Toggle components
3. **Extract Home Screen**: Move from main.cpp to screens/home_screen.cpp
4. **Add Remaining Components**: TopBar, TileButton, Slider, Card
5. **Add Settings Screen**: Brightness, units, calibration
6. **Integrate CAN/J1939**: Use AppState callbacks to update from vehicle data

## Testing

After implementing new screens:

```cpp
// In main.cpp setup(), test navigation:
Serial.println("Testing navigation...");
delay(2000);
AppState::getInstance().navigateToScreen(Screen::WINDOWS);
delay(2000);
AppState::getInstance().navigateToScreen(Screen::LOCKS);
delay(2000);
AppState::getInstance().navigateToScreen(Screen::HOME);
```

---

**The foundation is solid. Build on it systematically!** 🚀
