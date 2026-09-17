/*
 * ESP32-S3 BLE Sensor Advertiser
 * Change NODE_ID on each board (1, 2, 3...)
 */

#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>
#include <string>

#define NODE_ID             2          // ← CHANGE THIS on second board to 2
#define UPDATE_INTERVAL_MS  2000
#define MANUFACTURER_ID     0xFFFF

BLEAdvertising *pAdvertising = nullptr;

struct SensorPacket {
  uint8_t  nodeId;
  uint16_t temperature;   // *10
  uint16_t humidity;      // *10
  uint16_t pressure;
  uint16_t battery;
  uint32_t counter;
} __attribute__((packed));

void updateAdvertisingData() {
  SensorPacket pkt;
  pkt.nodeId      = NODE_ID;
  pkt.temperature = random(180, 320);   // 18.0 - 32.0 °C
  pkt.humidity    = random(300, 850);   // 30.0 - 85.0 %
  pkt.pressure    = random(980, 1030);
  pkt.battery     = random(3300, 4200);
  pkt.counter     = millis() / 1000;
  //Serial.printf("Mac ADDRESS:" , advertisedDevice)
  uint8_t mfgData[2 + sizeof(SensorPacket)];
  mfgData[0] = MANUFACTURER_ID & 0xFF;
  mfgData[1] = (MANUFACTURER_ID >> 8) & 0xFF;
  memcpy(&mfgData[2], &pkt, sizeof(SensorPacket));

  std::string mfgStr((char*)mfgData, sizeof(mfgData));

  BLEAdvertisementData advData;
  advData.setFlags(0x06);
  advData.setName(("SN" + String(NODE_ID)).c_str());   // Name = SN1, SN2...
  advData.setManufacturerData(mfgStr);

  pAdvertising->setAdvertisementData(advData);
Serial.printf("My MAC Address: %s\n", BLEDevice::getAddress().toString().c_str());
  Serial.printf("[%lu] Node %d | T:%.1f°C  H:%.1f%%  P:%d hPa  Bat:%d mV  Cnt:%lu\n",
                millis()/1000, pkt.nodeId,
                pkt.temperature/10.0, pkt.humidity/10.0,
                pkt.pressure, pkt.battery, pkt.counter);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== ESP32-S3 BLE Sensor Advertiser ===");
  Serial.printf("Node ID: %d\n", NODE_ID);
  Serial.printf("Device Name will be: SN%d\n", NODE_ID);

  String deviceName = "SensorNode_" + String(NODE_ID);
  BLEDevice::init(deviceName.c_str());

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  pAdvertising->setMinInterval(160);
  pAdvertising->setMaxInterval(320);

  updateAdvertisingData();
  pAdvertising->start();

  Serial.println("Advertising started!\n");
}

void loop() {
  delay(UPDATE_INTERVAL_MS);
  updateAdvertisingData();
  pAdvertising->start();
}