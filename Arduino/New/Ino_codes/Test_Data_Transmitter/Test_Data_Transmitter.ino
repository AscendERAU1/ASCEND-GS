#include <SPI.h>
#include <LoRa.h>

#define ss 10
#define rst 5
#define dio0 2

//const unsigned long GPS_TX_INTERVAL  = 10000;  // 10 s
const unsigned long DATA_TX_INTERVAL = 20000;  // 55 s
unsigned long lastGpsTx  = 0;
unsigned long lastDataTx = 0;
const unsigned long PACKET_GAP       = 250;    // time between packets in a burst
unsigned long lastPacketTx = 0;
uint8_t nextPacket = 0;      // 0 = idle, 1..4 = next packet to send


// ---------- fake sensor values (change these to test parsing) ----------
float fx = 0;   // increments each send so you can see values change

// ---------- helpers ----------
void addF(const char* tag, float v, uint8_t digits = 2) {
  LoRa.print(','); LoRa.print(tag); LoRa.print(':'); LoRa.print(v, digits);
}
void addI(const char* tag, long v) {
  LoRa.print(','); LoRa.print(tag); LoRa.print(':'); LoRa.print(v);
}

void sendGpsPacket() {
  LoRa.beginPacket();
  LoRa.print("GPS Packet: lt ");
  LoRa.print(39.123456 + fx * 0.0001, 6);
  LoRa.print(" ln ");
  LoRa.print(-105.123456, 6);
  LoRa.print(" alt ");
  LoRa.print(2500.0 + fx, 1);
  LoRa.endPacket();
  Serial.println("Sent GPS");
}

void sendD1(){
  LoRa.beginPacket();
  LoRa.print("D1,"); LoRa.print(millis());
  addI("nt", 24);
  addF("ox", 12.34 + fx); addF("oy", -3.20); addF("oz", 91.05);
  addF("gx", 0.01);       addF("gy", 0.02);  addF("gz", 0.03);
  addF("lx", 0.10);       addF("ly", 0.20);  addF("lz", 9.81);
  LoRa.endPacket();
}

void sendD2(){
  // D2
  LoRa.beginPacket();
  LoRa.print("D2,"); LoRa.print(millis());
  addF("mx", 20.1); addF("my", -5.2); addF("mz", 40.3);
  addF("ax", 0.11); addF("ay", 0.22); addF("az", 9.80);
  addF("vx", 0.00); addF("vy", 0.00); addF("vz", 9.81);
  addF("it", 25.5);
  LoRa.endPacket();
}

void sendD3(){
  // D3
  LoRa.beginPacket();
  LoRa.print("D3,"); LoRa.print(millis());
  addF("iax", 0.12); addF("iay", 0.23); addF("iaz", 9.79);
  addF("igx", 0.04); addF("igy", 0.05); addF("igz", 0.06);
  addI("co2", 420); addF("st", 23.4); addF("rh", 45.6);
  addF("ba", 2500.0 + fx, 1); addF("bp", 74000.0, 0); addF("bt", 22.1);
  addI("uv", 5); addI("als", 1200);
  LoRa.endPacket();
}

void sendD4(){
  // D4
  LoRa.beginPacket();
  LoRa.print("D4,"); LoRa.print(millis());
  addI("f1", 100); addI("f2", 200); addI("f3", 300); addI("f4", 400);
  addI("f5", 500); addI("f6", 600); addI("f7", 700); addI("f8", 800);
  addI("cl", 900); addI("nir", 1000);
  addF("et", 24.0); addF("ep", 740.5); addF("eh", 40.0);
  addF("eg", 85.2); addF("ea", 2501.0, 1);
  LoRa.endPacket();

  Serial.println("Sent D1-D4");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Test Sender");

  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(909E6)) {
    Serial.println("Starting LoRa failed!");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  LoRa.enableCrc();
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  unsigned long now = millis();

  // Start a new burst when the interval is up
  if (nextPacket == 0 && now - lastDataTx >= DATA_TX_INTERVAL) {
    lastDataTx = now;
    nextPacket = 1;
    lastPacketTx = now - PACKET_GAP;   // lets D1 go out immediately
  }

  // Send the next packet in the burstPacket != 0 && now - lastPacketTx >= PACKET once the gap has passed
  if (nextPacket != 0 && now - lastPacketTx >= PACKET_GAP) {
    lastPacketTx = now;
    switch (nextPacket) {
      case 1: sendD1(); break;
      case 2: sendD2(); break;
      case 3: sendD3(); break;
      case 4: sendD4(); Serial.println("Sent D1-D4"); break;
    }
    nextPacket = (nextPacket == 4) ? 0 : nextPacket + 1;
  }

  // anything else (sensor reads, GPS updates, logging) can go here
}