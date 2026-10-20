// // Each Tic's control mode must be set to "Serial/I2C/USB".  The
// // serial device number of one Tic must be set to its default
// // value of 14, and the serial device number of another Tic must
// // be set to 15.
// //
// // The GND pin of the Arduino must be be connected to a GND pin
// // on each Tic.  The TX pin of the Arduino must be connected to
// // the RX pins of each Tic.  This example does not read data, so
// // there is no need to connect the Tics' TX pins.
// //
// // See the comments and instructions in SerialSpeedControl.ino for
// // more information.

// #include <Tic.h>
// #include <math.h>
// #include <string.h>

// // On boards with a hardware serial port available for use, use
// // that port to communicate with the Tic. For other boards,
// // create a SoftwareSerial object using pin 10 to receive (RX)
// // and pin 11 to transmit (TX).
// #ifdef SERIAL_PORT_HARDWARE_OPEN
// #define ticSerial SERIAL_PORT_HARDWARE_OPEN
// #else
// #include <SoftwareSerial.h>
// SoftwareSerial ticSerial(10, 11);
// #endif

// #define MAXSPEED 1500 // in steps per second
// #define EARTH_RADIUS 6371000 // in meters
// #define MAX_Pitch 90 // in degrees
// #define MIN_Pitch 0 // in degrees

// TicSerial yawStepper(ticSerial, 14);
// TicSerial pitchStepper(ticSerial, 15);

// //Pinion has 16 teeth, stator gear has 80 teeth, so each rotation of base requires 80/16 = 5 rotations of the motor, which is 5*200 = 1000 steps
// //With planetary gear reduction of 1:100, each rotation of the output requires 100 rotations of the motor, which is 100*1000 = 100000 steps
// //So to move 360 degrees, we need to move 100000 steps.

// //Max speed is 3000 steps per second, to move 100000 steps, it will take 100000/3000 = 33.3 seconds, so we set the speed to 30000000 microsteps per 10000 seconds, which is 3000 steps per second.

// // Create an empty "bucket" to hold our incoming letters
// //Coordinates of the Ground Station Mini
// double gs_longitude = -112.45421 * PI / 180; // in radians
// double gs_latitude = 34.61292 * PI / 180; // in radians
// double gs_height = 1576.5993; // in meters

// //Coordinates of the Payload (initially set to the same as the Ground Station Mini, will be updated by the GPS data)
// double pay_longitude = gs_longitude; // in radians
// double pay_latitude = gs_latitude; // in radians
// double pay_height = gs_height; // in meters

// void resetCommandTimeout()
// {
//   yawStepper.resetCommandTimeout();
//   pitchStepper.resetCommandTimeout();
// }

// void delayWhileResettingCommandTimeout(uint32_t ms)
// {
//   uint32_t start = millis();
//   do
//   {
//     resetCommandTimeout();
//   } while ((uint32_t)(millis() - start) <= ms);
// }

// double calculate_azimuth(double long1, double long2, double lat1, double lat2)
// {
//   //Inputs:
//   // long1: longitude of the observer in radians
//   // long2: longitude of the target in radians
//   // lat1: latitude of the observer in radians
//   // lat2: latitude of the target in radians
//   //Output:
//   // azimuth: the angle from the observer to the target in radians, measured clockwise from north
//   //Purpose:
//   // This function calculates the azimuth from the observer to the target.
//   double dLong = long2 - long1;
//   double x = sin(dLong) * cos(lat2);
//   double y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLong);
//   double azimuth = atan2(x, y);
//   if (azimuth < 0)
//   {
//     azimuth += 2 * PI;
//   }
//   //Return in radians, measured clockwise from north
//   return azimuth;
// }

// double calculate_elevation(double long1, double long2, double lat1, double lat2, double h1, double h2)
// {
//   //Inputs:
//   // long1: longitude of the observer in radians
//   // long2: longitude of the target in radians
//   // lat1: latitude of the observer in radians
//   // lat2: latitude of the target in radians
//   // h1: height of the observer above sea level in meters
//   // h2: height of the target above sea level in meters
//   //Output:
//   // elevation: the angle from the observer to the target in radians, measured above the horizon
//   //Purpose:
//   // This function calculates the elevation from the observer to the target.
//   double dlat = lat2 - lat1;
//   double dlong = long2 - long1;
//   double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlong / 2) * sin(dlong / 2);
//   double c = 2 * atan2(sqrt(a), sqrt(1 - a));
//   double r1 = EARTH_RADIUS + h1;
//   double r2 = EARTH_RADIUS + h2;
//   // Calculate the elevation angle (this is a simplified version and may need adjustment based on the specific application)
//   double elevation = atan2(cos(c)-r1/r2, sin(c));
//   return elevation;
// }

// void testing_az_el_calculations(){
// //Ask user for new payload coordinates

//   // --- LONGITUDE ---
//   Serial.print("Longitude:");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       double pay_longitude_deg = Serial.parseFloat();
//       pay_longitude = pay_longitude_deg * PI / 180;
//       Serial.println("Received longitude: " + String(pay_longitude_deg) + " degrees");
      
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); // Wait a tiny split second to ensure all characters arrived
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
      
//       break; // Now break safely with an empty buffer!
//     }else{
//       delay(100); 
//     }
//   }

//   // --- LATITUDE ---
//   Serial.print("Latitude:");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       double pay_latitude_deg = Serial.parseFloat();
//       pay_latitude = pay_latitude_deg * PI / 180;
//       Serial.println("Received latitude: " + String(pay_latitude_deg) + " degrees");
      
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
      
//       break;
//     }else{
//       delay(100);
//     }
//   }

//   // --- HEIGHT ---
//   Serial.print("Height:");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       double pay_height_deg = Serial.parseFloat();
//       pay_height = pay_height_deg;
//       Serial.println("Received height: " + String(pay_height_deg) + " meters");
      
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
      
//       break;
//     }else{
//       delay(100);
//     }
//   }
  
//   // Print the calculated azimuth and elevation to the serial monitor for verification
//   Serial.println("Azimuth: " + String(calculate_azimuth(gs_longitude, pay_longitude, gs_latitude, pay_latitude) * 180 / PI) + " degrees");
//   Serial.println("Elevation: " + String(calculate_elevation(gs_longitude, pay_longitude, gs_latitude, pay_latitude, gs_height, pay_height) * 180 / PI) + " degrees");
// }

// void move_to_az_el(){
//   double azimuth_deg = 0;
//   double elevation_deg = 0;
//   double azimuth_rad = 0;
//   double elevation_rad = 0;

//   Serial.print("Set yaw to: ");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       azimuth_deg = Serial.parseFloat();
//       azimuth_rad = azimuth_deg * PI / 180;
//       yawStepper.setTargetPosition(azimuth_rad * 100000 / (2 * PI));
//       Serial.println("Received yaw: " + String(azimuth_deg) + " degrees");
      
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
      
//       break; // Now break safely with an empty buffer!
//     }
//   }
//   Serial.print("Set pitch to: ");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       elevation_deg = Serial.parseFloat();
//       elevation_rad = elevation_deg * PI / 180;
//       pitchStepper.setTargetPosition(elevation_rad * 100000 / (2 * PI));
//       Serial.println("Received pitch: " + String(elevation_deg) + " degrees");
      
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
      
//       break; // Now break safely with an empty buffer!
//     }

//   }
// }

// void setup()
// {
//   //Actual Setup
//   Serial.begin(9600);
//   // THE FIX: Stop parseInt from freezing the Arduino!
//   Serial.setTimeout(200);

//   ticSerial.begin(9600);
//   delay(20);

//   yawStepper.exitSafeStart();
//   pitchStepper.exitSafeStart();

//   // Add these lines to enforce your max speed:
//   yawStepper.setMaxSpeed((uint32_t)MAXSPEED * 10000ULL);
//   pitchStepper.setMaxSpeed((uint32_t)MAXSPEED * 10000ULL);

//   //Recenter both stepper motors to the initial position (pointing straight up)
//   yawStepper.setTargetPosition(0);
//   pitchStepper.setTargetPosition(0);
//   delayWhileResettingCommandTimeout(5000); // Wait for the motors to move to the

//   char response;
//   do{
//     Serial.print("Would you like to calibrate the azimuth and elevation by manually adjusting the yaw and pitch? (y/n): ");
//     while(true){
//       resetCommandTimeout();
//       if(Serial.available() > 0){
//         response = Serial.read();
//         // THE FIX: Vacuum up the leftover 'Enter' key characters
//         delay(10); 
//         while(Serial.available() > 0) { 
//           Serial.read(); 
//         }
//         if(response == 'y' || response == 'Y'){
//           move_to_az_el();
//         }else{
//           break;
//         }
//       }else{
//         delay(100);
//       }
//     }
//   }while (response == 'y' || response == 'Y');

//   Serial.print("Would you like to set current position as the zero position for azimuth and elevation? (y/n): ");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       char response = Serial.read();
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
//       if(response == 'y' || response == 'Y'){
//         yawStepper.haltAndSetPosition(0);
//         pitchStepper.haltAndSetPosition(0);
//         Serial.println("Current position set as zero position for azimuth and elevation.");
//       }
//       break;
//     }else{
//       delay(100);
//     }
//   }

//   Serial.println("Would you like to test azimuth and elevation calculations by inputting new payload coordinates? (y/n): ");
//   while(true){
//     resetCommandTimeout();
//     if(Serial.available() > 0){
//       response = Serial.read();
//       // THE FIX: Vacuum up the leftover 'Enter' key characters
//       delay(10); 
//       while(Serial.available() > 0) { 
//         Serial.read(); 
//       }
//       if(response == 'y' || response == 'Y'){
//         testing_az_el_calculations();
//       }

//       break;
//     }else{
//       delay(100);
//     }
//   }
// }

// void loop() {
//   //double azimuth = calculate_azimuth(gs_longitude, pay_longitude, gs_latitude, pay_latitude);
//   //double elevation = calculate_elevation(gs_longitude, pay_longitude, gs_latitude, pay_latitude, gs_height, pay_height);
//   double azimuth = 0;
//   double elevation = 0;
//   //make sure the elevation is between the minimum and maximum pitch angles
//   if(elevation < MIN_Pitch * PI / 180){
//     elevation = MIN_Pitch * PI / 180;
//   }
//   else if(elevation > MAX_Pitch * PI / 180){
//     elevation = MAX_Pitch * PI / 180;
//   }
//   //set the target position of the yaw and pitch steppers based on the calculated azimuth and elevation
//   resetCommandTimeout();
//   yawStepper.setTargetPosition(azimuth * 100000 / (2 * PI));
//   pitchStepper.setTargetPosition(elevation * 100000 / (2 * PI));
//   Serial.println("Set yaw to " + String(azimuth * 180 / PI) + " degrees and pitch to " + String(elevation * 180 / PI) + " degrees.");
//   delayWhileResettingCommandTimeout(20000);

//   azimuth = 180 * PI / 180;
//   elevation = 85 * PI / 180;
//   //make sure the elevation is between the minimum and maximum pitch angles
//   if(elevation < MIN_Pitch * PI / 180){
//     elevation = MIN_Pitch * PI / 180;
//   }
//   else if(elevation > MAX_Pitch * PI / 180){
//     elevation = MAX_Pitch * PI / 180;
//   }
//   //set the target position of the yaw and pitch steppers based on the calculated azimuth and elevation
//   resetCommandTimeout();
//   yawStepper.setTargetPosition(azimuth * 100000 / (2 * PI));
//   pitchStepper.setTargetPosition(elevation * 100000 / (2 * PI));
//   Serial.println("Set yaw to " + String(azimuth * 180 / PI) + " degrees and pitch to " + String(elevation * 180 / PI) + " degrees.");
//   delayWhileResettingCommandTimeout(20000);
// }