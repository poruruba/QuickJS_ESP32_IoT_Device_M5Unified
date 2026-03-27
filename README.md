# QuickJS_ESP32_IoT_Device(M5Unified対応版)

電子書籍「M5StackとJavascriptではじめるIoTデバイス制御」のサポートサイトです。<br>
https://poruruba.booth.pm/items/3735175
<br>
<br>
<br>
以下のQiitaでも適時機能拡張を紹介しています！<br>
https://qiita.com/poruruba

管理コンソールはこちら<br>
https://poruruba.github.io/QuickJS_ESP32_IoT_Device_M5Unified/QuickJS_ESP32_Firmware/data/html/

ダウンロードサイトはこちら（最新ではない場合があります）<br>
https://poruruba.github.io/EspWebDownloader/quickjs_esp32_firmware/

## サポートライブラリ

|ライブラリ名|概要|
|---|---|
|Esp32|ESP32 に関連する基本的な機能を提供します。|
|Console|UART にデバッグ文を出力します。|
|Audio|I2S に接続されたスピーカから MP3 音声を再生します。 |
|BlePeripheral|BLEペリフェラル機能を提供します。|
|Camera|ESP32 に接続されたカメラの画像撮影機能を提供します。|
|Coap|CoAP通信機能を提供します。|
|Crypto|暗号機能を提供します。|
|Env|I2C に接続された温湿度センサ（SHT30、DH12）を操作します。|
|EspNow|EspNow の機能を提供します。|
|Gpio|ESP32 の GPIO を制御します。|
|Graphql|GraphQL通信機能を提供します。|
|Http|HttpGateway を介して、HTTPS 通信します。|
|Imu|ESP32 に接続された 6 軸姿勢センサを制御します。|
|Ir|ESP32 に接続した赤外線送受信機を制御します。|
|Input|ボタンの押下を検出します。|
|Lcd|ESP32 に接続した LCD の表示を制御します。|
|Ledc|ESP32 の GPIO ピンに対して PWM 出力します。|
|Mml|Mml の楽譜を再生します。|
|Pixels|ESP32 に接続した RGB LED を制御します。|
|Prefs|ESP32 の不揮発メモリの読み書きをします。|
|Rtc|ESP32 に接続した RTC から時刻を設定・取得します。|
|Sd|ESP32 に接続された microSD カードのストレージに対するファイルの読み書きをします。|
|Udp|UDP パケットを送受信します。|
|Uart|UART 通信のための機能を提供します。|
|Utils|base64、url エンコード、HTTP 通信などのユーティリティです。|
|Websocket|Websocket によるサーバ通信機能を提供します。|
|WebsocketClient|Websocket によるクライアント通信機能を提供します。|
|Wire|周辺デバイスとの I2C 通信のための機能を提供します。|
|UnitAirquality|Air Quality Sensor モジュールを制御します。|
|UnitAngle8|8 ポテンショメータユニットを制御します。|
|UnitAsr|オフライン音声認識モジュールを制御します。|
|UnitAudioPlayer|オーディオプレイヤーユニットを制御します。|
|UnitByteButton|Byte ボタンユニットを制御します。|
|UnitColor|カラーセンサユニットを制御します。|
|UnitEnvPro|環境センサ Pro ユニットを制御します。|
|UnitGas|ガスセンサーユニットを制御します。|
|UnitGesture|ジェスチャーユニットを制御します。|
|UnitImuPro|IMU Pro ユニットを制御します。|
|UnitPbhub|I/O ハブユニットを制御します。|
|UnitSonicIo|超音波測距ユニット I/O を制御します。|
|UnitStep16|Step16ロータリエンコーダユニットを制御します。|
|UnitSynth|シンセサイザユニットを制御します。|

## 更新履歴
- 2022-3-26
  - UDP通信機能の追加
  - SDをセマフォで排他制御
  - モジュールサイズオーバーチェックを追加
- 2022-3-27
  - Audio機能の追加
  - Javascript実行の一時停止/再開の追加
- 2022-03-31
  - 不揮発メモリ操作の追加 
  - 一部動作不備の修正
- 2022-4-2
  - DAC出力のAudioに対応
  - M5Stack Fireでの動作を確認
- 2022-4-8
  - Syslog出力を追加
  - 試験的にBlocklyに対応
- 2022-4-9
  - WebAPI・BlocklyからLCDを操作できるように追加
- 2022-4-10
  - MQTT機能の追加
- 2022-4-16
  - 温湿度センサ(SHT30、DH12)の追加
- 2022-4-29
  - LCDにスプライト表示を追加
- 2022-5-1
  - audioにpause/resumeを追加
  - udpのcheckRecvの仕様を変更
- 2022-5-7
  - 各種M5ユニット用ライブラリを追加
- 2023-2-24
  - 8ポテンショメータユニットを追加
  - M5StampC3を追加
- 2023-5-20
  - LovyanGFXからM5Unitiedに変更
- 2023-7-9
  - JsonObjectを参照渡しに変更
- 2024-3-8
  - Httpモジュールのfetchのパラメータを変更。HttpBridgeサーバ方式に変更
  - EnvProユニットを追加
- 2024-5-3
  - Uartモジュールを追加
- 2024-9-29
  - esp32.ping()を追加
- 2024-11-03
  - Cryptoモジュールを追加
- 2024-12-25
  - esp32.getDatetimeを追加
  - http.fetchAws、setAwsCredentialを追加
- 2024-12-29
  - カメラ機能を追加
  - カスタムでボタンを追加できるようになりました。
- 2025-1-1
  - EspNow機能を追加
- 2025-1-18
  - Websocket機能を追加
- 2025-4-23
  - スリープモード機能を追加
- 2025-5-6
  - カスタムコールバック機能を追加
- 2025-5-17
  - 外部ディスプレイのサポートを追加
- 2025-8-2
  - Websocketクライアント機能を追加
- 2025-8-9
  - Byteボタンユニットを追加
- 2025-10-5
  - AudioPlayerユニット、STEP16ユニットを追加
- 2025-10-19
  - TOTP, Base32を追加
- 2025-10-26
  - オフライン音声認識モジュールを追加
- 2026-01-12
  - SNMPエージェントを追加
- 2026-02-11
  - BLEペリフェラルを変更
- 2026-02-22
  - AudioのI/Fを変更

## 誤記訂正
- 2022-03-31
  - 赤外線送受信とUDPのサンプルを追記 
- 2022-4-2
  - DAC出力のAudio対応に伴い、audio.beginの入力パラメータ仕様を変更 
- 2022-4-8
  - Syslog出力を追加
  - IPアドレス指定方法を変更
  - Blocklyにおいて、Javascript出力でpressになっていたのを修正
- 2022-5-1
  - Audioのdispose方法を修正
- 2023-6-15
  - SPIFFSのFile.path()となるべきところをFile.name()としていたところを修正
- 2025-11-24
  - Blocklyの定義が間違っていたのを修正

## QuickJS_ESP32_IoT_Deviceからの移行時の注意事項
以下の点で異なります。
- SDを使う前に、sd.begin(ssPin)を呼び出す必要があります。
- Inputのボタンの割り当て(BUTTON_X)が異なります。
- Imuの関数名が変更となっています。
