#include "can_manager.h"

#include <driver/twai.h>
#include <freertos/FreeRTOS.h>

#include <algorithm>

CanManager& CanManager::instance() {
    static CanManager manager;
    return manager;
}

bool CanManager::begin(gpio_num_t tx_pin, gpio_num_t rx_pin, std::uint32_t bitrate) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    bitrate_ = bitrate;

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin_, rx_pin_, TWAI_MODE_NORMAL);
    g_config.tx_queue_len = 10;
    g_config.rx_queue_len = 10;
    g_config.alerts_enabled = TWAI_ALERT_NONE;

    if (bitrate_ != 250000) {
        Serial.println("[CanManager] Unsupported bitrate requested. Falling back to 250 kbps.");
    }

    const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("[CanManager] Failed to install TWAI driver");
        ready_ = false;
        return false;
    }

    if (twai_start() != ESP_OK) {
        Serial.println("[CanManager] Failed to start TWAI driver");
        ready_ = false;
        return false;
    }

    ready_ = true;
    Serial.println("[CanManager] TWAI bus ready at 250 kbps");
    return true;
}

bool CanManager::sendButtonAction(const ButtonConfig& button) {
    if (!button.can.enabled) {
        Serial.printf("[CanManager] Button '%s' has no CAN frame assigned\n", button.label.c_str());
        return false;
    }
    return sendFrame(button.can);
}

bool CanManager::sendButtonReleaseAction(const ButtonConfig& button) {
    if (!button.can_off.enabled) {
        Serial.printf("[CanManager] Button '%s' has no CAN OFF frame assigned\n", button.label.c_str());
        return false;
    }
    return sendFrame(button.can_off);
}

bool CanManager::sendFrame(const CanFrameConfig& frame) {
    if (!ready_) {
        Serial.println("[CanManager] TWAI bus not initialized");
        return false;
    }

    // Check for bus errors and recover if needed
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            Serial.println("[CanManager] Bus-off detected, initiating recovery");
            twai_initiate_recovery();
            vTaskDelay(pdMS_TO_TICKS(100));
        } else if (status.state == TWAI_STATE_RECOVERING) {
            Serial.println("[CanManager] Bus is recovering, waiting...");
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    twai_message_t message = {};
    message.identifier = buildIdentifier(frame);
    message.extd = 1;
    message.data_length_code = static_cast<uint8_t>(frame.data.size());
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        message.data[i] = frame.data[i];
    }

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(50));
    if (result != ESP_OK) {
        Serial.printf("[CanManager] Failed to transmit frame (err=%d)\n", static_cast<int>(result));
        // Log the bus state for debugging
        if (twai_get_status_info(&status) == ESP_OK) {
            Serial.printf("[CanManager] Bus state: %d, TX errors: %lu, RX errors: %lu\n",
                         status.state, status.tx_error_counter, status.rx_error_counter);
        }
        return false;
    }

    Serial.printf("[CanManager] Sent PGN 0x%05lX with priority %u\n",
                  static_cast<unsigned long>(frame.pgn), frame.priority);
    return true;
}

std::uint32_t CanManager::buildIdentifier(const CanFrameConfig& frame) const {
    const std::uint8_t priority = frame.priority & 0x7;
    const std::uint8_t data_page = (frame.pgn >> 16) & 0x01;
    const std::uint8_t pdu_format = (frame.pgn >> 8) & 0xFF;
    std::uint8_t pdu_specific = frame.pgn & 0xFF;

    if (pdu_format < 240) {
        // PDU1 - destination specific
        pdu_specific = frame.destination_address;
    }

    return (static_cast<std::uint32_t>(priority) << 26) |
           (static_cast<std::uint32_t>(0) << 25) |
           (static_cast<std::uint32_t>(data_page) << 24) |
           (static_cast<std::uint32_t>(pdu_format) << 16) |
           (static_cast<std::uint32_t>(pdu_specific) << 8) |
           (static_cast<std::uint32_t>(frame.source_address));
}
