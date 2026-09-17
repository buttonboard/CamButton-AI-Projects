/*
 * ESP32-S3 BLE Sensor Receiver
 * Only accepts packets from SN1, SN2, SN3... (your advertisers)
 * Data is shown as readable string, not hex
 */

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <string>

#define SCAN_TIME  3

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // ========== FILTER: Only accept devices named SN1, SN2, SN3... ==========
    if (!advertisedDevice.haveName()) return;

    String name = advertisedDevice.getName().c_str();
    if (!name.startsWith("SN")) return;          // ignore everything else

    // Optional: stricter filter - only SN1 and SN2
    // if (name != "SN1" && name != "SN2") return;

    // ========== We only reach here for our own nodes ==========
    if (!advertisedDevice.haveManufacturerData()) return;

    std::string mfg = advertisedDevice.getManufacturerData();
    if (mfg.length() < 15) return;

    uint16_t companyId = (uint8_t)mfg[0] | ((uint8_t)mfg[1] << 8);
    if (companyId != 0xFFFF) return;

    // Decode the packet
    uint8_t  nodeId  = (uint8_t)mfg[2];
    uint16_t temp    = (uint8_t)mfg[3] | ((uint8_t)mfg[4] << 8);
    uint16_t hum     = (uint8_t)mfg[5] | ((uint8_t)mfg[6] << 8);
    uint16_t press   = (uint8_t)mfg[7] | ((uint8_t)mfg[8] << 8);
    uint16_t batt    = (uint8_t)mfg[9] | ((uint8_t)mfg[10] << 8);
    uint32_t counter = (uint8_t)mfg[11] | ((uint8_t)mfg[12] << 8) |
                       ((uint8_t)mfg[13] << 16) | ((uint8_t)mfg[14] << 24);

    // ========== Clean readable output ==========
    Serial.println("========================================");
    Serial.printf("Device Name : %s\n", name.c_str());
    Serial.printf("MAC Address : %s\n", advertisedDevice.getAddress().toString().c_str());
    Serial.printf("RSSI        : %d dBm\n", advertisedDevice.getRSSI());
    Serial.printf("Node ID     : %d\n", nodeId);
    Serial.printf("Temperature : %.1f °C\n", temp / 10.0);
    Serial.printf("Humidity    : %.1f %%\n", hum / 10.0);
    Serial.printf("Pressure    : %d hPa\n", press);
    Serial.printf("Battery     : %d mV\n", batt);
    Serial.printf("Counter     : %lu\n", counter);
    Serial.println("========================================\n");
  }
};

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== ESP32-S3 BLE Sensor Receiver ===");
  Serial.println("Only showing data from SN1, SN2, SN3...\n");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  pBLEScan->start(SCAN_TIME, false);
  pBLEScan->clearResults();
  delay(100);
}