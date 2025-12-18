/*
 * Bronco Controls - ESP32 Display Panel Configuration
 * Configuration for Waveshare ESP32-S3-Touch-LCD-4.3
 * 
 * Hardware: 800x480 RGB LCD with ST7262 controller + GT911 capacitive touch
 */

#ifndef ESP_PANEL_CONF_H
#define ESP_PANEL_CONF_H

// Set to 0 to use custom board configuration
#define ESP_PANEL_USE_SUPPORTED_BOARD   (0)

/*-------------------------------- LCD Related --------------------------------*/
#define ESP_PANEL_USE_LCD           (1)

// LCD controller: ST7262
#define ESP_PANEL_LCD_NAME          ST7262

// LCD resolution - native 800x480
#define ESP_PANEL_LCD_H_RES         (800)
#define ESP_PANEL_LCD_V_RES         (480)

// LCD Bus Settings - RGB interface
#define ESP_PANEL_LCD_BUS_SKIP_INIT_HOST        (1)
#define ESP_PANEL_LCD_BUS_TYPE      (3)  // RGB bus

// RGB Timing Parameters (from Waveshare specs)
#if ESP_PANEL_LCD_BUS_TYPE == 3
    #define ESP_PANEL_LCD_RGB_CLK_HZ            (16 * 1000 * 1000)
    #define ESP_PANEL_LCD_RGB_HPW               (10)
    #define ESP_PANEL_LCD_RGB_HBP               (10)
    #define ESP_PANEL_LCD_RGB_HFP               (20)
    #define ESP_PANEL_LCD_RGB_VPW               (10)
    #define ESP_PANEL_LCD_RGB_VBP               (10)
    #define ESP_PANEL_LCD_RGB_VFP               (10)
    #define ESP_PANEL_LCD_RGB_PCLK_ACTIVE_NEG   (1)
    #define ESP_PANEL_LCD_RGB_DATA_WIDTH        (16)  // RGB565
    
    // RGB GPIO pins for Waveshare ESP32-S3-Touch-LCD-4.3
    #define ESP_PANEL_LCD_RGB_IO_HSYNC          (46)
    #define ESP_PANEL_LCD_RGB_IO_VSYNC          (3)
    #define ESP_PANEL_LCD_RGB_IO_DE             (5)
    #define ESP_PANEL_LCD_RGB_IO_PCLK           (7)
    #define ESP_PANEL_LCD_RGB_IO_DATA0          (14)
    #define ESP_PANEL_LCD_RGB_IO_DATA1          (38)
    #define ESP_PANEL_LCD_RGB_IO_DATA2          (18)
    #define ESP_PANEL_LCD_RGB_IO_DATA3          (17)
    #define ESP_PANEL_LCD_RGB_IO_DATA4          (10)
    #define ESP_PANEL_LCD_RGB_IO_DATA5          (39)
    #define ESP_PANEL_LCD_RGB_IO_DATA6          (0)
    #define ESP_PANEL_LCD_RGB_IO_DATA7          (45)
    #define ESP_PANEL_LCD_RGB_IO_DATA8          (48)
    #define ESP_PANEL_LCD_RGB_IO_DATA9          (47)
    #define ESP_PANEL_LCD_RGB_IO_DATA10         (21)
    #define ESP_PANEL_LCD_RGB_IO_DATA11         (1)
    #define ESP_PANEL_LCD_RGB_IO_DATA12         (2)
    #define ESP_PANEL_LCD_RGB_IO_DATA13         (42)
    #define ESP_PANEL_LCD_RGB_IO_DATA14         (41)
    #define ESP_PANEL_LCD_RGB_IO_DATA15         (40)
    #define ESP_PANEL_LCD_RGB_IO_DISP           (-1)
    
    // 3-wire SPI for RGB LCD configuration
    #if !ESP_PANEL_LCD_BUS_SKIP_INIT_HOST
        #define ESP_PANEL_LCD_SPI_CLK_HZ            (500 * 1000)
        #define ESP_PANEL_LCD_SPI_MODE              (0)
        #define ESP_PANEL_LCD_SPI_CMD_BYTES         (1)
        #define ESP_PANEL_LCD_SPI_PARAM_BITS        (8)
    #endif
    #define ESP_PANEL_LCD_SPI_IO_CS             (6)
    #define ESP_PANEL_LCD_SPI_IO_SCK            (7)
    #define ESP_PANEL_LCD_SPI_IO_MOSI           (6)
    #define ESP_PANEL_LCD_SPI_IO_MISO           (-1)
    #define ESP_PANEL_LCD_SPI_IO_DC             (-1)
#endif

// LCD Color Settings
#define ESP_PANEL_LCD_COLOR_BITS    (16)  // RGB565
#define ESP_PANEL_LCD_COLOR_SPACE   (0)   // RGB
#define ESP_PANEL_LCD_INEVRT_COLOR  (0)

// LCD Transformation Flags - no rotation/mirroring
#define ESP_PANEL_LCD_SWAP_XY       (0)
#define ESP_PANEL_LCD_MIRROR_X      (0)
#define ESP_PANEL_LCD_MIRROR_Y      (0)

// LCD Reset pin
#define ESP_PANEL_LCD_IO_RST        (-1)
#define ESP_PANEL_LCD_RST_LEVEL     (0)

/*-------------------------------- LCD Touch Related --------------------------------*/
#define ESP_PANEL_USE_LCD_TOUCH     (1)

// Touch controller: GT911 (capacitive, I2C)
#define ESP_PANEL_LCD_TOUCH_NAME            GT911

// Touch capabilities
#define ESP_PANEL_TOUCH_MAX_POINTS          5
#define ESP_PANEL_TOUCH_MAX_BUTTONS         0

// Touch resolution matches LCD
#define ESP_PANEL_LCD_TOUCH_H_RES           (ESP_PANEL_LCD_H_RES)
#define ESP_PANEL_LCD_TOUCH_V_RES           (ESP_PANEL_LCD_V_RES)

// Touch Bus Settings - I2C
#define ESP_PANEL_LCD_TOUCH_BUS_TYPE            (0)  // I2C
#define ESP_PANEL_LCD_TOUCH_BUS_SKIP_INIT_HOST  (0)

// I2C configuration for touch
#if !ESP_PANEL_LCD_TOUCH_BUS_SKIP_INIT_HOST
    #define ESP_PANEL_LCD_TOUCH_BUS_HOST_ID     (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_CLK_HZ      (400 * 1000)
    #define ESP_PANEL_LCD_TOUCH_I2C_SCL_PULLUP  (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_SDA_PULLUP  (0)
    #define ESP_PANEL_LCD_TOUCH_I2C_IO_SCL      (9)
    #define ESP_PANEL_LCD_TOUCH_I2C_IO_SDA      (8)
#endif

// Touch Transformation Flags - match LCD orientation
#define ESP_PANEL_LCD_TOUCH_SWAP_XY         (0)
#define ESP_PANEL_LCD_TOUCH_MIRROR_X        (0)
#define ESP_PANEL_LCD_TOUCH_MIRROR_Y        (0)

// Touch pins
#define ESP_PANEL_LCD_TOUCH_IO_RST          (-1)
#define ESP_PANEL_LCD_TOUCH_IO_INT          (-1)
#define ESP_PANEL_LCD_TOUCH_RST_LEVEL       (0)
#define ESP_PANEL_LCD_TOUCH_INT_LEVEL       (0)

/*-------------------------------- Backlight Related --------------------------------*/
#define ESP_PANEL_USE_BL                    (0)  // Controlled via IO expander

/*-------------------------------- Others --------------------------------*/
#define ESP_PANEL_CHECK_RESULT_ASSERT       (0)

#endif /* ESP_PANEL_CONF_H */
