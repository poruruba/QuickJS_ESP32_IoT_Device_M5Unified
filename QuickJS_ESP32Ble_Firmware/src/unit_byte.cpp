/*
 *SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 *SPDX-License-Identifier: MIT
 */

#include "unit_byte.hpp"

bool mutexLocked = false;

void UnitByte::writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    _wire->beginTransmission(addr);
    _wire->write(reg);
    for (int i = 0; i < length; i++) {
        _wire->write(*(buffer + i));
    }
    _wire->endTransmission();
#if defined UNIT_BYTE_DEBUG
    Serial.print("Write bytes: [");
    Serial.print(addr);
    Serial.print(", ");
    Serial.print(reg);
    Serial.print(", ");
    for (int i = 0; i < length; i++) {
        Serial.print(buffer[i]);
        if (i < length - 1) {
            Serial.print(", ");
        }
    }
    Serial.println("]");
#else
#endif
}

void UnitByte::readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    uint8_t index = 0;
    _wire->beginTransmission(addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(addr, length);
    for (int i = 0; i < length; i++) {
        buffer[index++] = _wire->read();
    }
#if defined UNIT_BYTE_DEBUG
    Serial.print("Read bytes: [");
    Serial.print(addr);
    Serial.print(", ");
    Serial.print(reg);
    Serial.print(", ");
    for (int i = 0; i < length; i++) {
        Serial.print(buffer[i]);
        if (i < length - 1) {
            Serial.print(", ");
        }
    }
    Serial.println("]");
#else
#endif
}

uint8_t UnitByte::rgb888ToRgb233(uint32_t color)
{
    uint8_t r_led, g_led, b_led, rgb_233;
    r_led   = ((color >> 16) & 0xff);
    g_led   = ((color >> 8) & 0xff);
    b_led   = (color & 0xff);
    rgb_233 = ((r_led & 0xC0) | ((g_led & 0xE0) >> 2) | ((b_led & 0xE0) >> 5));
    return rgb_233;
}

void UnitByte::acquireMutex()
{
    while (mutexLocked) {
        delay(1);
    }
    mutexLocked = true;
}

void UnitByte::releaseMutex()
{
    mutexLocked = false;
}

bool UnitByte::begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
//    _wire->begin(_sda, _scl);
//    _wire->setClock(_speed);
//    delay(10);
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

uint8_t UnitByte::getSwitchStatus()
{
    acquireMutex();
    uint8_t temp = 0;
    uint8_t reg  = UNIT_BYTE_STATUS_REG;
    readBytes(_addr, reg, (uint8_t *)&temp, 1);
    releaseMutex();
    return temp;
}

uint8_t UnitByte::getSwitchStatus(uint8_t num)
{
    acquireMutex();
    uint8_t temp = 0;
    if (num > 7) num = 7;
    uint8_t reg = UNIT_BYTE_STATUS_8BYTE_REG + num;
    readBytes(_addr, reg, (uint8_t *)&temp, 1);
    releaseMutex();
    return temp;
}

uint8_t UnitByte::getLEDBrightness()
{
    acquireMutex();
    uint8_t temp = 0;
    uint8_t reg  = UNIT_BYTE_LED_BRIGHTNESS_REG;
    readBytes(_addr, reg, (uint8_t *)&temp, 1);
    releaseMutex();
    return temp;
}

void UnitByte::setLEDBrightness(uint8_t num, uint8_t brightness)
{
    acquireMutex();
    uint8_t reg = UNIT_BYTE_LED_BRIGHTNESS_REG + num;
    writeBytes(_addr, reg, (uint8_t *)&brightness, 1);
    releaseMutex();
}

void UnitByte::setSwitchOffRGB888(uint8_t num, uint32_t color)
{
    acquireMutex();
    if (num > 7) num = 7;
    uint8_t reg = UNIT_BYTE_SYS_DEFINE_SW0_RGB888_REG + num * 4;
    writeBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
}

uint32_t UnitByte::getSwitchOffRGB888(uint8_t num)
{
    acquireMutex();
    uint32_t color;
    if (num > 7) num = 7;
    uint8_t reg = UNIT_BYTE_SYS_DEFINE_SW0_RGB888_REG + num * 4;
    readBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
    return color;
}

void UnitByte::setSwitchOnRGB888(uint8_t num, uint32_t color)
{
    acquireMutex();
    if (num > 7) num = 7;
    uint8_t reg = UNIT_BYTE_SYS_DEFINE_SW1_RGB888_REG + num * 4;
    writeBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
}

uint32_t UnitByte::getSwitchOnRGB888(uint8_t num)
{
    acquireMutex();
    uint32_t color;
    if (num > 7) num = 7;
    uint8_t reg = UNIT_BYTE_SYS_DEFINE_SW1_RGB888_REG + num * 4;
    readBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
    return color;
}

void UnitByte::setRGB888(uint8_t num, uint32_t color)
{
    acquireMutex();
    uint8_t reg = UNIT_BYTE_RGB888_REG + num * 4;
    writeBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
}

uint32_t UnitByte::getRGB888(uint8_t num)
{
    acquireMutex();
    uint32_t color;
    uint8_t reg = UNIT_BYTE_RGB888_REG + num * 4;
    readBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
    return color;
}

void UnitByte::setRGB233(uint8_t num, uint32_t color)
{
    acquireMutex();
    uint8_t rgb_233 = rgb888ToRgb233(color);
    uint8_t reg     = UNIT_BYTE_RGB233_REG + num;
    writeBytes(_addr, reg, (uint8_t *)&rgb_233, 1);
    releaseMutex();
}

uint32_t UnitByte::getRGB233(uint8_t num)
{
    acquireMutex();
    uint32_t color;
    uint8_t reg = UNIT_BYTE_RGB233_REG + num;
    readBytes(_addr, reg, (uint8_t *)&color, 4);
    releaseMutex();
    return color;
}

void UnitByte::setLEDShowMode(byte_led_t mode)
{
    acquireMutex();
    uint8_t reg = UNIT_BYTE_LED_SHOW_MODE_REG;
    writeBytes(_addr, reg, (uint8_t *)&mode, 1);
    releaseMutex();
}

uint8_t UnitByte::getLEDShowMode()
{
    acquireMutex();
    uint8_t mode;
    uint8_t reg = UNIT_BYTE_LED_SHOW_MODE_REG;
    readBytes(_addr, reg, (uint8_t *)&mode, 1);
    releaseMutex();
    return mode;
}

void UnitByte::setFlashWriteBack()
{
    acquireMutex();
    uint8_t data = 1;
    uint8_t reg  = UNIT_BYTE_FLASH_WRITE_REG;
    writeBytes(_addr, reg, (uint8_t *)&data, 1);
    releaseMutex();
}

void UnitByte::setIRQEnable(bool en)
{
    acquireMutex();
    uint8_t reg = UNIT_BYTE_IRQ_SETUP_REG;
    writeBytes(_addr, reg, (uint8_t *)&en, 1);
    releaseMutex();
}

uint8_t UnitByte::getIRQEnable()
{
    acquireMutex();
    uint8_t en;
    uint8_t reg = UNIT_BYTE_IRQ_SETUP_REG;
    readBytes(_addr, reg, (uint8_t *)&en, 1);
    releaseMutex();
    return en;
}

uint8_t UnitByte::getFirmwareVersion()
{
    acquireMutex();
    uint8_t version;
    uint8_t reg = UNIT_BYTE_FIRMWARE_VERSION_REG;
    readBytes(_addr, reg, (uint8_t *)&version, 1);
    releaseMutex();
    return version;
}

uint8_t UnitByte::setI2CAddress(uint8_t newAddr)
{
    acquireMutex();
    uint8_t min = 1, max = 127;
    newAddr     = constrain(newAddr, min, max);
    uint8_t reg = UNIT_BYTE_I2C_ADDRESS_REG;
    writeBytes(_addr, reg, (uint8_t *)&newAddr, 1);
    releaseMutex();
    _addr = newAddr;
    return _addr;
}

uint8_t UnitByte::getI2CAddress()
{
    acquireMutex();
    uint8_t I2CAddress;
    uint8_t reg = UNIT_BYTE_I2C_ADDRESS_REG;
    readBytes(_addr, reg, (uint8_t *)&I2CAddress, 1);
    releaseMutex();
    return I2CAddress;
}
