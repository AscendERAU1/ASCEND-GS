#include <Tic.h>
#include <math.h>
#include <string.h>

// Use hardware Serial1 for the Arduino Nano ESP32 connected to TX1
#define ticSerial Serial1

#define MAXSPEED 1500 // in steps per second
#define EARTH_RADIUS 6371000 // in meters
#define MAX_Pitch 90 // in degrees
#define MIN_Pitch 0 // in degrees

TicSerial yawStepper(ticSerial, 14);
TicSerial pitchStepper(ticSerial, 15);

// Coordinates of the Ground Station Mini
double gs_longitude = -112.45421 * PI / 180; // in radians
double gs_latitude = 34.61292 * PI / 180; // in radians
double gs_height = 1576.5993; // in meters

// Coordinates of the Payload
double pay_longitude = gs_longitude; // in radians
double pay_latitude = gs_latitude; // in radians
double pay_height = gs_height; // in meters

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

double calculate_azimuth(double long1, double long2, double lat1, double lat2)
{
  double dLong = long2 - long1;
  double x = sin(dLong) * cos(lat2);
  double y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLong);
  double azimuth = atan2(x, y);
  if (azimuth < 0)
  {
    azimuth += 2 * PI;
  }
  return azimuth;
}

double calculate_elevation(double long1, double long2, double lat1, double lat2, double h1, double h2)
{
  double dlat = lat2 - lat1;
  double dlong = long2 - long1;
  double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlong / 2) * sin(dlong / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double r1 = EARTH_RADIUS + h1;
  double r2 = EARTH_RADIUS + h2;
  double elevation = atan2(cos(c)-r1/r2, sin(c));
  return elevation;
}

void testing_az_el_calculations(){
  // --- LONGITUDE ---
  Serial.print("Longitude:");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      double pay_longitude_deg = Serial.parseFloat();
      pay_longitude = pay_longitude_deg * PI / 180;
      Serial.println("Received longitude: " + String(pay_longitude_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      break;
    }else{
      delay(100); 
    }
  }

  // --- LATITUDE ---
  Serial.print("Latitude:");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      double pay_latitude_deg = Serial.parseFloat();
      pay_latitude = pay_latitude_deg * PI / 180;
      Serial.println("Received latitude: " + String(pay_latitude_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      break;
    }else{
      delay(100);
    }
  }

  // --- HEIGHT ---
  Serial.print("Height:");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      double pay_height_deg = Serial.parseFloat();
      pay_height = pay_height_deg;
      Serial.println("Received height: " + String(pay_height_deg) + " meters");
      
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      break;
    }else{
      delay(100);
    }
  }
  
  Serial.println("Azimuth: " + String(calculate_azimuth(gs_longitude, pay_longitude, gs_latitude, pay_latitude) * 180 / PI) + " degrees");
  Serial.println("Elevation: " + String(calculate_elevation(gs_longitude, pay_longitude, gs_latitude, pay_latitude, gs_height, pay_height) * 180 / PI) + " degrees");
}

void move_to_az_el(){
  double azimuth_deg = 0;
  double elevation_deg = 0;
  double azimuth_rad = 0;
  double elevation_rad = 0;

  Serial.print("Set yaw to: ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      azimuth_deg = Serial.parseFloat();
      azimuth_rad = azimuth_deg * PI / 180;
      yawStepper.setTargetPosition(azimuth_rad * 100000 / (2 * PI));
      Serial.println("Received yaw: " + String(azimuth_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      break;
    }
  }
  
  Serial.print("Set pitch to: ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      elevation_deg = Serial.parseFloat();
      elevation_rad = elevation_deg * PI / 180;
      pitchStepper.setTargetPosition(elevation_rad * 100000 / (2 * PI));
      Serial.println("Received pitch: " + String(elevation_deg) + " degrees");
      
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      break;
    }
  }
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

  char response;
  do{
    // Using println so the text flushes to the serial monitor immediately
    Serial.println("Would you like to calibrate the azimuth and elevation by manually adjusting the yaw and pitch? (y/n): ");
    
    while(true){
      resetCommandTimeout();
      if(Serial.available() > 0){
        response = Serial.read();
        delay(10); 
        while(Serial.available() > 0) { 
          Serial.read(); 
        }
        if(response == 'y' || response == 'Y'){
          move_to_az_el();
          break; // <-- CRITICAL FIX: Break out of the inner loop after moving so it can re-prompt!
        }else{
          break;
        }
      }else{
        delay(100);
      }
    }
  }while (response == 'y' || response == 'Y');

  Serial.println("Would you like to set current position as the zero position for azimuth and elevation? (y/n): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      char response = Serial.read();
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      if(response == 'y' || response == 'Y'){
        yawStepper.haltAndSetPosition(0);
        pitchStepper.haltAndSetPosition(0);
        Serial.println("Current position set as zero position for azimuth and elevation.");
      }
      break;
    }else{
      delay(100);
    }
  }

  Serial.println("Would you like to test azimuth and elevation calculations by inputting new payload coordinates? (y/n): ");
  while(true){
    resetCommandTimeout();
    if(Serial.available() > 0){
      response = Serial.read();
      delay(10); 
      while(Serial.available() > 0) { 
        Serial.read(); 
      }
      if(response == 'y' || response == 'Y'){
        testing_az_el_calculations();
      }
      break;
    }else{
      delay(100);
    }
  }
}

void loop() {
  double azimuth = 0;
  double elevation = 0;

  if(elevation < MIN_Pitch * PI / 180){
    elevation = MIN_Pitch * PI / 180;
  }
  else if(elevation > MAX_Pitch * PI / 180){
    elevation = MAX_Pitch * PI / 180;
  }

  resetCommandTimeout();
  yawStepper.setTargetPosition(azimuth * 100000 / (2 * PI));
  pitchStepper.setTargetPosition(elevation * 100000 / (2 * PI));
  Serial.println("Set yaw to " + String(azimuth * 180 / PI) + " degrees and pitch to " + String(elevation * 180 / PI) + " degrees.");
  delayWhileResettingCommandTimeout(20000);

  azimuth = 180 * PI / 180; //convert to Radians
  elevation = 85 * PI / 180; //convert to Radians
  if(elevation < MIN_Pitch * PI / 180){
    elevation = MIN_Pitch * PI / 180;
  }
  else if(elevation > MAX_Pitch * PI / 180){
    elevation = MAX_Pitch * PI / 180;
  }

  resetCommandTimeout();
  yawStepper.setTargetPosition(azimuth * 100000 / (2 * PI));
  pitchStepper.setTargetPosition(elevation * 100000 / (2 * PI));
  Serial.println("Set yaw to " + String(azimuth * 180 / PI) + " degrees and pitch to " + String(elevation * 180 / PI) + " degrees.");
  delayWhileResettingCommandTimeout(20000);
}