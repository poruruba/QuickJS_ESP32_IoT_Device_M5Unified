/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include <Arduino.h>
#include <unit_audioplayer.hpp>

bool AudioPlayerUnit::begin(HardwareSerial* serial, uint8_t RX, uint8_t TX)
{
    _serial = serial;
    _serial->begin(UNIT_AUDIOPLAYER_BAUD, SERIAL_8N1, RX, TX);
    if (getCurrentStorageDeviceType() != AUDIO_PLAYER_STORAGE_DEVICE_SD) {
        Serial.println("Unit AudioPlayer maybe not connected or SD card not inserted, please check the connection");
        return false;
    }
    return true;
}

void AudioPlayerUnit::sendCommand(uint8_t command, uint8_t* data, size_t dataLen, uint8_t* returnValue)
{
    memset(receivedData, 0, sizeof(receivedData));

    while (_serial->available()) {
        _serial->read();
    }
    // Prepare message: [command, ~command & 0xFF, dataLen] + data
    uint8_t message[32];
    message[0] = command;
    message[1] = (~command) & 0xFF;
    message[2] = dataLen;

    // Copy data to message
    for (size_t i = 0; i < dataLen; i++) {
        message[3 + i] = data[i];
    }

    // Calculate checksum
    uint8_t sum = 0;
    for (size_t i = 0; i < 3 + dataLen; i++) {
        sum += message[i];
    }

    message[3 + dataLen] = sum & 0xFF;

    this->command = command;
    if (returnValue != nullptr) {
        this->returnValue[0] = returnValue[0];
        this->returnValue[1] = returnValue[1];
        hasReturnValue       = true;
    } else {
        hasReturnValue = false;
    }

#ifdef UNIT_AUDIOPLAYER_DEBUG
    String debugMsg = "<-- ";
    for (size_t i = 0; i < 4 + dataLen; i++) {
        char buffer[6];
        sprintf(buffer, "0x%02X ", message[i]);
        debugMsg += buffer;
    }
    Serial.println(debugMsg);
#endif

    _serial->write(message, 4 + dataLen);
}

bool AudioPlayerUnit::waitForResponse(unsigned int timeOut)
{
    isReceived      = false;
    receivedDataLen = 0;

    unsigned long startTime = millis();
    while (!isReceived) {
        update();
        if (millis() - startTime > timeOut) {
            return false;
        }
        delay(10);
    }
    return true;
}

bool AudioPlayerUnit::update()
{
    if (_serial == nullptr) {
        Serial.println("Please call begin() first.");
        return false;
    }

    if (_serial->available() >= 6) {  // Minimum message length
        uint8_t data[32];             // Max buffer size
        size_t dataLen = _serial->available();
        dataLen        = dataLen > 32 ? 32 : dataLen;
        _serial->readBytes(data, dataLen);

#ifdef UNIT_AUDIOPLAYER_DEBUG
        String debugMsg = "--> ";
        for (size_t i = 0; i < dataLen; i++) {
            char buffer[6];
            sprintf(buffer, "0x%02X ", data[i]);
            debugMsg += buffer;
        }
        Serial.println(debugMsg);
#endif

        bool validMessage = false;
        if ((data[0] == 0x0A && command == 0x05) || (data[0] == command && data[1] == (~command & 0xFF))) {
            bool validReturnValue = true;
            if (hasReturnValue) {
                validReturnValue = (data[2] == returnValue[0] && data[3] == returnValue[1]);
            }

            uint8_t checksum = 0;
            for (size_t i = 0; i < dataLen - 1; i++) {
                checksum += data[i];
            }

            if (validReturnValue && data[dataLen - 1] == (checksum & 0xFF)) {
                validMessage = true;

                receivedDataLen = dataLen - 5;
                for (size_t i = 0; i < receivedDataLen; i++) {
                    receivedData[i] = data[i + 4];
                }

                isReceived = true;
                rawMessage = "";
                for (size_t i = 0; i < dataLen; i++) {
                    char buffer[6];
                    sprintf(buffer, "0x%02X ", data[i]);
                    rawMessage += buffer;
                }
                return true;
            }
        }

        if (!validMessage) {
#ifdef UNIT_AUDIOPLAYER_DEBUG
            Serial.println("Invalid frame received: header/footer mismatch");
#endif
            while (_serial->available()) {
                _serial->read();
            }
        }
    }

    return false;
}

play_status_t AudioPlayerUnit::checkPlayStatus()
{
    uint8_t data[]        = {0x00};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_status_t)receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

play_status_t AudioPlayerUnit::playAudio()
{
    uint8_t data[]        = {0x01};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_status_t)receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

play_status_t AudioPlayerUnit::pauseAudio()
{
    uint8_t data[]        = {0x02};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_status_t)receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

play_status_t AudioPlayerUnit::stopAudio()
{
    uint8_t data[]        = {0x03};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_status_t)receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint16_t AudioPlayerUnit::nextAudio()
{
    uint8_t data[]        = {0x05};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(600)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint16_t AudioPlayerUnit::previousAudio()
{
    uint8_t data[]        = {0x04};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(600)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint16_t AudioPlayerUnit::playAudioByIndex(uint16_t index)
{
    uint8_t data[]        = {0x06, (uint8_t)((index >> 8) & 0xFF), (uint8_t)(index & 0xFF)};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 3, returnValue);
    if (waitForResponse(500)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint8_t AudioPlayerUnit::playAudioByName(const String& name)
{
    uint8_t data[32];  // Ensure enough space
    data[0] = 0x07;

    // Convert string to bytes
    size_t nameLen = name.length();
    for (size_t i = 0; i < nameLen; i++) {
        data[i + 1] = name.charAt(i);
    }

    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, nameLen + 1, returnValue);
    if (waitForResponse(500)) {
        return receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

void AudioPlayerUnit::decreaseVolume()
{
    uint8_t data[] = {0x03};
    sendCommand(0x06, data, 1, nullptr);
    delay(100);
}

void AudioPlayerUnit::increaseVolume()
{
    uint8_t data[] = {0x02};
    sendCommand(0x06, data, 1, nullptr);
    delay(100);
}

uint8_t AudioPlayerUnit::getVolume()
{
    uint8_t data[]        = {0x00};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x06, data, 1, returnValue);
    if (waitForResponse(600)) {
        return receivedData[0];
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

void AudioPlayerUnit::setVolume(uint8_t volume)
{
    uint8_t data[] = {0x01, volume};
    sendCommand(0x06, data, 2, nullptr);
    delay(100);
}

storage_device_t AudioPlayerUnit::getCurrentStorageDeviceType()
{
    uint8_t data[]        = {0x08};
    uint8_t returnValue[] = {0x02, 0x08};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (storage_device_t)receivedData[0];
    }
    return AUDIO_PLAYER_STORAGE_DEVICE_ERROR;
}

play_device_t AudioPlayerUnit::getCurrentPlayDeviceType()
{
    uint8_t data[]        = {0x09};
    uint8_t returnValue[] = {0x02, 0x09};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_device_t)receivedData[0];
    }
    return AUDIO_PLAYER_PLAY_DEVICE_ERROR;
}

uint16_t AudioPlayerUnit::getTotalAudioNumber()
{
    uint8_t data[]        = {0x0D};
    uint8_t returnValue[] = {0x03, 0x0D};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint16_t AudioPlayerUnit::getCurrentAudioNumber()
{
    uint8_t data[]        = {0x0E};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

void AudioPlayerUnit::playCurrentAudioAtTime(uint8_t timeMin, uint8_t timeSec)
{
    uint8_t data[] = {0x0F, timeMin, timeSec};
    sendCommand(0x04, data, 3, nullptr);
    delay(100);
}

void AudioPlayerUnit::playAudioAtTime(uint16_t audioIndex, uint8_t timeMin, uint8_t timeSec)
{
    uint8_t data[] = {0x10, (uint8_t)((audioIndex >> 8) & 0xFF), (uint8_t)(audioIndex & 0xFF), timeMin, timeSec};
    sendCommand(0x04, data, 5, nullptr);
    delay(100);
}

void AudioPlayerUnit::nextDirectory()
{
    uint8_t data[]        = {0x13};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 1, returnValue);
    delay(100);
}

void AudioPlayerUnit::previousDirectory()
{
    uint8_t data[]        = {0x12};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 1, returnValue);
    delay(100);
}

void AudioPlayerUnit::endAudio()
{
    uint8_t data[] = {0x14};
    sendCommand(0x04, data, 1, nullptr);
    delay(100);
}

uint16_t AudioPlayerUnit::selectAudioNum(uint16_t audioNum)
{
    uint8_t data[]        = {0x16, (uint8_t)((audioNum >> 8) & 0xFF), (uint8_t)(audioNum & 0xFF)};
    uint8_t returnValue[] = {0x03, 0x0E};
    sendCommand(0x04, data, 3, returnValue);
    if (waitForResponse(600)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

uint16_t AudioPlayerUnit::getCurrentPathFileCount()
{
    uint8_t data[]        = {0x18};
    uint8_t returnValue[] = {0x03, 0x18};
    sendCommand(0x04, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (uint16_t)(receivedData[0] << 8 | receivedData[1]);
    }
    return AUDIO_PLAYER_STATUS_ERROR;
}

void AudioPlayerUnit::getTotalPlayTime(uint8_t* buffer)
{
    uint8_t data[]        = {0x00};
    uint8_t returnValue[] = {0x04, 0x00};
    sendCommand(0x05, data, 1, returnValue);
    if (waitForResponse(500) && receivedDataLen >= 3) {
        memcpy(buffer, receivedData, 3);
    }
}

void AudioPlayerUnit::repeatAtTime(uint8_t startMin, uint8_t startSec, uint8_t endMin, uint8_t endSec)
{
    uint8_t data[] = {0x00, startMin, startSec, endMin, endSec};
    sendCommand(0x08, data, 5, nullptr);
    delay(100);
}

void AudioPlayerUnit::endRepeat()
{
    uint8_t data[] = {0x01};
    sendCommand(0x08, data, 1, nullptr);
    delay(100);
}

play_mode_t AudioPlayerUnit::getPlayMode()
{
    uint8_t data[]        = {0x00};
    uint8_t returnValue[] = {0x02, 0x00};
    sendCommand(0x0B, data, 1, returnValue);
    if (waitForResponse(500)) {
        return (play_mode_t)receivedData[0];
    }
    return AUDIO_PLAYER_MODE_ERROR;
}

void AudioPlayerUnit::setPlayMode(play_mode_t mode)
{
    uint8_t data[] = {0x01, mode};
    sendCommand(0x0B, data, 2, nullptr);
    delay(100);
}

void AudioPlayerUnit::startCombinePlay(uint8_t mode, uint8_t* inputData, size_t dataLen)
{
    if (dataLen > 30) {
        return;
    }

    uint8_t message[32];
    message[0] = mode;

    for (size_t i = 0; i < dataLen; i++) {
        message[i + 1] = inputData[i];
    }

    sendCommand(0x0C, message, dataLen + 1, nullptr);
    delay(100);
}

void AudioPlayerUnit::endCombinePlay()
{
    uint8_t data[] = {0x02};
    sendCommand(0x0C, data, 1, nullptr);
    delay(100);
}

bool AudioPlayerUnit::intoSleepMode()
{
    uint8_t message[] = {0x0D, 0xF3, 0x01, 0x01, 0x02};
    _serial->write(message, sizeof(message));
    return true;
}