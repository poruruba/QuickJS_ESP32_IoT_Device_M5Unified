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

## QuickJS_ESP32_IoT_Deviceからの移行時の注意事項
以下の点で異なります。
- SDを使う前に、sd.begin(ssPin)を呼び出す必要があります。
- Inputのボタンの割り当て(BUTTON_X)が異なります。
- Imuの関数名が変更となっています。
