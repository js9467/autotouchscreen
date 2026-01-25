#pragma once

#include <Arduino.h>
#include <hal/gpio_types.h>
#include <vector>

#include "config_types.h"

// Struct for received CAN messages (different from CanMessage in config_types.h)
struct CanRxMessage {
    uint32_t identifier;
    uint8_t data[8];
    uint8_t length;
    uint32_t timestamp;
};

class CanManager {
public:
    static CanManager& instance();

    // Per Waveshare official documentation and demo code
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(19);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(20);

    bool begin(gpio_num_t tx_pin = DEFAULT_TX_PIN, gpio_num_t rx_pin = DEFAULT_RX_PIN, std::uint32_t bitrate = 250000);
    void stop();
    bool sendButtonAction(const ButtonConfig& button);
    bool sendButtonReleaseAction(const ButtonConfig& button);
    bool sendFrame(const CanFrameConfig& frame);
    
    bool receiveMessage(CanRxMessage& msg, uint32_t timeout_ms = 10);
    std::vector<CanRxMessage> receiveAll(uint32_t timeout_ms = 100);

    bool isReady() const { return ready_; }

private:
    CanManager() = default;

    bool ready_ = false;
    gpio_num_t tx_pin_ = DEFAULT_TX_PIN;
    gpio_num_t rx_pin_ = DEFAULT_RX_PIN;
    std::uint32_t bitrate_ = 250000;

    std::uint32_t buildIdentifier(const CanFrameConfig& frame) const;
};
