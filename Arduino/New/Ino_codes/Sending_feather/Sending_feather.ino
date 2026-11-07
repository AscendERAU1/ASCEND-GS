

// Important Libraries
#include <SPI.h>
#include <LoRa.h>


// Define pins (Define specific ones for feather)
#define ss 10
#define rst 5
#define dio0 2

int count = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

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

  String strBuffer = "None";

void loop() {

  // Getting the sending buffer for payload gps to send over LoRa

  // 500 ft
  if (count == 1) {
    strBuffer = "lt 34.615905 ln -112.450397 Alt 5700 ";
    Serial.println("Facing North 500 ft");
  }
  else if (count == 2) {
       strBuffer = "lt 34.614536 ln -112.448734 Alt 5700";
      Serial.println("Facing East 500 ft");
    }
  else if (count == 3) {
       strBuffer = "lt 34.613167 ln -112.450397 Alt 5700";
      Serial.println("Facing South 500 ft");
    }
  else if (count == 4) {
       strBuffer = "lt 34.614536 ln -112.452061 Alt 5700";
      Serial.println("Facing West 500 ft");
    }

  // Mile 
  else if (count == 5) {
       strBuffer = "lt 34.628993 ln -112.450397 Alt 7000";
      Serial.println("Facing North 1 mi");
    }
  else if (count == 6) {
       strBuffer = "lt 34.624758 ln -112.437974 Alt 7000 ";
      Serial.println("Facing Northeast 1 mi");
    }
  else if (count == 7) {
       strBuffer = "lt 34.614535 ln -112.432831 Alt 7000";
      Serial.println("Facing East 1 mi");
    }
  else if (count == 8) {
       strBuffer = "lt 34.604313 ln -112.437977 Alt 7000";
      Serial.println("Facing Southeast 1 mi");
    }
  else if (count == 9) {
       strBuffer = "lt 34.600079 ln -112.450397 Alt 7000";
      Serial.println("Facing South 1 mi");
    }
  else if (count == 10) {
       strBuffer = "lt 34.604313 ln -112.462817 Alt 7000";
      Serial.println("Facing Southwest 1 mi");
    }
  else if (count == 11) {
       strBuffer = "lt 34.614535 ln -112.467964 Alt 7000";
      Serial.println("Facing West 1 mi");
    }
  else if (count == 12) {
       strBuffer = "lt 34.624758 ln -112.462820 Alt 7000";
      Serial.println("Facing Northwest 1 mi");
  
  } else {
    Serial.println(" Finished reseting");
    count = 0;
  }

  if (count != 0) {
    char sendBuffer[100]; 
    strBuffer.toCharArray(sendBuffer, 100);

    // Start sending packet
    LoRa.beginPacket();
    LoRa.print("GPS Packet: ");
    LoRa.print(sendBuffer);
    LoRa.endPacket();
    Serial.println(sendBuffer);
    delay(30000);  // Wait 10 seconds between transmissions
  }
  
  count++;

}
