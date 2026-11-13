/* Azimuth.h
//
//
// CURRENTLY UNUSED PLEASE REFER TO AZIMUTH FULL WHICH IS USED
//
//
// Used for decleration of the azimuth struct and function
// Used in Arduino_Ground_Station.ino
*/

#ifndef AZIMUTH_H
#define AZIMUTH_H

typedef struct {
  float S;
  float ForwardAzimuth;  // Gets Azimuth in DEG's
  float ElevationAngle;  // Gets Angle in DEG's
} AzimuthResult;

// Function prototype
AzimuthResult azmuth_Range(double coordinate1[2], double coordinate2[2], float altitude1, float altitude2);

#endif
