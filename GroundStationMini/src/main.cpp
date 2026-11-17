#include <Tic.h>
#include <string.h>

// Radio Libraries
#include <SPI.h>
#include <LoRa.h>

// Azimuth & Range Library
#include "AzimuthFull.h"

// Use hardware Serial1 for the Arduino Nano ESP32 connected to TX1
#define ticSerial Serial1

#define MAXSPEED 1500 // in steps per second
#define MAX_Pitch 90  // in degrees
#define MIN_Pitch 0   // in degrees
#define BUFFER_SIZE 100
#define STEPS_PER_RADIAN (100000.0 / (2.0 * PI))

// Define pins used by the RFM95 module
#define ss 10
#define rst 5
#define dio0 2

char buffer[BUFFER_SIZE];
uint8_t bufIndex = 0;

TicSerial yawStepper(ticSerial, 14);
TicSerial pitchStepper(ticSerial, 15);

// Struct for parsing GPS data
typedef struct {
  float altitude;
  float latitude;
  float longitude;
  bool valid;
} GPSData;

float previous_y = 0.0, previous_x = 0.0;
static GPSData gps;

// Ground Station Coordinates (Latitude and Longitude in degrees, Height in meters)
const float gs_latitude = 34.614528f;
const float gs_longitude = -112.450361f;
const float gs_height_m = 1576.5993f;

// Link loss tracking
unsigned long lastPacketTime = 0;
const unsigned long TIMEOUT_MS = 15000; // 15 seconds warning threshold

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
  GPSData parsedGps = { 0, 0, 0, false };

  const char *altPtr = strstr(msg, " Alt ");
  const char *latPtr = strstr(msg, " lt ");
  const char *lonPtr = strstr(msg, " ln ");

  if (!altPtr || !latPtr || !lonPtr) return parsedGps;

  parsedGps.altitude = atof(altPtr + 4);
  parsedGps.latitude = atof(latPtr + 3);
  parsedGps.longitude = atof(lonPtr + 3);

  parsedGps.valid = true;
  return parsedGps;
}

// Calibration helper function for manual positioning
void move_to_az_el() {
  double azimuth_deg = 0;
  double elevation_deg = 0;
  double azimuth_rad = 0;
  double elevation_rad = 0;

  Serial.print("Set yaw to: ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      azimuth_deg = Serial.parseFloat();
      azimuth_rad = azimuth_deg * PI / 180.0;
      yawStepper.setTargetPosition(azimuth_rad * STEPS_PER_RADIAN);
      Serial.println("Received yaw: " + String(azimuth_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      break;
    }
  }
  
  Serial.print("Set pitch to: ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      elevation_deg = Serial.parseFloat();
      elevation_rad = elevation_deg * PI / 180.0;
      pitchStepper.setTargetPosition(elevation_rad * STEPS_PER_RADIAN);
      Serial.println("Received pitch: " + String(elevation_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      break;
    }
  }
}

// Multi-try Manual Coordinate Testing function for bench checks
void testing_az_el_calculations() {
  char test_again_response;
  do {
    float test_lat = 0;
    float test_lon = 0;
    float test_height_m = 0;

    Serial.print("Enter Payload Latitude (degrees): ");
    while(true) {
      resetCommandTimeout();
      if(Serial.available() > 0) {
        test_lat = Serial.parseFloat();
        Serial.println(test_lat);
        delay(10); while(Serial.available() > 0) { Serial.read(); }
        break;
      }
      delay(100);
    }

    Serial.print("Enter Payload Longitude (degrees): ");
    while(true) {
      resetCommandTimeout();
      if(Serial.available() > 0) {
        test_lon = Serial.parseFloat();
        Serial.println(test_lon);
        delay(10); while(Serial.available() > 0) { Serial.read(); }
        break;
      }
      delay(100);
    }

    Serial.print("Enter Payload Height (meters): ");
    while(true) {
      resetCommandTimeout();
      if(Serial.available() > 0) {
        test_height_m = Serial.parseFloat();
        Serial.println(test_height_m);
        delay(10); while(Serial.available() > 0) { Serial.read(); }
        break;
      }
      delay(100);
    }

    float coordinate1[2] = { gs_latitude, gs_longitude };
    float coordinate2[2] = { test_lat, test_lon };
    float altitude1_ft = gs_height_m / 0.3048f;
    float altitude2_ft = test_height_m / 0.3048f;

    AzimuthResult result = azmuth_Range(coordinate1, coordinate2, altitude1_ft, altitude2_ft);

    float elevation = result.ElevationAngle;
    if(elevation < MIN_Pitch) elevation = MIN_Pitch;
    else if(elevation > MAX_Pitch) elevation = MAX_Pitch;

    float azimuth = result.ForwardAzimuth;
    double azimuth_rad = azimuth * PI / 180.0;
    double elevation_rad = elevation * PI / 180.0;

    resetCommandTimeout();
    yawStepper.setTargetPosition(azimuth_rad * STEPS_PER_RADIAN);
    pitchStepper.setTargetPosition(elevation_rad * STEPS_PER_RADIAN);

    Serial.println(F("----------------------------------------"));
    Serial.println(F("Manual Test Result (Vincenty's Formulae):"));
    Serial.print(F("  Distance (S): ")); Serial.print(result.S); Serial.println(F(" meters"));
    Serial.print(F("  Azimuth:      ")); Serial.print(azimuth); Serial.println(F(" degrees"));
    Serial.print(F("  Elevation:    ")); Serial.print(elevation); Serial.println(F(" degrees"));
    Serial.println(F("Steppers updated for test target.\n"));

    // Prompt to test another set of coordinates
    Serial.println("Would you like to test another set of coordinates? (y/n): ");
    while(true) {
      resetCommandTimeout();
      if(Serial.available() > 0) {
        test_again_response = Serial.read();
        delay(10); 
        while(Serial.available() > 0) { Serial.read(); }
        break;
      }
      delay(100);
    }
  } while (test_again_response == 'y' || test_again_response == 'Y');
}

void setup()
{
  Serial.begin(115200); 
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

  // --- INITIAL CALIBRATION SEQUENCE ---
  char response;
  do {
    Serial.println("Would you like to calibrate the azimuth and elevation by manually adjusting the yaw and pitch? (y/n): ");
    
    while(true){
      resetCommandTimeout();
      if(Serial.available() > 0){
        response = Serial.read();
        delay(10); 
        while(Serial.available() > 0) { Serial.read(); }
        if(response == 'y' || response == 'Y'){
          move_to_az_el();
          break; 
        } else {
          break;
        }
      } else {
        delay(100);
      }
    }
  } while (response == 'y' || response == 'Y');

  Serial.println("Would you like to set current position as the zero position for azimuth and elevation? (y/n): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      char cal_response = Serial.read();
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      if(cal_response == 'y' || cal_response == 'Y'){
        yawStepper.haltAndSetPosition(0);
        pitchStepper.haltAndSetPosition(0);
        Serial.println("Current position set as zero position for azimuth and elevation.");
      }
      break;
    } else {
      delay(100);
    }
  }

  // --- MANUAL COORDINATE TESTING PROMPT (MULTI-TRY) ---
  Serial.println("Would you like to test azimuth and elevation calculations by inputting new payload coordinates? (y/n): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      char test_response = Serial.read();
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      if(test_response == 'y' || test_response == 'Y'){
        testing_az_el_calculations();
      }
      break;
    } else {
      delay(100);
    }
  }
  // ----------------------------------------------------

  Serial.println(F("\nGround Station Initialized with Vincenty's Formulae.\n"));

  // Radio Defines
  Serial.println("LoRa Receiver Setup...");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
  }
  LoRa.setSyncWord(0xF3);   // must match transmitter
  LoRa.enableCrc();         // drop corrupted packets

  Serial.println("LoRa Receiver Ready! Waiting for telemetry...");
}

void loop() {
  // Check for incoming LoRa packets
  if (readLoRaMessage()) { 
    Serial.println("Read packet:");
    Serial.println(buffer);
    
    gps = extractGPS(buffer);

    // Only process tracking when a valid, new GPS coordinate update arrives
    if (gps.valid && ((gps.latitude != previous_x) || (gps.longitude != previous_y))) {
      lastPacketTime = millis(); // Reset link-loss timer

      Serial.println("GPS DATA FOUND");
      Serial.print("Altitude: "); Serial.println(gps.altitude);
      Serial.print("Latitude: "); Serial.println(gps.latitude, 6);
      Serial.print("Longitude: "); Serial.println(gps.longitude, 6);

      // Flash onboard LED to acknowledge valid packet
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);

      previous_y = gps.longitude;
      previous_x = gps.latitude;

      // Prepare coordinate arrays (AzimuthFull.h expects altitude inputs in feet)
      float coordinate1[2] = { gs_latitude, gs_longitude };
      float coordinate2[2] = { gps.latitude, gps.longitude };
      float altitude1_ft = gs_height_m / 0.3048f;
      float altitude2_ft = gps.altitude / 0.3048f;

      // Calculate using Vincenty's formulae via AzimuthFull.h
      AzimuthResult result = azmuth_Range(coordinate1, coordinate2, altitude1_ft, altitude2_ft);

      // Clamp elevation angle to safe operational bounds
      float elevation = result.ElevationAngle;
      if (elevation < MIN_Pitch) {
        elevation = MIN_Pitch;
      } else if (elevation > MAX_Pitch) {
        elevation = MAX_Pitch;
      }

      float azimuth = result.ForwardAzimuth;

      // Convert degrees to radians for motor step calculation
      double azimuth_rad = azimuth * PI / 180.0;
      double elevation_rad = elevation * PI / 180.0;

      // Send target positions to steppers
      resetCommandTimeout();
      yawStepper.setTargetPosition(azimuth_rad * STEPS_PER_RADIAN);
      pitchStepper.setTargetPosition(elevation_rad * STEPS_PER_RADIAN);

      // Print telemetry summary to Serial Monitor
      Serial.println(F("----------------------------------------"));
      Serial.println(F("Calculated via Vincenty's Formulae:"));
      Serial.print(F("  Distance (S): ")); Serial.print(result.S); Serial.println(F(" meters"));
      Serial.print(F("  Azimuth:      ")); Serial.print(azimuth); Serial.println(F(" degrees"));
      Serial.print(F("  Elevation:    ")); Serial.print(elevation); Serial.println(F(" degrees"));
      Serial.println(F("Steppers updated.\n"));
    }
  }

  // Check if we haven't received a packet in a while
  if (millis() - lastPacketTime > TIMEOUT_MS && lastPacketTime != 0) {
    Serial.println(F("WARNING: No LoRa packet received recently. Holding last position."));
    lastPacketTime = millis(); // Reset timer so it doesn't spam every loop cycle
  }

  delayWhileResettingCommandTimeout(50);
}