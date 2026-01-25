#pragma once

#include <Arduino.h>
#include <hal/gpio_types.h>

#include "config_types.h"

class CanManager {
public:
    static CanManager& instance();

    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(15);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(4);

    bool begin(gpio_num_t tx_pin = DEFAULT_TX_PIN, gpio_num_t rx_pin = DEFAULT_RX_PIN, std::uint32_t bitrate = 250000);
    bool sendButtonAction(const ButtonConfig& button);
    bool sendButtonReleaseAction(const ButtonConfig& button);
    bool sendFrame(const CanFrameConfig& frame);

    bool isReady() const { return ready_; }

private:
    CanManager() = default;

    bool ready_ = false;
    gpio_num_t tx_pin_ = DEFAULT_TX_PIN;
    gpio_num_t rx_pin_ = DEFAULT_RX_PIN;
    std::uint32_t bitrate_ = 250000;

    std::uint32_t buildIdentifier(const CanFrameConfig& frame) const;
};
