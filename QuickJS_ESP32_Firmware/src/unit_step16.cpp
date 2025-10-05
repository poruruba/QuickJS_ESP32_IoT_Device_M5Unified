/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "unit_step16.hpp"

UnitStep16::UnitStep16(uint8_t i2c_addr, TwoWire *wire)
{
    _i2c_addr = i2c_addr;
    _wire     = wire;
}

bool UnitStep16::begin()
{
    _wire->begin();

    _wire->beginTransmission(_i2c_addr);
    if (_wire->endTransmission() != 0) {
        return false;
    }

    return true;
}

int UnitStep16::readRegister(uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg_addr);
    if (_wire->endTransmission() != 0) {
        return -1;
    }

    _wire->requestFrom(_i2c_addr, len);

    for (uint8_t i = 0; i < len; i++) {
        if (_wire->available()) {
            data[i] = _wire->read();
        } else {
            return -1;
        }
    }

    return 0;
}

int UnitStep16::writeRegister(uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg_addr);

    for (uint8_t i = 0; i < len; i++) {
        _wire->write(data[i]);
    }

    if (_wire->endTransmission() != 0) {
        return -1;
    }

    return 0;
}

uint8_t UnitStep16::getValue()
{
    uint8_t value = 0;
    readRegister(UNIT_STEP16_VALUE_REG, &value, 1);
    return value;
}

bool UnitStep16::setLedConfig(uint8_t config)
{
    return writeRegister(UNIT_STEP16_LED_CONFIG_REG, &config, 1) == 0;
}

uint8_t UnitStep16::getLedConfig()
{
    uint8_t config = 0;
    readRegister(UNIT_STEP16_LED_CONFIG_REG, &config, 1);
    return config;
}

bool UnitStep16::setLedBrightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    return writeRegister(UNIT_STEP16_LED_BRIGHTNESS_REG, &brightness, 1) == 0;
}

uint8_t UnitStep16::getLedBrightness()
{
    uint8_t brightness = 0;
    readRegister(UNIT_STEP16_LED_BRIGHTNESS_REG, &brightness, 1);
    return brightness;
}

bool UnitStep16::setSwitchState(uint8_t state)
{
    return writeRegister(UNIT_STEP16_SWITCH_REG, &state, 1) == 0;
}

uint8_t UnitStep16::getSwitchState()
{
    uint8_t state = 0;
    readRegister(UNIT_STEP16_SWITCH_REG, &state, 1);
    return state;
}

bool UnitStep16::setRgbConfig(uint8_t config)
{
    return writeRegister(UNIT_STEP16_RGB_CONFIG_REG, &config, 1) == 0;
}

uint8_t UnitStep16::getRgbConfig()
{
    uint8_t config = 0;
    readRegister(UNIT_STEP16_RGB_CONFIG_REG, &config, 1);
    return config;
}

bool UnitStep16::setRgbBrightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    return writeRegister(UNIT_STEP16_RGB_BRIGHTNESS_REG, &brightness, 1) == 0;
}

uint8_t UnitStep16::getRgbBrightness()
{
    uint8_t brightness = 0;
    readRegister(UNIT_STEP16_RGB_BRIGHTNESS_REG, &brightness, 1);
    return brightness;
}

bool UnitStep16::setRgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t rgb[3] = {r, g, b};
    return writeRegister(UNIT_STEP16_RGB_VALUE_REG, rgb, 3) == 0;
}

bool UnitStep16::getRgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t rgb[3];
    if (readRegister(UNIT_STEP16_RGB_VALUE_REG, rgb, 3) != 0) {
        return false;
    }
    *r = rgb[0];
    *g = rgb[1];
    *b = rgb[2];
    return true;
}

bool UnitStep16::setRValue(uint8_t r)
{
    return writeRegister(UNIT_STEP16_R_VALUE_REG, &r, 1) == 0;
}

bool UnitStep16::setGValue(uint8_t g)
{
    return writeRegister(UNIT_STEP16_G_VALUE_REG, &g, 1) == 0;
}

bool UnitStep16::setBValue(uint8_t b)
{
    return writeRegister(UNIT_STEP16_B_VALUE_REG, &b, 1) == 0;
}

bool UnitStep16::saveToFlash(uint8_t save)
{
    bool result = writeRegister(UNIT_STEP16_SAVE_FLASH_REG, &save, 1) == 0;
    delay(50);  // Wait for the save operation to complete
    return result;
}

uint8_t UnitStep16::getVersion()
{
    uint8_t version = 0;
    readRegister(UNIT_STEP16_VERSION_REG, &version, 1);
    return version;
}

uint8_t UnitStep16::getAddress()
{
    uint8_t address = 0;
    readRegister(UNIT_STEP16_ADDRESS_REG, &address, 1);
    return address;
}

bool UnitStep16::setAddress(uint8_t address)
{
    bool result = writeRegister(UNIT_STEP16_ADDRESS_REG, &address, 1) == 0;
    delay(50);  // Wait for the save operation to complete
    return result;
}

bool UnitStep16::setDefaultConfig()
{
    setLedConfig(UNIT_STEP16_LED_CONFIG_DEFAULT);
    setLedBrightness(UNIT_STEP16_LED_BRIGHTNESS_DEFAULT);
    setSwitchState(UNIT_STEP16_SWITCH_DEFAULT);
    setRgbConfig(UNIT_STEP16_RGB_CONFIG_DEFAULT);
    setRgbBrightness(UNIT_STEP16_RGB_BRIGHTNESS_DEFAULT);
    setRgb(UNIT_STEP16_R_VALUE_DEFAULT, UNIT_STEP16_G_VALUE_DEFAULT, UNIT_STEP16_B_VALUE_DEFAULT);
    saveToFlash(UNIT_STEP16_SAVE_LED_CONFIG);
    saveToFlash(UNIT_STEP16_SAVE_RGB_CONFIG);
    setAddress(UNIT_STEP16_DEFAULT_I2C_ADDRESS);
    return true;
}

void UnitStep16::printRegisters()
{
    Serial.println("------------- Step16 -------------");
    Serial.print("encoder value: 0x");
    Serial.println(getValue(), HEX);

    Serial.print("LED config: ");
    Serial.print(getLedConfig());
    Serial.println("s");

    Serial.print("LED brightness: ");
    Serial.println(getLedBrightness());

    Serial.print("switch state: ");
    Serial.println(getSwitchState() == UNIT_STEP16_SWITCH_CLOCKWISE ? "clockwise" : "counterclockwise");

    Serial.print("RGB config: ");
    Serial.println(getRgbConfig() == UNIT_STEP16_RGB_CONFIG_OFF ? "off" : "on");

    Serial.print("RGB brightness: ");
    Serial.println(getRgbBrightness());

    uint8_t r, g, b;
    getRgb(&r, &g, &b);
    Serial.print("RGB value: R=");
    Serial.print(r);
    Serial.print(", G=");
    Serial.print(g);
    Serial.print(", B=");
    Serial.println(b);

    Serial.print("version: 0x");
    Serial.println(getVersion(), HEX);

    Serial.print("address: 0x");
    Serial.println(getAddress(), HEX);

    Serial.println("--------------------------------");
}
