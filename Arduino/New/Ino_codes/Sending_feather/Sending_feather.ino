

// Important Libraries
#include <SPI.h>
#include <LoRa.h>


// Define pins (Define specific ones for feather)
#define ss 10
#define rst 5
#define dio0 2


void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender");

  // Setup LoRa
  // Set the previously defined pins
  LoRa.setPins(ss, rst, dio0);

  // Set frequency to 915 MHz 
  while (!LoRa.begin(915E6)) {
    // Internal Check
    Serial.println("Starting LoRa failed!");
    delay(500);
  }

  // Sync word for transmitting 
  LoRa.setSyncWord(0xF3);
  // Error checking
  LoRa.enableCrc();
  Serial.println("LoRa Initializing OK!");
}

void loop() {

  // Getting the sending buffer for payload gps to send over LoRa
  
  String strBuffer = "lt 34.5915 ln -112.3593 Alt 10000" ;
  char sendBuffer[100];
  strBuffer.toCharArray(sendBuffer, 100);

  // Start sending packet
  LoRa.beginPacket();
  LoRa.print("GPS Packet: ");
  LoRa.print(sendBuffer);
  LoRa.endPacket();
  Serial.println(sendBuffer);
  delay(1000); // Wait 1 seconds between transmissions
}
 