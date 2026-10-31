#include <Tic.h>
#include <math.h>
#include <string.h>
#include "AzimuthFull.h"

// Use hardware Serial1 for the Arduino Nano ESP32 connected to TX1
#define ticSerial Serial1

#define MAXSPEED 1500 // in steps per second
#define MAX_Pitch 90  // in degrees
#define MIN_Pitch 0   // in degrees

TicSerial yawStepper(ticSerial, 14);
TicSerial pitchStepper(ticSerial, 15);

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

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(200);

  // Explicitly map TX to pin D8 so data actually leaves the board
  ticSerial.begin(9600, SERIAL_8N1, -1, D8);
  delay(20);

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

  // --- LATITUDE PROMPT ---
  Serial.print("Enter Payload Latitude (degrees): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      pay_latitude = Serial.parseFloat();
      Serial.println(pay_latitude);
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      break;
    }
    delay(100);
  }

  // --- LONGITUDE PROMPT ---
  Serial.print("Enter Payload Longitude (degrees): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      pay_longitude = Serial.parseFloat();
      Serial.println(pay_longitude);
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      break;
    }
    delay(100);
  }

  // --- HEIGHT PROMPT ---
  Serial.print("Enter Payload Height (meters): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      pay_height_m = Serial.parseFloat();
      Serial.println(pay_height_m);
      delay(10); 
      while(Serial.available() > 0) { Serial.read(); }
      break;
    }
    delay(100);
  }

  // Prepare coordinate arrays
  // Note: AzimuthFull.h expects altitude inputs in feet (it internally converts them via * 0.3048f)
  float coordinate1[2] = { gs_latitude, gs_longitude };
  float coordinate2[2] = { pay_latitude, pay_longitude };
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