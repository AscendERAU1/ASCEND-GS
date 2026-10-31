/*
// Arduino_Ground_Station.ino 
//
// ORDER OF OPERATIONS
//  Upload/Turn on arduino and plug into computer
//  Type coordinates of ground station into serial montior (alternatively change values for coordinate 2)
//  Plug in and turn on Featherweight ground station
//  Plug in and turn on Featherweight Tracker
//  Start C script to forward serial
//  
//
// This is a script used for the Embry-Riddle Areonautical University Acsend Ground station
// should be used for the Arduino Uno ground station, used with the Featherweight GPS Tracker
//
// Description:
//  This takes in a serial message parses it into the Example Message* and extracts latitude, longitude, and altitude.
//  It then takes those values does "Azimuth Range" calculations to get the movement of the Ground Station. Then sends
//  the info to the third and final part of the script that moves the Ground Station to look at the Payload or GPS Sender.
//
// Features:
//  Blinks a light when it receives the correct serial message (should look like Example Message*)
//  Able to switch from automatic tracking and manual tracking with the switch of either of the labled levers
//  Will send out serial of the received message can send serial of the received alt lat and long
//
//  CO-REQS (run with)
//  Serial_Read_Send_(LINUX or WINDOWS).c
//
// Example Message*(Explained)
// @ GPS_STAT 202 0000 00 00 00:17:06.570 CRC_OK  TRK GPSTrk05169 Alt 000000 lt +00.00000 ln +00.00000 Vel +0000 +000 +0000 Fix 0 #  0  0  0  0 000_00_00 000_00_00 000_00_00 000_00_00 000_00_00 CRC: 5F95
// Start message //Placeholder //Time     //CRC check not corrupt           //Latitude   //Longitude //Velocity           //GPS Fix type      //Additional raw satellite or time data (format placeholder)
//Length of message                 //ID of Tracker             //Altitude                                                     //Sat tracking info                                       //Check for packet
//IMPORTANT ONES:                                                                                 || The rest is unneeded
// @ GPS_STAT                  :TIME                              ALT        LT           LN                                                                            (END POINT DELIMINATOR)   CRC: 
// Checking for serial will be used for featherweight
//
// Azimuth Range(description)
// 
// Azmuth_range: Calculates the distance and the altitude.
//
// Takes in a coordinate1, coordinate2, altitude1 and altitude2 to get the said calculated values 
//
// Coord1 and coord2 is formatted: Lat, Long


*/



// Library Defines
#include <Arduino.h>

//Azimuth Range defines
#include <math.h>
#include "AzimuthFull.h"

//Servo Control Defines
#include <Wire.h>
#include <Tic.h>

// Defines for GPS Parsing

/* NOT USED CURRENTLY need to parse time and date to use
typedef struct {
  double altitude;
  double latitude;
  double longitude;
  double date; 
  double time;
  bool valid;
} GPSData;
*/


#define LIGHT 12
#define BUFFER_SIZE 350

// Struct for parsing
typedef struct {
  float altitude;
  float latitude;
  float longitude;
  bool valid;
} GPSData;

float x_value, y_value, previous_y = 0.0, previous_x = 0.0;

char buffer[BUFFER_SIZE];
uint8_t bufIndex = 0;

int32_t xSteps = 0;
int32_t ySteps = 0;

enum ControlMode { MANUAL, AUTO };
ControlMode controlMode = MANUAL;

  //Lever defines

  // Lever pins
const int joyLeverPin = 4;
const int zeroLeverPin = 5;
const int debugPin = 3;

// Lever states
bool joylever = false;
bool zerolever = false;
static bool lastZeroLever = false;
//debug defines
float promptalt, promptlong, promptlat;

bool readSerialMessage() {
  while (Serial.available()) {
    char c = Serial.read();

    // Prevent buffer overflow
    if (bufIndex < BUFFER_SIZE - 1) {
      buffer[bufIndex++] = c;
      buffer[bufIndex] = '\0';
    }

    // End of message detected
    if (strstr(buffer, "CRC:") != NULL) {

      return true;  // buffer now contains full message
    }
  }
  return false;  // no complete message yet
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

//Servo control
// Defines
// Define Tic controllers with their addresses
TicI2C tic1(14);  // Address for the first Tic (motor X)
TicI2C tic2(15);  // Address for the second Tic (motor Y)

// Constants for stepper motor parameters
const long stepsPerRevolution = 45701;  // Set this to your motor's steps per 360° rotation
long speedX = 0;
long speedY = 0;
int joyCenter = 512;
int deadzone = 200;
int lowerDeadzone = joyCenter - deadzone;
int upperDeadzone = joyCenter + deadzone;

// Joystick pins
const int joystickXPin = A0;
const int joystickYPin = A1;

// Joystick settings
//const int centerThreshold = 100;  // Deadzone threshold around center position
const long maxSpeed = 20000000;   // Max speed for the motor in steps per second (adjust as necessary)

// Servo Commands defines
// Sends a "Reset command timeout" command to both Tic controllers
void resetCommandTimeout() {
  tic1.resetCommandTimeout();
  tic2.resetCommandTimeout();
}

void SetZeroPosition() {
  tic1.haltAndSetPosition(0);  // Set current position of tic1 to 0
  tic2.haltAndSetPosition(0);  // Set current position of tic1 to 0
  tic1.exitSafeStart();
  tic2.exitSafeStart();
}

// Delays while resetting command timeout to avoid interruption
void delayWhileResettingCommandTimeout(uint32_t ms) {
  uint32_t start = millis();
  do {
    resetCommandTimeout();
  } while ((uint32_t)(millis() - start) <= ms);
}

void waitForPosition(TicI2C &ticController, int32_t targetPosition) {
  do {
    resetCommandTimeout();
  } while (ticController.getCurrentPosition() != targetPosition);
}


// Convert the angle to steps and set the target position for the specified motor
void setMotorPosition(char motor, float angle) {
  int32_t steps = round((angle * stepsPerRevolution) / 360);  // Convert angle to steps
                                                       // Note that the following sets a new zero point after each movemement for testing purposes. In future, use calibration message to set zero and move from there.
  if (motor == 'X') {
    tic1.setTargetPosition(steps);  // Move tic1 (X motor) to the calculated position
  } else if (motor == 'Y') {
    tic2.setTargetPosition(steps);  // Move tic2 (Y motor) to the calculated position
  }
}

// Adjust motor speeds based on joystick input
void controlVelocityWithJoystick() {
  int joystickX = analogRead(joystickXPin);
  int joystickY = analogRead(joystickYPin);

  if ((joystickX >= lowerDeadzone) && (joystickX <= upperDeadzone)) {
    speedX = 0;
  } else if (joystickX < lowerDeadzone) {
    speedX = -maxSpeed;
  } else if (joystickX > upperDeadzone) {
    speedX = maxSpeed;
  }
  if ((joystickY >= lowerDeadzone) && (joystickY <= upperDeadzone)) {
    speedY = 0;
  } else if (joystickY < lowerDeadzone) {
    speedY = -maxSpeed;
  } else if (joystickY > upperDeadzone) {
    speedY = maxSpeed;
  }


  // Set motor speeds
  tic1.setTargetVelocity(speedX);
  tic2.setTargetVelocity(speedY);
/*
  Serial.print("Joystick X velocity: ");
  Serial.print(speedX);
  Serial.println(" steps/s");
  Serial.print("Joystick Y velocity: ");
  Serial.print(speedY);
  Serial.println(" steps/s");
  */
  // Ensure the Tic does not timeout
  delayWhileResettingCommandTimeout(100);
}


bool joyleverCheck() {
  return digitalRead(joyLeverPin) == LOW;  // flipped = LOW
}

bool zeroleverCheck() {
  return digitalRead(zeroLeverPin) == LOW;  // flipped = LOW
}

bool debugpincheck(){
  return digitalRead(debugPin) == LOW; // flipped = low
}


void setup() {
  Serial.begin(115200);
  Serial.println(F("\nStarting... \n"));

  Wire.begin();

  delay(1000);             // delay to make sure serial is established
  pinMode(LIGHT, OUTPUT);  //Light for serial com

  // DEFINE SWITCHES AND LIGHTS RESPECIVELY HERE

  // Initialize Tic controllers and exit safe start mode
  tic1.haltAndSetPosition(0);  // Set current position of tic1 to 0
  tic2.haltAndSetPosition(0);  // Set current position of tic2 to 0
  tic1.exitSafeStart();
  tic2.exitSafeStart();
  Serial.println(F("tic initiated"));

  // Lever pins
  pinMode(joyLeverPin, INPUT_PULLUP);
  Serial.println(F("joy pin started"));
  pinMode(zeroLeverPin, INPUT_PULLUP);
  Serial.println(F("zero pin started"));
  pinMode(debugPin, INPUT_PULLUP);
  Serial.println(F("debug pin started"));
}

/* If ram is a problem here is a ram monitor

extern int __heap_start, *__brkval;
int freeRam() {
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

//How to call
Serial.print(F("Free RAM: "));
Serial.println(freeRam());

*/

void loop() {

  static GPSData gps;
  static AzimuthResult output;

  // Scanning Serial
  if (readSerialMessage()) {
    Serial.println("Received:");
    Serial.println(buffer);

    //Parsing GPS Data
    gps = extractGPS(buffer);

    // Clear buffer for next message
    bufIndex = 0;
    buffer[0] = '\0';
  }


  // ALL FUNCTIONS FOR WHEN GPS HAS BEEN CALLED
  // Includes:
  // Gps parsing, Azimuth Range
  if (gps.valid && ((gps.latitude != previous_x) || (gps.longitude != previous_y))) {

    Serial.println("GPS DATA FOUND");
    Serial.print("Altitude: ");
    /*IF RAM IS A PROBLEM 
    Serial.println(F("Your message (this will work with print normal and the vars)"))
    */
    Serial.println(gps.altitude);
    Serial.print("Latitude: ");
    Serial.println(gps.latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(gps.longitude, 6);

    //Blink Light
    digitalWrite(LIGHT, HIGH);
    digitalWrite(LIGHT, LOW);

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

  // checking states of lever
  joylever = joyleverCheck();
  zerolever = zeroleverCheck();

  if (joylever && controlMode== AUTO){
    controlMode= MANUAL;
  }

  //checking zero lever state
  if (zerolever && !lastZeroLever) {
    SetZeroPosition();
    controlMode = AUTO;
  }

  lastZeroLever=zerolever;

  if (controlMode== MANUAL && joylever){
    controlVelocityWithJoystick();
  }

  if (controlMode== AUTO && gps.valid) {

    setMotorPosition('X', x_value);  // Move X motor
    setMotorPosition('Y', -y_value);  // Move Y motor

  }

  resetCommandTimeout();  // Reset command timeout to avoid Tic shutdown needs to go last
}