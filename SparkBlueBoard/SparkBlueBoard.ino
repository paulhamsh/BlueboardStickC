// if CLASSIC is defined, will compile with BLE stack which allows serial bluetooth
// - if not defined will use NimBLE which has no serial bluetooth
// it matters because Android Spark apps use serial bluetooth
// but BLE doesn't handle Spark disconnection well, whereas NimBLE does
//#define CLASSIC

// define this if using a bluetooth controller like Akai LPD8 Wireless or IK Multimedia iRig Blueboard
#define BT_CONTROLLER

#include <M5StickC.h>
#include "Spark.h"

SparkPreset my_preset{0x0,0x7f,"F00DF00D-FEED-0123-4567-987654321004","Paul Preset Test","0.7","Nothing Here","icon.png",120.000000,{ 
  {"bias.noisegate", true, 2, {0.316873, 0.304245}}, 
  {"Compressor", false, 2, {0.341085, 0.665754}}, 
  {"Booster", true, 1, {0.661412}}, 
  {"Bassman", true, 5, {0.768152, 0.491509, 0.476547, 0.284314, 0.389779}}, 
  {"UniVibe", false, 3, {0.500000, 1.000000, 0.700000}}, 
  {"VintageDelay", true, 4, {0.152219, 0.663314, 0.144982, 1.000000}}, 
  {"bias.reverb", true, 7, {0.120109, 0.150000, 0.500000, 0.406755, 0.299253, 0.768478, 0.100000}} },0x00 };


// Blueboard does this:
// In mode B the BB gives 0x80 0x80 0x90 0xNN 0x64 or 0x80 0x80 0x80 0xNN 0x00 for on and off
// In mode C the BB gives 0x80 0x80 0xB0 0xNN 0x7F or 0x80 0x80 0xB0 0xNN 0x00 for on and off


///// MAIN PROGRAM

// variables for midi input 
bool got_midi;

byte mi1, mi2, mi3;
int midi_chan, midi_cmd;
byte b;
int last_preset, curr_preset;

const int bb_light_size = 5;
byte bb_light_on[] = {
  0x80, 0x80, 0xB0, 0x14, 0x7f
};

byte bb_light_off[] = {
  0x80, 0x80, 0xB0, 0x14, 0x00
};

void setup() {
  M5.begin();
  
  M5.Lcd.setRotation(-1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println("SPARK BLUEBRD");
  Serial.println("Started");
#ifdef  CLASSIC
   Serial.println("CLASSIC");
#else
   Serial.println("BLE");
#endif

  spark_state_tracker_start();  // set up data to track Spark and app state

  if (conn_status[BLE_MIDI]);

  curr_preset = 0;
  last_preset = 0;
}


void loop() {
  M5.update();
  
  // BLE MIDI
  if (!midi_in.is_empty()) {                                   // Bluetooth Midi
    midi_in.get(&mi1);  // junk, discard
    midi_in.get(&mi1);  // junk, discard
    midi_in.get(&mi1);    
    midi_in.get(&mi2);
    midi_in.get(&mi3);

    midi_chan = (mi1 & 0x0f) + 1;
    midi_cmd = mi1 & 0xf0;

    Serial.print("MIDI ");
    Serial.print(mi1, HEX);
    Serial.print(" ");
    Serial.print(mi2);
    Serial.print(" ");   
    Serial.println(mi3);
    
    if (mi1 == 0x90 || (mi1 == 0xB0 && mi3 == 0x7F)) {     
      switch (mi2) {  
        case 0x3C:
        case 0x14:
        case 24:
          curr_preset = 0;
          break;
        case 0x3E:
        case 0x15:
        case 26:
          curr_preset = 1;
          break;
        case 0x40:
        case 0x16:
        case 28:
          curr_preset = 2;
          break;
        case 0x41:
        case 0x17:
        case 29:
          curr_preset = 3;
          break;
      }
    }
  }

  if (curr_preset != last_preset) {
    change_hardware_preset(curr_preset);

    if (connected_pedal) {                       // from SparkComms
      bb_light_off[3] = last_preset + 20;
      bb_light_on[3] = curr_preset + 20;

      pReceiver_pedal->writeValue(bb_light_off, bb_light_size, false);           // from SparkComms
      pReceiver_pedal->writeValue(bb_light_on, bb_light_size, false);            // from SparkComms
    }
    last_preset = curr_preset;
  }
  update_spark_state();
}
