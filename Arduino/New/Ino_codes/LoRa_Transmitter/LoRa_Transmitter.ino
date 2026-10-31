#include <SPI.h>
#include <LoRa.h>

#define ss 10
#define rst 5
#define dio0 2

int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender");

  LoRa.setPins(ss, rst, dio0);

  // Set frequency to 915 MHz for North America (use 868E6 for Europe, 433E6 for Asia)
  while (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    delay(500);
  }
LoRa.setSyncWord(0xF3);
LoRa.enableCrc();
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // Send packet
  LoRa.beginPacket();
  LoRa.print("Hello from Arduino: ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;
  delay(1000); // Wait 10 seconds between transmissions
}
 