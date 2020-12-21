//
// Make M5StickC pretend to be WP6003.
//
// see. https://www.zukeran.org/shin/d/2020/12/19/co2-sensor-2/
//
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ble2902.h>
#include <M5StickC.h>

#define SERVICE_UUID          "0000FFF0-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_1 "0000FFF1-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_4 "0000FFF4-0000-1000-8000-00805F9B34FB"

boolean connected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      connected = true;
      M5.Lcd.println("connect");
    };

    void onDisconnect(BLEServer* pServer) {
      connected = false;
      M5.Lcd.println("disconnect");
    }
};

class MyCallbacks_fff1: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      //    M5.Lcd.fillScreen(BLACK);
   if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          M5.lcd.printf("%02x ", rxValue[i]);
        }
        if(rxValue[0]!=0xab) M5.lcd.println();
      }
    }
};


BLEService *pService;
BLECharacteristic *pCharacteristic_fff1;
BLECharacteristic *pCharacteristic_fff4;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.setTextSize(1);

  BLEDevice::init("6003#FAKE_DEVICEID"); // fake device id
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);

  pCharacteristic_fff1 = pService->createCharacteristic(
                           CHARACTERISTIC_UUID_1,
                           BLECharacteristic::PROPERTY_WRITE |
                           BLECharacteristic::PROPERTY_WRITE_NR
                         );
  pCharacteristic_fff1->setCallbacks(new MyCallbacks_fff1());

  pCharacteristic_fff4 = pService->createCharacteristic(
                           CHARACTERISTIC_UUID_4,
                           BLECharacteristic::PROPERTY_READ |
                           BLECharacteristic::PROPERTY_NOTIFY
                         );
  pCharacteristic_fff4->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  M5.Lcd.println("start");
}

int n = 0;
uint8_t notify_data[] = { 0x0a, 0x14, 0x0c, 0x13, 0x14, 0x23, 0x00, 0xbb, 0x58, 0x02, 0x00, 0x03, 0x00, 0x00, 0x01, 0x00, 0x02, 0x19 };

void loop() {
  // put your main code here, to run repeatedly:
  delay(5000);

  if (connected) {
    pCharacteristic_fff4->setValue(notify_data, sizeof(notify_data));
    pCharacteristic_fff4->notify();
    M5.lcd.print("* ");
    //    M5.Lcd.fillScreen(BLACK);
    //    M5.lcd.setCursor(0, 1, 1);
    //    M5.lcd.printf("send notify %d", n++);
  }
  //  M5.Lcd.fillScreen(BLACK);
  //  M5.lcd.setCursor(0, 1, 1);
  //  M5.lcd.printf("%s %d",locs[loc].loc_name,ap); M5.lcd.println();
  //  M5.Lcd.println(locs[loc].aps[ap].ssid);
}
