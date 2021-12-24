#include <M5StickC.h>
#include "NimBLEDevice.h"


// BLE 
NimBLEAdvertisedDevice device;
NimBLEClient *pClient_bb, *pClient_sp;
NimBLERemoteService *pService_bb, *pService_sp;
NimBLERemoteCharacteristic *pReceiver_bb, *pReceiver_sp;
NimBLERemoteCharacteristic *pSender_sp, *pSender_bb;

const int preset_cmd_size = 26;
const int bb_light_size = 5;

byte preset_cmd[] = {
  0x01, 0xFE, 0x00, 0x00,
  0x53, 0xFE, 0x1A, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0xF0, 0x01, 0x24, 0x00,
  0x01, 0x38, 0x00, 0x00,
  0x00, 0xF7
};

byte bb_light_on[] = {
  0x80, 0x80, 0xB0, 0x14, 0x7f
};

byte bb_light_off[] = {
  0x80, 0x80, 0xB0, 0x14, 0x00
};

bool triggered;
int curr_preset;
int old_preset;

void notifyCB_bb(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  int i;
  
  Serial.print("Blueboard: ");
  for (i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // In mode B the BB gives 0x80 0x80 0x90 0xNN 0x64 or 0x80 0x80 0x80 0xNN 0x00 for on and off
  // In mode C the BB gives 0x80 0x80 0xB0 0xNN 0x7F or 0x80 0x80 0xB0 0xNN 0x00 for on and off
  
  if (pData[2] == 0x90 || (pData[2] == 0xB0 && pData[4] == 0x7F)) {
    switch (pData[3]) {
      case 0x3C:
      case 0x14:
        curr_preset = 0;
        break;
      case 0x3E:
      case 0x15:
        curr_preset = 1;
        break;
      case 0x40:
      case 0x16:
        curr_preset = 2;
        break;
      case 0x41:
      case 0x17:
        curr_preset = 3;
        break;
    }
    triggered = true;
  }
}


void notifyCB_sp(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  int i;

  Serial.print("Spark: ");
  for (i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


bool connected_bb, connected_sp;


void setup() {
  // put your setup code here, to run once:
  M5.begin();
  
  M5.Lcd.setRotation(-1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println("SPARK BLUEBRD");
  Serial.println("Started");

  triggered = false;
  curr_preset = 0;
  old_preset = 0;
  
  connected_bb = false;
  connected_sp = false;
  int i;
  
  NimBLEDevice::init("");

  while (!connected_bb ) { //|| !connected_sp) {
    NimBLEDevice::init("");
  
    NimBLEScan *pScan = NimBLEDevice::getScan();
    NimBLEScanResults results = pScan->start(4);

    NimBLEUUID BBserviceUuid("03b80e5a-ede8-4b33-a751-6ce34ec4c700"); // service for iRig Blueboard MIDI mode 
    NimBLEUUID SPserviceUuid("ffc0"); // service for Spark  
  
    Serial.println("**********************************************************");
    for(i = 0; i < results.getCount() && (!connected_bb || !connected_sp); i++) {
      device = results.getDevice(i);

      // Print info
      Serial.print("Name ");
      Serial.print(device.getName().c_str());
      Serial.print(" : ");

      if (strcmp(device.getName().c_str(),"iRig BlueBoard") == 0) {
        Serial.print("Found BlueBoard by name - trying to connect....");
        pClient_bb = NimBLEDevice::createClient();
        if(pClient_bb->connect(&device)) {
          connected_bb = true;
          Serial.println("connected");
          M5.Lcd.println("Blueboard on");
        }
      }
      else if (device.isAdvertisingService(SPserviceUuid)) {
        pClient_sp = NimBLEDevice::createClient();
        if(pClient_sp->connect(&device)) {
          connected_sp = true;
          Serial.println("connected");
          M5.Lcd.println("Spark on");
        }
      }
      else
        Serial.println();
    }

    // Get the services
  
    if (connected_bb) {
      pService_bb = pClient_bb->getService(BBserviceUuid);
      if (pService_bb != nullptr) {
        pReceiver_bb = pService_bb->getCharacteristic("7772e5db-3868-4112-a1a9-f2669d106bf3");
        //pSender_bb = pService_bb->getCharacteristic("7772e5db-3868-4112-a1a9-f2669d106bf3");
        if (pReceiver_bb && pReceiver_bb->canNotify()) {
          if (!pReceiver_bb->subscribe(true, notifyCB_bb, true)) {
            connected_bb = false;
            pClient_bb->disconnect();
          }
        }
      }
    }
    
    
    if (connected_sp) {
      pService_sp = pClient_sp->getService(SPserviceUuid);
      if (pService_sp != nullptr) {
        pSender_sp   = pService_sp->getCharacteristic("ffc1");
        pReceiver_sp = pService_sp->getCharacteristic("ffc2");
        if (pReceiver_sp && pReceiver_sp->canNotify()) {
          if (!pReceiver_sp->subscribe(true, notifyCB_sp, true)) {
            connected_sp = false;
            pClient_sp->disconnect();
          }
        }
      }
    }
  }
}


void loop() {
  // put your main code here, to run repeatedly:

  M5.update();
  if (triggered) {
    triggered = false;
    if (connected_sp) {
      preset_cmd[preset_cmd_size-2] = curr_preset;
      pSender_sp->writeValue(preset_cmd, preset_cmd_size, false);
    if (connected_bb) {
      bb_light_off[3] = old_preset + 20;
      bb_light_on[3] = curr_preset + 20;
      old_preset = curr_preset;
      pReceiver_bb->writeValue(bb_light_off, bb_light_size, false);
      pReceiver_bb->writeValue(bb_light_on, bb_light_size, false);
    }
    }
  }
}
