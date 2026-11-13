// Radio Libs
#include <SPI.h>
#include <LoRa.h>


// Define pins used by the RFM95 module
#define ss 10
#define rst 5
#define dio0 2


// Buffer defines
#define NUM_SLOTS 5
#define SLOT_SIZE 256

/* Debug
volatile uint16_t rxCount = 0;
volatile uint16_t lastLen = 0;
*/

// Takes
char slots[NUM_SLOTS][SLOT_SIZE]; // Used for each of the 4 packets we end up receiving

volatile uint8_t head = 0;   // next slot the callback will fill
volatile uint8_t tail = 0;   // next slot loop() will process
volatile uint16_t overruns = 0;


// Receive interupt

void onReceive() {

//  Debug
//  rxCount++;
//  lastLen = LoRa.available();    // bytes waiting for this packet

    uint8_t next = (head + 1) % NUM_SLOTS;
    if (next == tail) {            // all slots full, drop this packet
        overruns++;
        while (LoRa.available()) LoRa.read();  // Drop the packets if fails
        return;
    }

    int i = 0;
    while (LoRa.available() && i < SLOT_SIZE - 1) {
        slots[head][i++] = (char)LoRa.read();
    }
    slots[head][i] = '\0';
    head = next;                  
}


void packetConsumer(char *msg){
    char* save = nullptr;


    char* tok = strtok_r(msg, ",", &save);   // first call: pass the buffer

    while (tok != NULL) {
      Serial.println(tok);     
      tok = strtok_r(NULL, ",", &save);
    }

}


void setup() {
  Serial.begin(115200);
  Serial.println(F("\nStarting... \n"));

   // Radio Defines

  Serial.println("LoRa Receiver");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(909E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setSyncWord(0xF3);   // must match the transmitter
  LoRa.enableCrc();         // drop corrupted packets

  Serial.println("LoRa Receiver Ready!");

  LoRa.onReceive(onReceive);   // Calls when receiving packet
  LoRa.receive();              // start listening continuously
}




void loop(){
  /* Debug 
  static unsigned long t = 0;
  if (millis() - t > 2000) {
    t = millis();
    Serial.print("rx="); Serial.print(rxCount);
    Serial.print(" over="); Serial.print(overruns);
    Serial.print(" len="); Serial.println(lastLen);
  }
  */
  while (tail != head) {
    char *msg = slots[tail];

    packetConsumer(msg);


    tail = (tail + 1) % NUM_SLOTS;   // free the slot when done
  }


}