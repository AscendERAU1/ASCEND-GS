// Important Libraries
#include <SPI.h>
#include <LoRa.h>

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Wire.h>

// Define pins (Define specific ones for feather)
#define ss 10
#define rst 5
#define dio0 2

#define PMTK_NMEA_OUTPUT_CONFIG "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_NMEA_UPDATE_1HZ "$PMTK220,1000*1F"

SoftwareSerial gpsSerial(8,7);
Adafruit_GPS GPS(&gpsSerial);


void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("hiiiiiii");
  GPS.begin(9600);
  GPS.sendCommand(PMTK_NMEA_OUTPUT_CONFIG);
  GPS.sendCommand(PMTK_NMEA_UPDATE_1HZ);

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
  GPS.read();
  // Getting the sending buffer for payload gps to send over LoRa
  if (GPS.newNMEAreceived()) {
    String strBuffer = "";
    if (!GPS.parse(GPS.lastNMEA())) {
      return;
    }
    strBuffer += " lt ";
    strBuffer += String(GPS.latitude, 4);
    strBuffer += " ln -";
    strBuffer += String(GPS.longitude, 4);
    strBuffer += " Alt ";
    strBuffer += String(GPS.altitude ,4);
    char sendBuffer[100];
    strBuffer.toCharArray(sendBuffer, 100);

    // Start sending packet
    LoRa.beginPacket();
    LoRa.print("GPS Packet:");
    LoRa.print(sendBuffer);
    LoRa.endPacket();
    Serial.println(sendBuffer);
    delay(1000); // Wait 1 seconds between transmissions
  }
}
 
