/*
 *SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 *SPDX-License-Identifier: MIT
 */

#ifndef __UNIT_BYTE_H
#define __UNIT_BYTE_H

#include "Arduino.h"
#include "Wire.h"

// #define UNIT_BYTE_DEBUG Serial  // This macro definition can be annotated without sending and receiving data prints
//         Define the serial port you want to use, e.g., Serial1 or Serial2
#if defined UNIT_BYTE_DEBUG
#define serialPrint(...)   UNIT_BYTEBUTTON_DEBUG.print(__VA_ARGS__)
#define serialPrintln(...) UNIT_BYTEBUTTON_DEBUG.println(__VA_ARGS__)
#define serialPrintf(...)  UNIT_BYTEBUTTON_DEBUG.printf(__VA_ARGS__)
#define serialFlush()      UNIT_BYTEBUTTON_DEBUG.flush()
#else
#endif

/**
 * @brief Switch status register.
 *
 * This register holds the current status of the switch. It can be used to read
 * the operational state of the unit.
 */
#define UNIT_BYTE_STATUS_REG (0x00)

/**
 * @brief LED brightness control register.
 *
 * This register is used to set or get the brightness level of the switch's LED.
 * Values typically range from 0 (off) to 255 (max brightness).
 */
#define UNIT_BYTE_LED_BRIGHTNESS_REG (0x10)

/**
 * @brief LED show mode register.
 *
 * This register determines the display mode of the LED,  User-defined mode or System default mode
 */
#define UNIT_BYTE_LED_SHOW_MODE_REG (0x19)

/**
 * @brief RGB888 color register.
 *
 * This register allows direct control of the RGB color values in 8-bit per channel format.
 * It supports a wide range of colors by manipulating the red, green, and blue components.
 */
#define UNIT_BYTE_RGB888_REG (0x20)

/**
 * @brief RGB233 color register.
 *
 * This register is used for setting colors in a reduced RGB format (2 bits for red,
 * 3 bits for green, and 3 bits for blue).
 */
#define UNIT_BYTE_RGB233_REG (0x50)

/**
 * @brief 8-byte status register.
 *
 * This register provides an extended status of the switch in an 8-byte format,
 * allowing for more detailed operational data.
 */
#define UNIT_BYTE_STATUS_8BYTE_REG (0x60)

/**
 * @brief System-defined switch 0 RGB888 register.
 *
 * This register holds RGB color settings specifically for system-defined switch 0
 * in 8-bit per channel format.
 */
#define UNIT_BYTE_SYS_DEFINE_SW0_RGB888_REG (0x70)

/**
 * @brief System-defined switch 1 RGB888 register.
 *
 * This register holds RGB color settings specifically for system-defined switch 1
 * in 8-bit per channel format.
 */
#define UNIT_BYTE_SYS_DEFINE_SW1_RGB888_REG (0x90)

/**
 * @brief Flash write register.
 *
 * This register is used for writing back data to the flash memory, typically for
 * saving configuration changes or states.
 */
#define UNIT_BYTE_FLASH_WRITE_REG (0xF0)

/**
 * @brief Interrupt request setup register.
 *
 * This register configures the interrupt settings for the switch, allowing for
 * specific actions based on external or internal events.
 */
#define UNIT_BYTE_IRQ_SETUP_REG (0xF1)

/**
 * @brief Firmware version register.
 *
 * This register contains the current firmware version of the switch, useful for
 * debugging or checking for updates.
 */
#define UNIT_BYTE_FIRMWARE_VERSION_REG (0xFE)

/**
 * @brief I2C address register.
 *
 * This register defines the I2C address for the device. It is necessary for communication
 * with the device over the I2C bus.
 */
#define UNIT_BYTE_I2C_ADDRESS_REG (0xFF)

typedef enum {
    BYTE_LED_USER_DEFINED = 0,  // User-defined mode
    BYTE_LED_MODE_DEFAULT       // System default mode
} byte_led_t;

class UnitByte {
public:
    /**
     * @brief Initializes the device with optional I2C settings.
     *
     * This function configures the I2C communication settings, allowing the user to specify
     * custom SDA and SCL pins as well as the I2C speed. If no parameters are provided, default values are used.
     *
     * @param wire   Pointer to the TwoWire object for I2C communication (default is &Wire).
     * @param sda    The SDA pin number (default is -1, meaning use the default).
     * @param scl    The SCL pin number (default is -1, meaning use the default).
     * @param speed  The I2C bus speed in Hz (default is 4000000L).
     * @return True if initialization was successful, false otherwise.
     */
    bool begin(TwoWire* wire = &Wire, uint8_t addr = -1, uint8_t sda = -1, uint8_t scl = -1, uint32_t speed = 4000000L);

    /**
     * @brief Sets the brightness of a specified LED.
     *
     * This function controls the brightness of an LED at the given address and index.
     *
     * @param num        The index of the LED to control.
     * @param brightness The brightness level (0-255).
     */
    void setLEDBrightness(uint8_t num, uint8_t brightness);

    /**
     * @brief Sets the RGB888 color for switch 0.
     *
     * This function sets the RGB color value for switch 0 at the given address and index.
     *
     * @param num    The index of the switch (0 for SW0).
     * @param color  The RGB888 color value as a 32-bit integer.
     */
    void setSwitchOffRGB888(uint8_t num, uint32_t color);

    /**
     * @brief Sets the RGB888 color for switch 1.
     *
     * This function sets the RGB color value for switch 1 at the given address and index.
     *
     * @param num    The index of the switch (1 for SW1).
     * @param color  The RGB888 color value as a 32-bit integer.
     */
    void setSwitchOnRGB888(uint8_t num, uint32_t color);

    /**
     * @brief Sets the RGB888 color for a specified LED.
     *
     * This function sets the RGB color value for a generic LED at the given address and index.
     *
     * @param num    The index of the LED.
     * @param color  The RGB888 color value as a 32-bit integer.
     */
    void setRGB888(uint8_t num, uint32_t color);

    /**
     * @brief Sets the RGB233 color for a specified LED.
     *
     * This function sets the RGB233 color value for a generic LED at the given address and index.
     *
     * @param num    The index of the LED.
     * @param color  The RGB888 color value as a 32-bit integer (to be converted).
     */
    void setRGB233(uint8_t num, uint32_t color);

    /**
     * @brief Enables or disables IRQ functionality.
     *
     * This function sets the IRQ enable status at the given address.
     *
     * @param en   True to enable IRQ, false to disable it.
     */
    void setIRQEnable(bool en);

    /**
     * @brief Sets the LED show mode.
     *
     * This function defines the display mode for the LEDs at the specified address.
     *
     * @param mode The desired LED show mode of type byte_led_t.
     */
    void setLEDShowMode(byte_led_t mode);

    /**
     * @brief Initiates flash write-back operation.
     *
     * This function signals the device to perform a write-back operation to flash memory.
     *
     */
    void setFlashWriteBack();

    /**
     * @brief Sets a new I2C address for the device.
     *
     * This function changes the current I2C address of the device to a new specified ID.
     *
     * @param newAddr   The new I2C address to be set.1-127
     */
    uint8_t setI2CAddress(uint8_t newAddr);

    /**
     * @brief Retrieves the current status of the switch.
     *
     * This function reads and returns the status of all switches.
     *
     * @return The current status of the switch.
     */
    uint8_t getSwitchStatus();

    /**
     * @brief Retrieves the 8-byte status of the switch.
     *
     * This function returns a specific byte of the 8-byte status data from the switch.
     *
     * @param num  The num of the byte to retrieve (0-7).
     * @return The requested byte of switch status.
     */
    uint8_t getSwitchStatus(uint8_t num);

    /**
     * @brief Retrieves the brightness of the LED.
     *
     * This function reads and returns the current brightness level of the LED at the specified address.
     *
     * @return The brightness level of the LED (0-255).
     */
    uint8_t getLEDBrightness();

    /**
     * @brief Checks if IRQ functionality is enabled.
     *
     * This function returns the IRQ enable status at the given address.
     *
     * @return True if IRQ is enabled, false otherwise.
     */
    uint8_t getIRQEnable();

    /**
     * @brief Retrieves the firmware version of the device.
     *
     * This function reads and returns the firmware version number from the specified address.
     *
     * @return The firmware version number.
     */
    uint8_t getFirmwareVersion();

    /**
     * @brief Gets the current I2C address of the device.
     *
     * This function retrieves the I2C address currently set for the device.
     *
     * @return The current I2C address.
     */
    uint8_t getI2CAddress();

    /**
     * @brief Retrieves the LED show mode.
     *
     * This function reads and returns the current LED show mode at the specified address.
     *
     * @return The current LED show mode.
     */
    uint8_t getLEDShowMode();

    /**
     * @brief Gets the RGB888 color value for switch 0.
     *
     * This function retrieves the RGB888 color value for switch 0 at the given address and index.
     *
     * @param num  The index of the switch (0 for SW0).
     * @return The RGB888 color value as a 32-bit integer.
     */
    uint32_t getSwitchOffRGB888(uint8_t num);

    /**
     * @brief Gets the RGB888 color value for switch 1.
     *
     * This function retrieves the RGB888 color value for switch 1 at the given address and index.
     *
     * @param num  The index of the switch (1 for SW1).
     * @return The RGB888 color value as a 32-bit integer.
     */
    uint32_t getSwitchOnRGB888(uint8_t num);

    /**
     * @brief Gets the RGB888 color value for a specified LED.
     *
     * This function retrieves the RGB888 color value for an LED at the given address and index.
     *
     * @param num  The index of the LED.
     * @return The RGB888 color value as a 32-bit integer.
     */
    uint32_t getRGB888(uint8_t num);

    /**
     * @brief Gets the RGB233 color value for a specified LED.
     *
     * This function retrieves the RGB233 color value for an LED at the given I2C address and index.
     * The RGB233 format uses 2 bits for red, 3 bits for green, and 3 bits for blue, resulting in a 32-bit integer
     * where the lower 8 bits are typically unused.
     *
     * @param num  The index of the LED (0-based).
     * @return The RGB233 color value as a 32-bit integer.
     */
    uint32_t getRGB233(uint8_t num);

private:
    TwoWire* _wire;
    uint8_t _addr;
    uint8_t _scl;
    uint8_t _sda;
    uint32_t _speed;

    /**
     * @brief Writes multiple bytes to a specified register.
     *
     * This function writes a sequence of bytes from the provided buffer
     * to the device located at the specified I2C address and register.
     *
     * @param addr   The I2C address of the device.
     * @param reg    The register address where the data will be written.
     * @param buffer A pointer to the data buffer that contains the bytes to be written.
     * @param length The number of bytes to write from the buffer.
     */
    void writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);

    /**
     * @brief Reads multiple bytes from a specified register.
     *
     * This function reads a sequence of bytes from the device located at
     * the specified I2C address and register into the provided buffer.
     *
     * @param addr   The I2C address of the device.
     * @param reg    The register address from which the data will be read.
     * @param buffer A pointer to the data buffer where the read bytes will be stored.
     * @param length The number of bytes to read into the buffer.
     */
    void readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);

    /**
     * @brief Converts an RGB888 color value to RGB233 format.
     *
     * This function takes a 24-bit RGB888 color value and converts it
     * to a 8-bit RGB233 representation (2 bits for red, 3 bits for green,
     * and 3 bits for blue).
     *
     * @param color The RGB888 color value as a 32-bit integer.
     * @return The converted RGB233 color value as an 8-bit integer.
     */
    uint8_t rgb888ToRgb233(uint32_t color);

    /**
     * @brief Acquires a mutex lock.
     *
     * This function attempts to acquire a mutex lock to ensure thread-safe access
     * to shared resources. It should be paired with a corresponding call to
     * releaseMutex() to prevent deadlocks.
     */
    void acquireMutex();

    /**
     * @brief Releases a mutex lock.
     *
     * This function releases a previously acquired mutex lock, allowing other
     * threads to access shared resources. It should only be called after
     * successfully acquiring the mutex with acquireMutex().
     */
    void releaseMutex();
};

#endif
