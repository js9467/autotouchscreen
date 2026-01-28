/**
 * POWERCELL NGX - Simple CAN Communication Test
 * 
 * This is a minimal test program to establish CAN communication
 * with the Infinitybox POWERCELL NGX.
 * 
 * Hardware: Waveshare ESP32-S3-Touch-LCD-7
 * CAN Pins: TX=GPIO20, RX=GPIO19
 * Bitrate: 250 kbps (J1939 default)
 */

#include <Arduino.h>
#include "driver/twai.h"

// CAN Hardware Configuration
#define CAN_TX_PIN GPIO_NUM_20
#define CAN_RX_PIN GPIO_NUM_19
#define CAN_BITRATE 250000  // 250 kbps - POWERCELL NGX default

// POWERCELL NGX Configuration
#define POWERCELL_ADDRESS 1  // Cell address (1-16)
#define SOURCE_ADDRESS 0x63  // Our source address (required: 0x63)

// Calculate CAN IDs for POWERCELL at address 1
// Config message: FF41 (FF4 + cell address)
// Response message: FF51 (FF5 + cell address)
#define CONFIG_CAN_ID  ((0x18FF4100 | (POWERCELL_ADDRESS)) | (SOURCE_ADDRESS << 8))
#define RESPONSE_CAN_ID (0x18FF5100 | (POWERCELL_ADDRESS))

// Timing
#define SEND_INTERVAL 5000  // Send configuration query every 5 seconds

bool can_initialized = false;
unsigned long last_send = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=== POWERCELL NGX CAN Communication Test ===");
    Serial.println("Hardware: ESP32-S3-Touch-LCD-7");
    Serial.printf("CAN TX Pin: GPIO%d\n", CAN_TX_PIN);
    Serial.printf("CAN RX Pin: GPIO%d\n", CAN_RX_PIN);
    Serial.printf("Bitrate: %d bps\n", CAN_BITRATE);
    Serial.printf("POWERCELL Address: %d\n", POWERCELL_ADDRESS);
    Serial.println("==========================================\n");
    
    // Initialize TWAI (CAN) driver
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Serial.println("✓ CAN driver installed");
    } else {
        Serial.println("✗ Failed to install CAN driver");
        return;
    }
    
    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        Serial.println("✓ CAN driver started");
        can_initialized = true;
    } else {
        Serial.println("✗ Failed to start CAN driver");
        return;
    }
    
    Serial.println("\n✓ CAN initialized successfully!");
    Serial.println("Listening for POWERCELL responses...\n");
}

void sendPollRequest() {
    if (!can_initialized) return;
    
    // Poll message: Byte 1 = 0x11, rest = 0x00
    twai_message_t message;
    message.identifier = CONFIG_CAN_ID;
    message.data_length_code = 8;
    message.extd = 1;  // Extended ID
    message.rtr = 0;
    message.ss = 0;
    message.self = 0;
    message.dlc_non_comp = 0;
    
    message.data[0] = 0x11;  // Poll request command
    message.data[1] = 0x00;
    message.data[2] = 0x00;
    message.data[3] = 0x00;
    message.data[4] = 0x00;
    message.data[5] = 0x00;
    message.data[6] = 0x00;
    message.data[7] = 0x00;
    
    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        Serial.println("→ Sent poll request to POWERCELL");
        Serial.printf("  CAN ID: 0x%08X\n", message.identifier);
        Serial.print("  Data: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%02X ", message.data[i]);
        }
        Serial.println();
    } else {
        Serial.println("✗ Failed to send message");
    }
}

void sendConfigurationMessage() {
    if (!can_initialized) return;
    
    /**
     * Configuration Example:
     * - J1939 Data Rate: 250 kb/s (bits 7-6 = 00)
     * - Loss-of-Com Timer: 10 seconds (bits 5-4 = 00)
     * - Reporting Timer: 250 ms (bits 3-2 = 00)
     * - PWM Frequency: 200 Hz (bits 1-0 = 01)
     * - All outputs: Maintain State on loss of com (00 for each)
     * - Custom config number: 1
     */
    
    twai_message_t message;
    message.identifier = CONFIG_CAN_ID;
    message.data_length_code = 8;
    message.extd = 1;
    message.rtr = 0;
    message.ss = 0;
    message.self = 0;
    message.dlc_non_comp = 0;
    
    message.data[0] = 0x99;  // Configuration confirmation bits (required)
    message.data[1] = 0x01;  // Data rate=250k, LOC=10s, Report=250ms, PWM=200Hz
    message.data[2] = 0x00;  // LOC behavior: outputs 1-4 maintain state
    message.data[3] = 0x00;  // LOC behavior: outputs 5-8 maintain state
    message.data[4] = 0x00;  // LOC behavior: outputs 9-10 maintain state
    message.data[5] = 0x00;  // Reserved
    message.data[6] = 0x00;  // Reserved
    message.data[7] = 0x01;  // User configuration number
    
    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        Serial.println("→ Sent configuration to POWERCELL");
        Serial.printf("  CAN ID: 0x%08X\n", message.identifier);
        Serial.print("  Data: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%02X ", message.data[i]);
        }
        Serial.println();
        Serial.println("  NOTE: Power cycle POWERCELL to apply changes!");
    } else {
        Serial.println("✗ Failed to send configuration");
    }
}

void checkForMessages() {
    if (!can_initialized) return;
    
    twai_message_t message;
    
    // Check for received messages (non-blocking)
    while (twai_receive(&message, 0) == ESP_OK) {
        Serial.println("\n← Received CAN message:");
        Serial.printf("  CAN ID: 0x%08X\n", message.identifier);
        Serial.printf("  Length: %d bytes\n", message.data_length_code);
        Serial.print("  Data: ");
        for (int i = 0; i < message.data_length_code; i++) {
            Serial.printf("%02X ", message.data[i]);
        }
        Serial.println();
        
        // Check if this is a POWERCELL response
        if ((message.identifier & 0xFFFF0000) == (RESPONSE_CAN_ID & 0xFFFF0000)) {
            Serial.println("  ★ POWERCELL RESPONSE DETECTED!");
            
            if (message.data_length_code >= 8) {
                Serial.printf("  Major Version: %d\n", message.data[0]);
                Serial.printf("  Minor Version: %d\n", message.data[1]);
                Serial.printf("  ADC Source: %d\n", message.data[2]);
                Serial.printf("  User Config: %d\n", message.data[3]);
                
                // Decode configuration
                uint8_t config = message.data[4];
                const char* dataRate[] = {"250 kbps", "500 kbps", "1 Mbps", "Invalid"};
                const char* locTimer[] = {"10s", "20s", "30s", "60s"};
                const char* reportTimer[] = {"250ms", "500ms", "1000ms", "Invalid"};
                const char* pwmFreq[] = {"Invalid", "200Hz", "20kHz", "Invalid"};
                
                Serial.printf("  Data Rate: %s\n", dataRate[(config >> 6) & 0x03]);
                Serial.printf("  LOC Timer: %s\n", locTimer[(config >> 4) & 0x03]);
                Serial.printf("  Report Timer: %s\n", reportTimer[(config >> 2) & 0x03]);
                Serial.printf("  PWM Freq: %s\n", pwmFreq[config & 0x03]);
            }
        }
        Serial.println();
    }
}

void loop() {
    if (!can_initialized) {
        delay(1000);
        return;
    }
    
    // Check for incoming messages continuously
    checkForMessages();
    
    // Send poll request periodically
    if (millis() - last_send > SEND_INTERVAL) {
        sendPollRequest();
        last_send = millis();
    }
    
    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        while(Serial.available()) Serial.read(); // Clear buffer
        
        switch(cmd) {
            case 'p':
            case 'P':
                Serial.println("\n--- Manual Poll Request ---");
                sendPollRequest();
                break;
                
            case 'c':
            case 'C':
                Serial.println("\n--- Sending Configuration ---");
                sendConfigurationMessage();
                break;
                
            case 'h':
            case 'H':
            case '?':
                Serial.println("\n=== Commands ===");
                Serial.println("P - Send poll request");
                Serial.println("C - Send configuration");
                Serial.println("H - Show this help");
                Serial.println("================\n");
                break;
        }
    }
    
    delay(10);
}
