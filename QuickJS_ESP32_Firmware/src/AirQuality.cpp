/*
  AirQuality library v1.0
  2010 Copyright (c) Seeed Technology Inc.  All right reserved.

  Original Author: Bruce.Qin

  ESP32/FreeRTOS port: AVR timer replaced with xTimerCreate (2-second interval)
  Non-blocking init: state machine driven by update()

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/
#include <Arduino.h>
#include <freertos/timers.h>

#include "AirQuality.h"

// Called every 2 seconds by FreeRTOS software timer.
// Replaces the AVR Timer1 overflow ISR (61 overflows x 32.768ms ≈ 2s).
void AirQuality::timerCallback(TimerHandle_t xTimer) {
    AirQuality* self = (AirQuality*) pvTimerGetTimerID(xTimer);
    self->first_vol = analogRead(self->_pin);
    self->timer_index++;
}

// Get the avg voltage over 5 minutes.
void AirQuality::avgVoltage(void) {
    temp += first_vol;
    i++;

    if (millis() - _avgStartTime >= 5 * 60 * 1000UL) // 5分経過
    {
        // Vol_standard in 5 minutes
        vol_standard = temp / i;
        temp = 0;
        i = 0;
        _avgStartTime = millis();
    }
}

void AirQuality::startTimer(void) {
    _timer = xTimerCreate(
        "AirQualityTimer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        (void*)this,
        timerCallback
    );
    if (_timer != NULL) {
        xTimerStart(_timer, 0);
    }
}

// 初期化を開始する。すぐ返る。以降 update() を毎回呼ぶこと。
void AirQuality::init(uint8_t pin) {
    if (_pin == pin && _ready) {
        if (_timer == NULL) // stopTimer() 後の再開
            startTimer();
        return;
    }

    _pin = pin;
    pinMode(_pin, INPUT);
    _ready = false;
    _initState = INIT_WAIT_START;
    _initStateTime = millis();
    _retry = 0;
    _timer = NULL;
    _slopeValue = -1;
    timer_index = false;
    error = false;
}

bool AirQuality::isReady(void) {
    return _ready;
}

// 計測タイマを停止・削除する。
void AirQuality::stopTimer(void) {
    if (_timer != NULL) {
        xTimerStop(_timer, 0);
        xTimerDelete(_timer, 0);
        _timer = NULL;
    }
}

// 初期化・計測をすべて中断する。
void AirQuality::resetAll(void) {
    stopTimer();
    _initState = INIT_ERROR;
    _ready = false;
}

// loop() から毎回呼ぶ。
// READY になるまでは初期化ステートマシンを進め、
// READY 以降は2秒ごとの計測値を更新する。
void AirQuality::update(void) {
    switch (_initState) {
        case INIT_WAIT_START:
            if (millis() - _initStateTime >= 3000) {
                _initStateTime = millis();
                _initState = INIT_WARMING_UP;
            }
            return;

        case INIT_WARMING_UP:
            if (millis() - _initStateTime >= 20000) {
                init_voltage = analogRead(_pin);
                _initState = INIT_CHECK_VOLTAGE;
            }
            return;

        case INIT_CHECK_VOLTAGE:
            if (init_voltage < 798 && init_voltage > 10) {
                first_vol = analogRead(_pin);
                last_vol = first_vol;
                vol_standard = last_vol;
                error = false;
                _avgStartTime = millis();
                startTimer();
                _initState = INIT_READY;
                _ready = true;
            } else {
                _retry++;
                _initStateTime = millis();
                _initState = INIT_RETRY_WAIT;
                if (_retry >= 5) {
                    error = true;
                    _initState = INIT_ERROR;
                }
            }
            return;

        case INIT_RETRY_WAIT:
            if (millis() - _initStateTime >= 60000) {
                init_voltage = analogRead(_pin);
                _initState = INIT_CHECK_VOLTAGE;
            }
            return;

        case INIT_ERROR:
        case INIT_READY:
            break;
    }

    // 以下は INIT_READY のときのみ実行される
    // timer_index の回数分ループして、スキップされた発火を消化する
    while (timer_index > 0) {
        timer_index--;

        if (first_vol - last_vol > 400 || first_vol > 700) {
            // High pollution! Force signal active.
            avgVoltage();
            _slopeValue = 0;
        } else if ((first_vol - last_vol > 400 && first_vol < 700) || first_vol - vol_standard > 150) {
            // High pollution!
            avgVoltage();
            _slopeValue = 1;
        } else if ((first_vol - last_vol > 200 && first_vol < 700) || first_vol - vol_standard > 50) {
            // Low pollution!
            avgVoltage();
            _slopeValue = 2;
        } else {
            // Air fresh
            avgVoltage();
            _slopeValue = 3;
        }
    } // while (timer_index > 0)
}

// 最新の汚染レベルを返す。update() を呼んでも未計測の場合は -1。
int AirQuality::slope(void) {
    return _slopeValue;
}