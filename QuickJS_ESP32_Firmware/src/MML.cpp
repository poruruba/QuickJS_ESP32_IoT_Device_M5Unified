//
// MMLクラスライブラリ V1.0
// 作成  2019/03/24 by たま吉さん
// 修正  2019/05/23,不具合対応
// 修正  2020/01/19,M5Stackでのコンパイルエラー対応
//

#include "MML.h"
//#define isBreak()  (false)

// note定義
const PROGMEM  uint16_t mml_scale[] = {
  4186,  // C
  4435,  // C#
  4699,  // D
  4978,  // D#
  5274,  // E
  5588,  // F
  5920,  // F#
  6272,  // G
  6643, // G#
  7040, // A
  7459, // A#
  7902, // B
};

// mml_scaleテーブルのインデックス
#define MML_C_BASE  0
#define MML_CS_BASE 1
#define MML_D_BASE  2
#define MML_DS_BASE 3
#define MML_E_BASE  4
#define MML_F_BASE  5
#define MML_FS_BASE 6
#define MML_G_BASE  7
#define MML_GS_BASE 8
#define MML_A_BASE  9
#define MML_AS_BASE 10
#define MML_B_BASE  11

const PROGMEM  uint8_t mml_scaleBase[] = {
  MML_A_BASE, MML_B_BASE, MML_C_BASE, MML_D_BASE, MML_E_BASE, MML_F_BASE, MML_G_BASE,
};

void MML::init() {
  playduration = 0;
  flgdbug = 0;      // デバッグフラグ
  if (func_init != 0)
    func_init();
}

// TONE 周波数 [,音出し時間 [,ボリューム]]
// freq   周波数
// tm     音出し時間
// vol    音出し時間
void MML::tone(int16_t freq, int16_t tm,int16_t vol) {
  if (func_tone != 0) 
    func_tone(freq, tm, vol);
}

//　NOTONE
void MML::notone() {
  if (func_notone != 0)
    func_notone();
}

// 1文字出力
void  MML::debug(uint8_t c) {
  if (func_putc != 0)
    func_putc(c);
}

//  演奏キューチェック
uint8_t MML::available() {
  if (!flgRun)
    return 0;
  if (playduration) {
    if ( endTick < millis() ) {
        if (!flgR) notone();
        playduration = 0;
        endTick = 0;
        return 1;
    } else {
      return 0;
    }
  }
  return 1;
}
    
// TEMPO テンポ
void MML::tempo(int16_t tempo) {
  if ( (tempo < MML_MIN_TMPO) || (tempo > MML_MAX_TMPO) ) 
    return;
  common_tempo = tempo;
}

// 長さ引数の評価
int16_t MML::getParamLen() {
  int16_t tmpLen = getParam();
  if (tmpLen == -1)
    tmpLen = 0;
  else if (!((tmpLen==1)||(tmpLen==2)||(tmpLen==4)||(tmpLen==8)||
          (tmpLen==16)||(tmpLen==32)||(tmpLen==64)) ) {
    // 長さ指定エラー
    tmpLen = -1;
    err = ERR_MML; 
  }
  return tmpLen;
}

// 引数の評価
//  戻り値
//   引数なし -1 引数あり 0以上
int16_t MML::getParam() {
  int16_t tmpParam = 0;
  char* tmpPtr = mml_ptr;
  while(isdigit(*mml_ptr)) {
     if (flgdbug) debug(*mml_ptr); // デバッグ
     tmpParam*= 10;
     tmpParam+= *mml_ptr - '0';
     mml_ptr++;
  }
  if (tmpPtr == mml_ptr) {
    tmpParam = -1;
  }
  return tmpParam;
}

// 演奏開始
// 引数
//  mode  0ビット  0:フォアグランド演奏、1:バックグラウンド演奏
//        1ビット  0:先頭から            1:中断途中から
//        2ビット  0:リピートなし        1:リピートあり (バックグラウンド演奏時のみ）
void MML::play(uint8_t mode) {  
  if (!(mode & MML_RESUME))  mml_ptr = mml_text;              // 先頭からの演奏
  repeat =  ((mode & MML_REPEAT) && (mode & MML_BGM)) ? 1:0;  // リピートモード
  if ( !(mode & MML_BGM) ) {
    playMode = 1;
    flgRun = 1;
    playTick(0);  // フォアグランド演奏
  } else {
    flgRun = 1;
    playMode = 2; // バックグランド演奏
  }
}

// PLAY 文字列
void MML::playTick(uint8_t flgTick) {
  uint16_t freq;                     // 周波数
  uint16_t local_len = common_len ;  // 個別長さ
  uint8_t  local_oct = common_oct ;  // 個別高さ
  
  int8_t  scale = 0;                 // 音階
  uint32_t duration;                 // 再生時間(msec)
  uint8_t flgExtlen = 0;
  uint8_t c;                         // 1文字取り出し用
  int16_t tmpLen;                    // 音の長さ評価用
  err = 0; 

  // MMLの評価
  while(*mml_ptr) {
    if (flgdbug) debug(*mml_ptr); // デバッグ
    flgExtlen = 0;
    local_len = common_len;
    local_oct = common_oct;

    // 中断の判定
    if (!flgRun) {
      break;
    }
  
    // 空白はスキップ    
    if (*mml_ptr == ' '|| *mml_ptr == '&') {
      mml_ptr++;
      continue;
    }

    // デバッグコマンド
    if (*mml_ptr == '?') {
      if (flgdbug) 
        flgdbug = 0;
      else 
        flgdbug = 1;
      mml_ptr++;
      continue;
    }   

     c = toUpperCase(*mml_ptr);
    if ( ((c >= 'A') && (c <= 'G')) || (c == 'R') ) {  //**** 音階記号 A - Z,R 
      flgR = 0;
      if (c == 'R') {
        flgR = 1;
        mml_ptr++;
      } else {
        scale = pgm_read_byte(mml_scaleBase + c-'A'); // 音階コードの取得   
        mml_ptr++;
        if (*mml_ptr == '#' || *mml_ptr == '+') {
          //** 個別の音階半音上げ # or +
          if (flgdbug) debug(*mml_ptr); // デバッグ
          // 半音上げる
          if (scale < MML_B_BASE) {
            scale++;
          } else {
            if (local_oct < MML_MAX_OCT) {
              scale = MML_B_BASE;
              local_oct++;
            }
          }
          mml_ptr++;        
        } else if (*mml_ptr == '-') {
          //** 個別の音階半音下げ # or +
          if (flgdbug) debug(*mml_ptr); // デバッグ
          // 半音下げる
          if (scale > MML_C_BASE) {
            scale--;
          } else {
            if (local_oct > 1) {
              scale = MML_B_BASE;
              local_oct--;
            }
          }                
          mml_ptr++;      
        } 
      }
      
      //** 個別の長さの指定 
      if ( (tmpLen = getParamLen() ) < 0)
        break;
      if (tmpLen > 0) {
        local_len = tmpLen;
      }
      
      //** 半音伸ばし
      if (*mml_ptr == '.') {
        if (flgdbug) debug(*mml_ptr); // デバッグ
        mml_ptr++;
        flgExtlen = 1;
      } 

      //** 音階の再生
      duration = 240000/common_tempo/local_len;  // 再生時間(msec)
      if (flgExtlen)duration += duration>>1;

      if (flgR) {
        // 休符
        if (flgTick) {
          playduration = duration;
          endTick = millis()+duration;
          break;
        } else {
          delay(duration); 
        }
      } else {
        // 音符
        freq = pgm_read_word(&mml_scale[scale])>>(MML_MAX_OCT-local_oct); // 再生周波数(Hz);  
        if (flgTick) {         
          playduration = duration;
          endTick = millis()+duration;
          tone(freq, 0, common_vol);                      // 音の再生(時間指定なし）
          break;
        } else {
          tone(freq, (uint16_t)duration, common_vol);     // 音の再生   
        }
      }
      
    } else if (c == 'L') {  // グローバルな長さの指定     
      //**** 省略時長さ指定 L[n][.] 
      mml_ptr++;
       if ( (tmpLen = getParamLen() ) < 0)
        break;
      if (tmpLen > 0) {
        local_len = tmpLen;
        common_len = tmpLen;
        
        //** 半音伸ばし
        if (*mml_ptr == '.') {
           if (flgdbug) debug(*mml_ptr); // デバッグ
            mml_ptr++;
            common_len += common_len>>1;
            local_len =  common_len;
        } 
      } else {
        // 引数省略時は、デフォルトを設定する
        common_len = MML_len;
        local_len =  MML_len;              
      }
    //**** ボリューム指定 Vn 
    } else if (c == 'V') {  // グローバルなボリュームの指定     
      mml_ptr++;
      uint16_t tmpVol = getParam();
      if (tmpVol < 0 || tmpVol > MML_MAX_VOL) {
        err = ERR_MML; 
        break;
      }
      common_vol = tmpVol;     
    //**** 音の高さ指定 On 
    } else if (c == 'O') { // グローバルなオクターブの指定
      mml_ptr++;
      uint16_t tmpOct = getParam();
      if (tmpOct < 1 || tmpOct > MML_MAX_OCT) {
        err = ERR_MML; 
        break;
      }
      common_oct = tmpOct;
      local_oct = tmpOct;
    } else if (c == '>') { // グローバルな1オクターブ上げる
      if (common_oct < MML_MAX_OCT) {
        common_oct++;
      }
      mml_ptr++;
    //**** 1オクターブ下げる < 
    } else if (c == '<') { // グローバルな1オクターブ下げる
      if (common_oct > 1) {
        common_oct--;
      }
      mml_ptr++;
    //**** テンポ指定 Tn 
    } else if (c == 'T') { // グローバルなテンポの指定
      mml_ptr++;      
      //** 長さの指定
      uint32_t tmpTempo = getParam();
      if (tmpTempo < MML_MIN_TMPO || tmpTempo > MML_MAX_TMPO) {
        err = ERR_MML; 
        break;               
      }
      common_tempo = tmpTempo;      
    } else {
      err = ERR_MML; 
      break;
    }
  }
  if ((!*mml_ptr && available()) || isError() ) {
    flgRun = 0;    // 演奏終了
    playMode = 0;
  }
}
