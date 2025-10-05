/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _UNIT_STEP16_HPP_
#define _UNIT_STEP16_HPP_

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Encoder value register (0x0-0xF)
 */
#define UNIT_STEP16_VALUE_REG (0x00)

/**
 * @brief LED configuration register
 */
#define UNIT_STEP16_LED_CONFIG_REG (0x10)

/**
 * @brief LED brightness register (0-100)
 */
#define UNIT_STEP16_LED_BRIGHTNESS_REG (0x20)

/**
 * @brief Rotation direction register
 */
#define UNIT_STEP16_SWITCH_REG (0x30)

/**
 * @brief RGB on/off configuration
 */
#define UNIT_STEP16_RGB_CONFIG_REG (0x40)

/**
 * @brief RGB brightness register (0-100)
 */
#define UNIT_STEP16_RGB_BRIGHTNESS_REG (0x41)

/**
 * @brief RGB color value register (3 bytes)
 */
#define UNIT_STEP16_RGB_VALUE_REG (0x50)

/**
 * @brief Red color value register
 */
#define UNIT_STEP16_R_VALUE_REG (0x50)

/**
 * @brief Green color value register
 */
#define UNIT_STEP16_G_VALUE_REG (0x51)

/**
 * @brief Blue color value register
 */
#define UNIT_STEP16_B_VALUE_REG (0x52)

/**
 * @brief Save configuration to flash
 */
#define UNIT_STEP16_SAVE_FLASH_REG (0xF0)

/**
 * @brief Firmware version register
 */
#define UNIT_STEP16_VERSION_REG (0xFE)

/**
 * @brief Device I2C address register
 */
#define UNIT_STEP16_ADDRESS_REG (0xFF)

/**
 * @brief LED off
 */
#define UNIT_STEP16_LED_CONFIG_OFF (0x00)

/**
 * @brief LED on
 */
#define UNIT_STEP16_LED_CONFIG_ON (0xFF)

/**
 * @brief LED default configuration
 */
#define UNIT_STEP16_LED_CONFIG_DEFAULT (0xFE)

/**
 * @brief LED brightness minimum
 */
#define UNIT_STEP16_LED_BRIGHTNESS_MIN (0x00)

/**
 * @brief LED brightness maximum
 */
#define UNIT_STEP16_LED_BRIGHTNESS_MAX (0x64)

/**
 * @brief LED brightness default
 */
#define UNIT_STEP16_LED_BRIGHTNESS_DEFAULT (0x32)

/**
 * @brief Clockwise
 */
#define UNIT_STEP16_SWITCH_CLOCKWISE (0x01)

/**
 * @brief Counterclockwise
 */
#define UNIT_STEP16_SWITCH_COUNTERCLOCKWISE (0x00)

/**
 * @brief Default switch state
 */
#define UNIT_STEP16_SWITCH_DEFAULT (UNIT_STEP16_SWITCH_CLOCKWISE)

/**
 * @brief RGB off
 */
#define UNIT_STEP16_RGB_CONFIG_OFF (0x00)

/**
 * @brief RGB on
 */
#define UNIT_STEP16_RGB_CONFIG_ON (0x01)

/**
 * @brief RGB default configuration
 */
#define UNIT_STEP16_RGB_CONFIG_DEFAULT (UNIT_STEP16_RGB_CONFIG_ON)

/**
 * @brief RGB brightness minimum
 */
#define UNIT_STEP16_RGB_BRIGHTNESS_MIN (0x00)

/**
 * @brief RGB brightness maximum
 */
#define UNIT_STEP16_RGB_BRIGHTNESS_MAX (0x64)

/**
 * @brief RGB brightness default
 */
#define UNIT_STEP16_RGB_BRIGHTNESS_DEFAULT (0x32)

/**
 * @brief Red color value default
 */
#define UNIT_STEP16_R_VALUE_DEFAULT (0x00)

/**
 * @brief Green color value default
 */
#define UNIT_STEP16_G_VALUE_DEFAULT (0x00)

/**
 * @brief Blue color value default
 */
#define UNIT_STEP16_B_VALUE_DEFAULT (0x00)

/**
 * @brief Save LED configuration
 */
#define UNIT_STEP16_SAVE_LED_CONFIG (0x01)

/**
 * @brief Save RGB configuration
 */
#define UNIT_STEP16_SAVE_RGB_CONFIG (0x02)

/**
 * @brief Default I2C address
 */
#define UNIT_STEP16_DEFAULT_I2C_ADDRESS (0x48)

/**
 * @class UnitStep16
 * @brief Driver class for M5Unit-Step16 rotary encoder module
 *
 * @note This class provides an interface to control the M5Unit-Step16 module,
 * which features a 16-position rotary encoder with LED display and RGB lighting.
 * Communication is done via I2C protocol.
 */
class UnitStep16 {
public:
    /**
     * @brief Construct a new UnitStep16 object
     * @param i2c_addr I2C address of the device (default: 0x48)
     * @param wire Pointer to TwoWire instance (default: &Wire)
     */
    UnitStep16(uint8_t i2c_addr = UNIT_STEP16_DEFAULT_I2C_ADDRESS, TwoWire *wire = &Wire);

    /**
     * @brief Initialize the UnitStep16
     * @return true if the initialization is successful, false otherwise
     */
    bool begin();

    /**
     * @brief Get the current encoder value
     * @return Encoder value (0x0-0xF)
     */
    uint8_t getValue();

    // LED control functions
    /**
     * @brief Set LED display configuration
     * @param config LED configuration value:
     *               - 0x00: Always off
     *               - 0x01-0xFE: Screen off time in seconds
     *               - 0xFF: Always on
     * @return true on success, false on failure
     */
    bool setLedConfig(uint8_t config);

    /**
     * @brief Get current LED configuration
     * @return LED configuration value
     */
    uint8_t getLedConfig();

    /**
     * @brief Set LED brightness
     * @param brightness Brightness value (0-100)
     * @return true on success, false on failure
     */
    bool setLedBrightness(uint8_t brightness);

    /**
     * @brief Get current LED brightness
     * @return LED brightness value (0-100)
     */
    uint8_t getLedBrightness();

    // Rotation direction control
    /**
     * @brief Set encoder rotation direction
     * @param state Direction state:
     *              - 0: Counterclockwise
     *              - 1: Clockwise
     * @return true on success, false on failure
     */
    bool setSwitchState(uint8_t state);

    /**
     * @brief Get current rotation direction
     * @return Direction state (0: counterclockwise, 1: clockwise)
     */
    uint8_t getSwitchState();

    // RGB control functions
    /**
     * @brief Set RGB LED on/off state
     * @param config RGB configuration:
     *               - 0: Off
     *               - 1: On
     * @return true on success, false on failure
     */
    bool setRgbConfig(uint8_t config);

    /**
     * @brief Get current RGB configuration
     * @return RGB configuration (0: off, 1: on)
     */
    uint8_t getRgbConfig();

    /**
     * @brief Set RGB LED brightness
     * @param brightness Brightness value (0-100)
     * @return true on success, false on failure
     */
    bool setRgbBrightness(uint8_t brightness);

    /**
     * @brief Get current RGB brightness
     * @return RGB brightness value (0-100)
     */
    uint8_t getRgbBrightness();

    /**
     * @brief Set RGB LED color
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     * @return true on success, false on failure
     */
    bool setRgb(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Get current RGB color values
     * @param r Pointer to store red value
     * @param g Pointer to store green value
     * @param b Pointer to store blue value
     * @return true on success, false on failure
     */
    bool getRgb(uint8_t *r, uint8_t *g, uint8_t *b);

    /**
     * @brief Set red color value only
     * @param r Red value (0-255)
     * @return true on success, false on failure
     */
    bool setRValue(uint8_t r);

    /**
     * @brief Set green color value only
     * @param g Green value (0-255)
     * @return true on success, false on failure
     */
    bool setGValue(uint8_t g);

    /**
     * @brief Set blue color value only
     * @param b Blue value (0-255)
     * @return true on success, false on failure
     */
    bool setBValue(uint8_t b);

    // Flash storage
    /**
     * @brief Save configuration to flash memory, it will cost almost 50ms to write to flash
     * @param save Save option:
     *             - 1: Save LED configuration
     *             - 2: Save RGB configuration
     * @return true on success, false on failure
     */
    bool saveToFlash(uint8_t save);

    // Device information
    /**
     * @brief Get firmware version
     * @return Firmware version number
     */
    uint8_t getVersion();

    /**
     * @brief Get current device I2C address
     * @return Current I2C address
     */
    uint8_t getAddress();

    /**
     * @brief Set new device I2C address, it will cost almost 50ms to write to flash
     * @param address New I2C address
     * @return true on success, false on failure
     * @note The device will need to be re-initialized with the new address
     */
    bool setAddress(uint8_t address);

    /**
     * @brief Reset all settings to default values
     * @return true on success, false on failure
     * @note Default settings: LED=0xFE, LED brightness=50,
     *       Switch=1, RGB=on, RGB brightness=50, RGB color=black
     */
    bool setDefaultConfig();

    /**
     * @brief Print all register values to Serial (for debugging)
     * @note Serial must be initialized before calling this function
     */
    void printRegisters();

private:
    uint8_t _i2c_addr;  // I2C address of the device
    TwoWire *_wire;     // Pointer to the I2C interface

    /**
     * @brief Read data from register
     * @param reg_addr Register address to read from
     * @param data Pointer to buffer to store read data
     * @param len Number of bytes to read
     * @return 0 on success, -1 on failure
     */
    int readRegister(uint8_t reg_addr, uint8_t *data, uint8_t len);

    /**
     * @brief Write data to register
     * @param reg_addr Register address to write to
     * @param data Pointer to data to write
     * @param len Number of bytes to write
     * @return 0 on success, -1 on failure
     */
    int writeRegister(uint8_t reg_addr, uint8_t *data, uint8_t len);
};

#endif  // UNIT_STEP16_HPP
