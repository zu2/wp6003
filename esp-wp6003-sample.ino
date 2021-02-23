//#include <M5StickC.h>
#include "M5Atom.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>

bool   notified = false;
time_t notify_time;

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID "0000FFF0-0000-1000-8000-00805F9B34FB"
#define COMMAND_UUID "0000FFF1-0000-1000-8000-00805F9B34FB"
#define SENSOR_UUID  "0000FFF4-0000-1000-8000-00805F9B34FB"
#define DEVICE_NAME  "6003#XXXXXXXXXXXXX" // Your WP6003 device name

static BLEUUID  serviceUUID(SERVICE_UUID);
static BLEUUID  commandUUID(COMMAND_UUID);
static BLEUUID  sensorUUID(SENSOR_UUID);

static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCommand;
static BLERemoteCharacteristic* pRemoteSensor;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" data length:");
  Serial.print(length);
  Serial.print(", ");
  for (auto i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);Serial.print(" ");
  }
  Serial.println("");
  if(pData[0]!=0x0a) {
    return;
  }
  double temp  = (pData[6] * 256 + pData[7]) / 10.0;
  double tvoc  = (pData[10] * 256 + pData[11])/1000.0;
  double hcho  = (pData[12] * 256 + pData[13])/1000.0;
  int    co2   = (pData[16] * 256 + pData[17]);
  Serial.print("now: "); Serial.println(time(NULL));
  Serial.printf("Time: 20%02d/%02d/%02d %02d:%02d\n",pData[1],pData[2],pData[3],pData[4],pData[5]);
  Serial.printf("Temp: %.1f\n",temp);
  Serial.printf("TVOC: %.3f\n",tvoc);
  Serial.printf("HCHO: %.3f\n",hcho);
  Serial.printf("CO2 : %d\n",co2);

  notified = true;
  notify_time = time(NULL);
}

void sendCalibration() {
  Serial.println("send calibration command 'ad'");
  byte command_ad[] = { 0xad };
  pRemoteCommand->writeValue(command_ad, sizeof(command_ad));
  delay(500);
} 

bool connectToServer(BLEAddress pAddress) {
  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());
  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->connect(pAddress);
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  Serial.print(" - Connected to server :");
  Serial.println(serviceUUID.toString().c_str());
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }

  pRemoteCommand = pRemoteService->getCharacteristic(commandUUID);
  Serial.print(" - Found our command UUID: ");
  Serial.println(commandUUID.toString().c_str());
  if (pRemoteCommand == nullptr) {
    Serial.print("Failed to find our command UUID: ");
    return false;
  }
  Serial.println(" - Found our characteristic");

  pRemoteSensor = pRemoteService->getCharacteristic(sensorUUID);
  Serial.print(" - Found our sensor UUID: ");
  Serial.println(sensorUUID.toString().c_str());
  if (pRemoteSensor == nullptr) {
    Serial.print("Failed to find our sensor UUID: ");
    return false;
  }
  Serial.println(" - Found our Sensor");

#if 0
  delay(1000);
  Serial.println("send initialize command 'ee'");
  byte command_ee[] = { 0xee };
  pRemoteCommand->writeValue(command_ee, sizeof(command_ee));
  delay(2000);
#endif

  notified = false;
  notify_time = time(NULL);
  Serial.println("send register notify");
  pRemoteSensor->registerForNotify(notifyCallback);
  M5.dis.drawpix(0, 0x0000f0); // blue
  delay(500);

  time_t t = time(NULL);
  struct tm *tm;
  tm = localtime(&t);
  byte command_aa[] = { 0xaa, (byte)(tm->tm_year%100), (byte)(tm->tm_mon+1), (byte)(tm->tm_mday),
                              (byte)(tm->tm_hour), (byte)(tm->tm_min), (byte)(tm->tm_sec) };
  pRemoteCommand->writeValue(command_aa, sizeof(command_aa));
  delay(500);
   
  Serial.println("send notify interval 'ae'");
  byte command_ae[] = { 0xae, 0x01, 0x01 }; // notify every 1min
  pRemoteCommand->writeValue(command_ae, sizeof(command_ae));
  delay(500);
  
  Serial.println("send notify request");
  static byte command_ab[] = { 0xab };
  pRemoteCommand->writeValue(command_ab, sizeof(command_ab));
  delay(500);
  
  return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.print(advertisedDevice.toString().c_str());
      Serial.print(", ");
      Serial.println(advertisedDevice.getName().c_str());
      if (advertisedDevice.getName() == DEVICE_NAME) {
        Serial.println(advertisedDevice.getAddress().toString().c_str());
        advertisedDevice.getScan()->stop();
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
        doConnect = true;
      }
      delay(10);
    }
};


void setup() {
  M5.begin(true, false, true);
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  Serial.println("waiting to connect");
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  Serial.println("getScan");
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  Serial.println("setAdvertisedDeviceCallbacks");
  Serial.print("connect to "); Serial.println(DEVICE_NAME);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10);
  Serial.println("");
  Serial.println("End of BLE setup");
}

int notifyRequest = 0;

void loop()
{
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      connected = true;
      doConnect = false;
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      connected = false;
    }
  }
  if(connected==true){
    notifyRequest++;
    if (notifyRequest > 600) { // 60sec
      Serial.println("send requesti notify 'ab'"); // request notify
      static byte command_ab[] = { 0xab };
      pRemoteCommand->writeValue(command_ab, sizeof(command_ab));
      notifyRequest = 0;
      time_t now = time(NULL);
      if(now>notify_time+1200){ // no notify about 20 min
        ESP.restart(); // restart
      }
    }
  }
  delay(100);
}
