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
#ifndef __AIRQUALITY_H__
#define __AIRQUALITY_H__

#include <Arduino.h>

class AirQuality
{
public:
    bool error;

    void init(uint8_t pin);      // 初期化開始（すぐ返る）。update() を毎回呼ぶこと。
    bool isReady(void);      // 初期化完了確認
    void update(void);       // loop() から毎回呼ぶ。初期化進行 & 計測更新を行う。
    int slope(void);         // 最新の汚染レベルを返す (0-3, -1=未計測)
    void stopTimer(void);    // 計測タイマを停止する
    void resetAll(void);     // 初期化・計測をすべて中断する

private:
    enum InitState {
        INIT_WAIT_START,    // 3秒待ち
        INIT_WARMING_UP,    // 20秒待ち（センサ加熱）
        INIT_CHECK_VOLTAGE, // 電圧チェック
        INIT_RETRY_WAIT,    // 60秒待ち（リトライ）
        INIT_READY,
        INIT_ERROR
    };

    int i;
    long vol_standard;
    int init_voltage;
    int first_vol;
    int last_vol;
    int temp;
    volatile int timer_index;

    uint8_t _pin;
    TimerHandle_t _timer;
    volatile bool _ready;
    InitState _initState;
    unsigned long _initStateTime;   // 現在ステートの開始時刻 (ms)
    unsigned char _retry;           // リトライ回数
    int _slopeValue;                // getSlope() が返す最新値 (-1=未計測)
    unsigned long _avgStartTime;    // avgVoltage() の5分計測開始時刻 (ms)

    void avgVoltage(void);
    void startTimer(void);
    static void timerCallback(TimerHandle_t xTimer);
};
#endif
