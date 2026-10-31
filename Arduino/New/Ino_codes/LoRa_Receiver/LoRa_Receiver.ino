#include <SPI.h>
#include <LoRa.h>

// Define pins used by the RFM95 module
#define ss 10
#define rst 9
#define dio0 2

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setSyncWord(0xF3);   // must match the transmitter
  LoRa.enableCrc();         // NEW: drop corrupted packets

  Serial.println("LoRa Receiver Ready!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String data = "";
    for (int i = 0; i < packetSize; i++) {
      data += (char)LoRa.read();
    }
    Serial.print("Received packet: ");
    Serial.print(data);
    Serial.print(" | RSSI: ");
    Serial.println(LoRa.packetRssi());
  }
}