import * as unitsynth from "UnitSynth";
import * as bytebutton from "UnitByteButton";
import * as wire from "Wire";

var base = unitsynth.NOTE_BASE + unitsynth.NOTE_OCTAVE * 3;
var tones = [
  base + unitsynth.NOTE_C,
  base + unitsynth.NOTE_D,
  base + unitsynth.NOTE_E,
  base + unitsynth.NOTE_F,
  base + unitsynth.NOTE_G,
  base + unitsynth.NOTE_A,
  base + unitsynth.NOTE_B,
  base + unitsynth.NOTE_OCTAVE + unitsynth.NOTE_C
];
const channel = 0;
var prev_status = 0xff;

function setup(){
  wire.begin(32, 33);
  bytebutton.begin();
  unitsynth.begin(26, 36);
  unitsynth.setAllNoteOff(channel);
  unitsynth.setInstrument(channel, unitsynth.CATEGORY_PIANO_00);
}

function loop(){
  var status = bytebutton.getSwitchStatus();
  var buttons = (prev_status ^ status) & (~status & 0xff);
  prev_status = status;

  for( let i = 0 ; i < 8; i++ ){
    if( (buttons & (0x01 << (8 - i - 1))) ){
      unitsynth.setNoteOn(channel, tones[i], 100);
    }
  }
}