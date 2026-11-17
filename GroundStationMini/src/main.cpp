#include <Tic.h>
#include <math.h>
#include <string.h>

// Radio Librarys
#include <SPI.h>
#include <LoRa.h>

// Tommy Library
#include "AzimuthFull.h"

// Use hardware Serial1 for the Arduino Nano ESP32 connected to TX1
#define ticSerial Serial1

#define MAXSPEED 1500 // in steps per second
#define MAX_Pitch 90  // in degrees
#define MIN_Pitch 0   // in degrees
#define BUFFER_SIZE 100

char buffer[BUFFER_SIZE];
uint8_t bufIndex = 0;

TicSerial yawStepper(ticSerial, 14);
TicSerial pitchStepper(ticSerial, 15);

// Struct for parsing
typedef struct {
  float altitude;
  float latitude;
  float longitude;
  bool valid;
} GPSData;

float x_value, y_value, previous_y = 0.0, previous_x = 0.0;


static GPSData gps;
static AzimuthResult output;


// Ground Station Coordinates (Latitude and Longitude in degrees, Height in meters)
const float gs_latitude = 34.61292f;
const float gs_longitude = -112.45421f;
const float gs_height_m = 1576.5993f;

void resetCommandTimeout()
{
  yawStepper.resetCommandTimeout();
  pitchStepper.resetCommandTimeout();
}

void delayWhileResettingCommandTimeout(uint32_t ms)
{
  uint32_t start = millis();
  do
  {
    resetCommandTimeout();
  } while ((uint32_t)(millis() - start) <= ms);
}

// Reading LoRa message
bool readLoRaMessage() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    bufIndex = 0;
    while (LoRa.available() && bufIndex < sizeof(buffer) - 1) {
      buffer[bufIndex++] = (char)LoRa.read();
    }
    buffer[bufIndex] = '\0';
    return true;
  }
  return false;
}

/* ---------- GPS PARSER ---------- */
GPSData extractGPS(const char *msg) {
  
  GPSData gps = { 0, 0, 0, false };

  /*//Checking for bad input
  if (strstr(msg, "CRC_ERR") != NULL) {
    Serial.print("Invalid");
    return gps;  // invalid packet → terminate immediately
  }*/

  const char *altPtr = strstr(msg, " Alt ");
  const char *latPtr = strstr(msg, " lt ");
  const char *lonPtr = strstr(msg, " ln ");

  if (!altPtr || !latPtr || !lonPtr) return gps;

  gps.altitude = atof(altPtr + 4);
  gps.latitude = atof(latPtr + 3);
  gps.longitude = atof(lonPtr + 3);

  gps.valid = true;
  return gps;
}

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(200);

  // Explicitly map TX to pin D8 so data actually leaves the board
  ticSerial.begin(9600, SERIAL_8N1, -1, D8);
  delay(20);

  // Set Pin Mode for LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);

  yawStepper.exitSafeStart();
  pitchStepper.exitSafeStart();

  yawStepper.setMaxSpeed((uint32_t)MAXSPEED * 10000ULL);
  pitchStepper.setMaxSpeed((uint32_t)MAXSPEED * 10000ULL);

  yawStepper.setTargetPosition(0);
  pitchStepper.setTargetPosition(0);
  delayWhileResettingCommandTimeout(5000);

  Serial.println(F("\nGround Station Initialized with Vincenty's Formulae.\n"));
}

void loop() {
  float pay_latitude = 0;
  float pay_longitude = 0;
  float pay_height_m = 0;

  // Prompt user for payload coordinates via Serial Monitor
  // // --- LATITUDE PROMPT ---
  // Serial.print("Enter Payload Latitude (degrees): ");
  // while(true){
  //   resetCommandTimeout();
  //   if(Serial.available() > 0){
  //     pay_latitude = Serial.parseFloat();
  //     Serial.println(pay_latitude);
  //     delay(10); 
  //     while(Serial.available() > 0) { Serial.read(); }
  //     break;
  //   }
  //   delay(100);
  // }

  // // --- LONGITUDE PROMPT ---
  // Serial.print("Enter Payload Longitude (degrees): ");
  // while(true){
  //   resetCommandTimeout();
  //   if(Serial.available() > 0){
  //     pay_longitude = Serial.parseFloat();
  //     Serial.println(pay_longitude);
  //     delay(10); 
  //     while(Serial.available() > 0) { Serial.read(); }
  //     break;
  //   }
  //   delay(100);
  // }

  // // --- HEIGHT PROMPT ---
  // Serial.print("Enter Payload Height (meters): ");
  // while(true){
  //   resetCommandTimeout();
  //   if(Serial.available() > 0){
  //     pay_height_m = Serial.parseFloat();
  //     Serial.println(pay_height_m);
  //     delay(10); 
  //     while(Serial.available() > 0) { Serial.read(); }
  //     break;
  //   }
  //   delay(100);
  // }

  if(readLoRaMessage()){ 
    //Parsing GPS Data
    Serial.println("Read:");
    Serial.println(buffer);
    
    gps = extractGPS(buffer);
    
    // readLoRaMessage clears the buffer for you already
  }

  // ALL FUNCTIONS FOR WHEN GPS HAS BEEN CALLED
  // Includes:
  // Gps parsing, Azimuth Range
  if (gps.valid && ((gps.latitude != previous_x) || (gps.longitude != previous_y))) {

    Serial.println("GPS DATA FOUND");
    Serial.print("Altitude: ");
    /*IF RAM IS A PROBLEM [SUCCESS] Took 10.28 seconds 
    Serial.println(F("Your message (this will work with print normal and the vars)"))
    */
    Serial.println(gps.altitude);
    Serial.print("Latitude: ");
    Serial.println(gps.latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(gps.longitude, 6);

    //Blink Light
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);


    // Replace with internal GPS for coord1
    // Change this NOW
    float coordinate1[2] = { 34.614716, -112.450387 };  // Given Coords
    float coordinate2[2] = { gps.latitude, gps.longitude };
    float altitude1 = 4500.0;  //given altitude
    float altitude2 = gps.altitude;

    // Run Function to calculate it
    output = azmuth_Range(coordinate1, coordinate2, altitude1, altitude2);

    // Sets x_value and y_value
    x_value = -(output.ForwardAzimuth);
    y_value = (output.ElevationAngle);

    previous_y = gps.longitude;
    previous_x = gps.latitude;

  }  // end of gps valid if

  // Prepare coordinate arrays
  // Note: AzimuthFull.h expects altitude inputs in feet (it internally converts them via * 0.3048f)
  float coordinate1[2] = { gs_latitude, gs_longitude };
  float coordinate2[2] = { previous_x, previous_y };
  float altitude1_ft = gs_height_m / 0.3048f;
  float altitude2_ft = pay_height_m / 0.3048f;

  // Calculate using Vincenty's formulae via AzimuthFull.h
  AzimuthResult result = azmuth_Range(coordinate1, coordinate2, altitude1_ft, altitude2_ft);

  // Clamp elevation angle to safe operational bounds
  float elevation = result.ElevationAngle;
  if(elevation < MIN_Pitch){
    elevation = MIN_Pitch;
  }
  else if(elevation > MAX_Pitch){
    elevation = MAX_Pitch;
  }

  float azimuth = result.ForwardAzimuth;

  // Convert degrees to radians for motor step calculation
  double azimuth_rad = azimuth * PI / 180.0;
  double elevation_rad = elevation * PI / 180.0;

  // Send target positions to steppers
  resetCommandTimeout();
  yawStepper.setTargetPosition(azimuth_rad * 100000.0 / (2.0 * PI));
  pitchStepper.setTargetPosition(elevation_rad * 100000.0 / (2.0 * PI));

  // Print telemetry summary to Serial Monitor
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Calculated via Vincenty's Formulae:"));
  Serial.print(F("  Distance (S): ")); Serial.print(result.S); Serial.println(F(" meters"));
  Serial.print(F("  Azimuth:      ")); Serial.print(azimuth); Serial.println(F(" degrees"));
  Serial.print(F("  Elevation:    ")); Serial.print(elevation); Serial.println(F(" degrees"));
  Serial.println(F("Steppers updated. Prompting for next target...\n"));

  delayWhileResettingCommandTimeout(2000);
}