# Web-Based Configuration System

## Overview

Your Bronco Controls device now has a complete web-based configuration system that allows you to:

- ✅ Configure pages, buttons, colors, and text via web interface
- ✅ Upload and manage custom icons and images
- ✅ Add, delete, and modify pages dynamically
- ✅ Assign J1939 CAN frames to any button
- ✅ Export and import configurations as JSON files
- ✅ Connect via WiFi (AP or Station mode)
- ✅ Real-time UI updates when configuration changes

## Getting Started

### 1. Initial Setup

When you first power on the device, it will:
- Create a WiFi Access Point (AP) named `BroncoControls`
- Password: `bronco123`
- IP Address: `192.168.4.250`
- Touchscreen header shows both the AP IP (always 192.168.4.250) and, once joined to another network, the DHCP LAN IP so you can find it without a laptop.

### 2. Connecting to the Web Interface

1. Connect your phone/laptop to the `BroncoControls` WiFi network
2. Open a web browser and go to: `http://192.168.4.250`
3. You'll see the Bronco Controls configuration interface

### 3. Web Interface Features

#### Pages Tab
- **View All Pages**: See all configured pages
- **Add New Page**: Click "+ Add New Page" to create a page
- **Edit Page**: Click on any page to edit its settings and buttons
- **Page Settings**: Configure name, background color, header visibility
- **Add Buttons**: Add buttons to the page with custom positions and colors
- **Delete Page**: Remove pages you don't need

#### Device Settings Tab
- **Device Name**: Change the display name
- **Header Colors**: Customize header bar colors
- **WiFi Settings**: 
  - Switch between AP mode and Station mode
  - Configure WiFi credentials
  - Connect to your existing WiFi network
  - Tap "Scan Networks" to discover nearby SSIDs and auto-fill the join form before pushing credentials
  - Use the "Join Home WiFi" card to quickly push new STA credentials while the AP stays online

#### System Tab
- **System Information**: View memory, storage, and WiFi status
- **Export Configuration**: Download your configuration as JSON
- **Import Configuration**: Upload a saved configuration
- **Reboot Device**: Restart the device

### 4. Configuring Buttons

When adding or editing a button, you can configure:

**Visual Settings:**
- Text label
- Button color
- Text color
- Position (X, Y coordinates)
- Size (Width, Height)

**J1939 CAN Frame:**
- Enable/Disable CAN transmission
- PGN (Parameter Group Number)
- Priority (0-7)
- Source Address
- Destination Address (255 = broadcast)
- Data Length (0-8 bytes)
- Data Bytes (8 bytes in hex format)

### 5. J1939 CAN Communication

The device supports full J1939 CAN communication at 250 kbps. Configure CAN pins in `can_manager.h`:

```cpp
#define CAN_TX_PIN GPIO_NUM_21
#define CAN_RX_PIN GPIO_NUM_22
```

When a button is pressed, if a CAN frame is configured and enabled, it will be transmitted on the CAN bus.

## File Structure

```
src/
├── main_new.cpp          - Main program with web configuration
├── config_manager.h/cpp  - Configuration storage and management
├── web_server.h/cpp      - REST API web server
├── web_interface.h       - HTML/CSS/JavaScript web UI
├── ui_builder.h/cpp      - Dynamic LVGL UI generation
├── can_manager.h/cpp     - J1939 CAN bus communication
└── [original files...]
```

## Switching to Web Configuration

To use the new web-based system:

1. Backup your current `main.cpp`:
   ```bash
   mv src/main.cpp src/main_old.cpp
   ```

2. Rename the new main file:
   ```bash
   mv src/main_new.cpp src/main.cpp
   ```

3. Build and upload:
   ```bash
   pio run --target upload
   ```

## API Endpoints

The device provides a REST API for programmatic access:

### Pages
- `GET /api/pages` - List all pages
- `GET /api/pages/{index}` - Get page details
- `POST /api/pages` - Add new page
- `PUT /api/pages/{index}` - Update page
- `DELETE /api/pages/{index}` - Delete page

### Buttons
- `POST /api/pages/{pageIndex}/buttons` - Add button
- `PUT /api/pages/{pageIndex}/buttons/{buttonIndex}` - Update button
- `DELETE /api/pages/{pageIndex}/buttons/{buttonIndex}` - Delete button

### Images
- `POST /api/images` - Upload image
- `GET /api/images` - List images
- `DELETE /api/images/{filename}` - Delete image

### Configuration
- `GET /api/config` - Get full configuration
- `POST /api/config` - Update configuration
- `GET /api/export` - Export configuration as JSON
- `POST /api/import` - Import configuration from JSON

### System
- `GET /api/system` - System information
- `POST /api/reboot` - Reboot device

## Configuration File Format

Configuration is stored in `/config.json` on the device's LittleFS filesystem. Example:

```json
{
  "device_name": "Bronco Controls",
  "wifi_ap_mode": true,
  "ap_ssid": "BroncoControls",
  "ap_password": "bronco123",
  "header_color": 1668818,
  "header_text_color": 16777215,
  "page_count": 1,
  "home_page_index": 0,
  "pages": [
    {
      "name": "Home",
      "bg_color": 0,
      "show_header": true,
      "button_count": 2,
      "buttons": [
        {
          "text": "Windows",
          "color": 2196243,
          "text_color": 16777215,
          "x": 10,
          "y": 80,
          "width": 200,
          "height": 100,
          "can_frame": {
            "enabled": true,
            "pgn": 61444,
            "priority": 6,
            "source_addr": 0,
            "dest_addr": 255,
            "data_length": 8,
            "data": [1, 2, 3, 4, 5, 6, 7, 8]
          }
        }
      ]
    }
  ]
}
```

## Troubleshooting

### Can't connect to WiFi AP
- Make sure you're connecting to the right SSID
- Try forgetting the network and reconnecting
- Check that WiFi is enabled on your device

### Web interface not loading
- Verify IP address (should be 192.168.4.250 in AP mode)
- If already joined to your house WiFi, read the LAN IP from the on-screen network badges
- Check serial monitor for actual IP address
- Try accessing via `http://` (not `https://`)

### CAN frames not sending
- Verify CAN pins are correctly configured
- Check CAN bus termination resistors
- Use serial monitor to see CAN transmission logs
- Verify CAN transceiver hardware is connected

### UI not updating after configuration change
- The UI should automatically rebuild when config changes
- If not, try rebooting the device
- Check serial monitor for error messages

## Memory Requirements

- Flash: ~2MB for code + assets
- Heap: ~200KB minimum free recommended
- LittleFS: ~1MB for configuration and images
- PSRAM: Required for LVGL buffers

## Future Enhancements

Potential additions you could make:
- Image upload for button icons
- Background images for pages
- Touch gestures (swipe between pages)
- OTA (Over-The-Air) firmware updates
- Password protection for web interface
- CAN bus monitoring and diagnostics
- Page templates and themes
- Multi-language support

## Support

For questions or issues:
1. Check the serial monitor for debug messages
2. Review the configuration JSON file
3. Try resetting to default configuration
4. Check the source code comments for details

## License

Same as the original project.
