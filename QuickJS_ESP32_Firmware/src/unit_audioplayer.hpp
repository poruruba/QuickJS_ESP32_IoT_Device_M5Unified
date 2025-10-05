/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AUDIOPLAYERUNIT_H
#define AUDIOPLAYERUNIT_H

#include <Arduino.h>
#include <map>
#include <functional>
#include "driver/uart.h"

// Unit AudioPlayer baud rate is fixed at 9600
#define UNIT_AUDIOPLAYER_BAUD 9600

// Uncomment to enable debug output
//#define UNIT_AUDIOPLAYER_DEBUG

typedef enum {
    AUDIO_PLAYER_MODE_ALL_LOOP = 0,  // 00: Play all tracks in order, loop after finishing
    AUDIO_PLAYER_MODE_SINGLE_LOOP,   // 01: Loop the current track
    AUDIO_PLAYER_MODE_FOLDER_LOOP,   // 02: Loop all tracks in the current folder
    AUDIO_PLAYER_MODE_RANDOM,        // 03: Play tracks randomly from the entire disk
    AUDIO_PLAYER_MODE_SINGLE_STOP,   // 04: Play current track once and stop
    AUDIO_PLAYER_MODE_ALL_ONCE,      // 05: Play all tracks in order once, then stop
    AUDIO_PLAYER_MODE_FOLDER_ONCE,   // 06: Play all tracks in the current folder once, then stop
    AUDIO_PLAYER_MODE_ERROR = 0xFF,  // Error value for play_mode_t return types
} play_mode_t;

typedef enum {
    AUDIO_PLAYER_STATUS_STOPPED = 0,
    AUDIO_PLAYER_STATUS_PLAYING,
    AUDIO_PLAYER_STATUS_PAUSED,
    AUDIO_PLAYER_STATUS_ERROR = 0xFF,  // Error value for play_status_t return types
} play_status_t;

typedef enum {
    AUDIO_PLAYER_STORAGE_DEVICE_UDISK = 1,
    AUDIO_PLAYER_STORAGE_DEVICE_SD,
    AUDIO_PLAYER_STORAGE_DEVICE_FLASH,
    AUDIO_PLAYER_STORAGE_DEVICE_UDISK_OR_SD,
    AUDIO_PLAYER_STORAGE_DEVICE_FLASH_OR_UDISK,
    AUDIO_PLAYER_STORAGE_DEVICE_FLASH_OR_SD,
    AUDIO_PLAYER_STORAGE_DEVICE_ERROR = 0xFF,  // Error value for storage_device_t return types
} storage_device_t;

typedef enum {
    AUDIO_PLAYER_PLAY_DEVICE_UDISK = 0,
    AUDIO_PLAYER_PLAY_DEVICE_SD,
    AUDIO_PLAYER_PLAY_DEVICE_FLASH,
    AUDIO_PLAYER_PLAY_DEVICE_ERROR = 0xFF,  // Error value for play_device_t return types
} play_device_t;

class AudioPlayerUnit {
public:
    /**
     * @brief Initializes the AudioPlayer unit with serial communication parameters.
     *
     * This function sets up the serial communication for the AudioPlayer unit by configuring
     * the specified hardware serial port with the given baud rate and pin assignments.
     *
     * @param serial Pointer to a HardwareSerial object, defaults to Serial1
     * @param RX The GPIO pin number for receiving data, defaults to 16
     * @param TX The GPIO pin number for transmitting data, defaults to 17
     * @return true if initialization was successful, false otherwise
     */
    bool begin(HardwareSerial *serial = &Serial1, uint8_t RX = 16, uint8_t TX = 17);

    /**
     * @brief Sends a command to the AudioPlayer unit.
     *
     * @param command Command byte to send
     * @param data Data bytes to send
     * @param dataLen Length of data bytes
     * @param returnValue Expected return value for validation, can be nullptr
     */
    void sendCommand(uint8_t command, uint8_t *data, size_t dataLen, uint8_t *returnValue = nullptr);

    /**
     * @brief Waits for a message from the AudioPlayer unit.
     *
     * @param timeOut Timeout in milliseconds
     * @return true if a message was received, false if timeout
     */
    bool waitForResponse(unsigned int timeOut);

    /**
     * @brief Updates the AudioPlayer unit's state and processes new messages.
     *
     * This function should be called regularly in the main loop to handle
     * new messages and update the unit's state.
     *
     * @return true if a new message was processed, false otherwise
     */
    bool update();

    /**
     * @brief Checks the current play status.
     *
     * @return Current play status
     */
    play_status_t checkPlayStatus();

    /**
     * @brief Starts or resumes audio playback.
     *
     * @return Status code
     */
    play_status_t playAudio();

    /**
     * @brief Pauses audio playback.
     *
     * @return Status code
     */
    play_status_t pauseAudio();

    /**
     * @brief Stops audio playback.
     *
     * @return Status code
     */
    play_status_t stopAudio();

    /**
     * @brief Skips to the next audio file.
     *
     * @return Current audio file number or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint16_t nextAudio();

    /**
     * @brief Skips to the previous audio file.
     *
     * @return Current audio file number or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint16_t previousAudio();

    /**
     * @brief Plays an audio file by its index.
     *
     * @param index Audio file index
     * @return Current audio file number or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint16_t playAudioByIndex(uint16_t index);

    /**
     * @brief Plays an audio file by its name.
     *
     * @param name Audio file name
     * @return Received data or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint8_t playAudioByName(const String &name);

    /**
     * @brief Decreases the volume.
     */
    void decreaseVolume();

    /**
     * @brief Increases the volume.
     */
    void increaseVolume();

    /**
     * @brief Gets the current volume level.
     *
     * @return Volume level
     */
    uint8_t getVolume();

    /**
     * @brief Sets the volume level.
     *
     * @param volume Volume level
     */
    void setVolume(uint8_t volume);

    /**
     * @brief Gets the current online device type.
     *
     * @return Device type
     */
    storage_device_t getCurrentStorageDeviceType();

    /**
     * @brief Gets the current play device type.
     *
     * @return Device type (0: USB, 1: SD, 2: SPI FLASH)
     */
    play_device_t getCurrentPlayDeviceType();

    /**
     * @brief Gets the total number of audio files.
     *
     * @return Total number of audio files
     */
    uint16_t getTotalAudioNumber();

    /**
     * @brief Gets the current audio file number.
     *
     * @return Current audio file number
     */
    uint16_t getCurrentAudioNumber();

    /**
     * @brief Plays the current audio file at a specific time.
     *
     * @param timeMin Minutes
     * @param timeSec Seconds
     */
    void playCurrentAudioAtTime(uint8_t timeMin, uint8_t timeSec);

    /**
     * @brief Plays a specific audio file at a specific time.
     *
     * @param audioIndex Audio file index
     * @param timeMin Minutes
     * @param timeSec Seconds
     */
    void playAudioAtTime(uint16_t audioIndex, uint8_t timeMin, uint8_t timeSec);

    /**
     * @brief Skips to the next directory.
     */
    void nextDirectory();

    /**
     * @brief Skips to the previous directory.
     */
    void previousDirectory();

    /**
     * @brief Ends audio playback.
     */
    void endAudio();

    /**
     * @brief Selects an audio file by its number.
     *
     * @param audioNum Audio file number
     * @return Current audio file number or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint16_t selectAudioNum(uint16_t audioNum);

    /**
     * @brief Gets the number of files in the current path.
     *
     * @return Number of files or AUDIO_PLAYER_STATUS_ERROR if an error occurred
     */
    uint16_t getCurrentPathFileCount();

    /**
     * @brief Gets the total play time of the current audio file.
     *
     * @param buffer Buffer to store the time (3 bytes: hour, minute, second)
     */
    void getTotalPlayTime(uint8_t *buffer);

    /**
     * @brief Sets the repeat play time range.
     *
     * @param startMin Start minute
     * @param startSec Start second
     * @param endMin End minute
     * @param endSec End second
     */
    void repeatAtTime(uint8_t startMin, uint8_t startSec, uint8_t endMin, uint8_t endSec);

    /**
     * @brief Ends repeat playback.
     */
    void endRepeat();

    /**
     * @brief Gets the current play mode.
     *
     * @return Play mode
     */
    play_mode_t getPlayMode();

    /**
     * @brief Sets the play mode.
     *
     * @param mode Play mode
     */
    void setPlayMode(play_mode_t mode);

    /**
     * @brief Starts combined playback.
     *
     * @param mode Combine play mode
     * @param inputData Data bytes
     * @param dataLen Length of data bytes
     */
    void startCombinePlay(uint8_t mode, uint8_t *inputData, size_t dataLen);

    /**
     * @brief Ends combined playback.
     */
    void endCombinePlay();

    /**
     * @brief Puts the device into sleep mode.
     *
     * @return true if successful
     */
    bool intoSleepMode();

private:
    HardwareSerial *_serial;
    String rawMessage;
    uint8_t command;
    uint8_t returnValue[2];
    bool hasReturnValue;
    bool isReceived;
    uint8_t receivedData[32];
    size_t receivedDataLen;
};

#endif  // AUDIOPLAYERUNIT_H