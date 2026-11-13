/* Azimuth.h
//
//=======================================================
//=++++++++++++++++++ Azimuth Range +++++++++++++++++++++
//=======================================================
//
// Used for decleration of the azimuth struct and function
// Used in Arduino_Ground_Station.ino
//
// If used remove the math from the main ino 
// 
// This is different from the Azimuth.ino as it is the whole math included
//
// Altitude is expected in Feet
//
*/
#ifndef AZIMUTH_H
#define AZIMUTH_H

#include <Arduino.h>
#include <math.h>

/* =================== STRUCT =================== */
typedef struct {
  float S;
  float ForwardAzimuth;   // degrees
  float ElevationAngle;   // degrees
} AzimuthResult;

/* =================== CONSTANTS =================== */
static const float AZ_A = 6378137.0f;              // semi-major axis (m)
static const float AZ_F = 1.0f / 298.257223563f;   // flattening
static const int   AZ_MAX_ITER = 200;

/* =================== FUNCTION =================== */
static inline AzimuthResult azmuth_Range(
    const float coordinate1[2],
    const float coordinate2[2],
    float altitude1,
    float altitude2)
{
  AzimuthResult result;

  float b = (1.0f - AZ_F) * AZ_A;
  float tol = expf(-12.0f);

  float phi_1, phi_2, U_1, U_2;
  float L, L_1, L_2, lamda;
  float sin_lamda, cos_lamda;
  float sin_sigma, cos_sigma, sigma;
  float sin_alpha, cos2_alpha, cos2sigmam;
  float C, lamda_prev, s;
  float sin_U1, sin_U2, cos_U1, cos_U2;

  float h1 = altitude1 * 0.3048f;
  float h2 = altitude2 * 0.3048f;


  /* ---- Identical points check ---- */
  if (coordinate1[0] == coordinate2[0] &&
      coordinate1[1] == coordinate2[1]) {
    result.S = 0;
    result.ForwardAzimuth = 0;
    result.ElevationAngle = 0;
    return result;
  }

  /* ---- Convert degrees to radians ---- */
  phi_1 = coordinate1[0] * PI / 180.0f;
  L_1   = coordinate1[1] * PI / 180.0f;
  phi_2 = coordinate2[0] * PI / 180.0f;
  L_2   = coordinate2[1] * PI / 180.0f;

  U_1 = atan((1 - AZ_F) * tan(phi_1));
  U_2 = atan((1 - AZ_F) * tan(phi_2));

  L = L_2 - L_1;
  lamda = L;

  sin_U1 = sin(U_1);
  cos_U1 = cos(U_1);
  sin_U2 = sin(U_2);
  cos_U2 = cos(U_2);

  /* ---- Vincenty iteration ---- */
  for (int i = 0; i < AZ_MAX_ITER; i++) {
    sin_lamda = sin(lamda);
    cos_lamda = cos(lamda);

    float t1 = cos_U2 * sin_lamda;
    float t2 = cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lamda;
    sin_sigma = sqrtf(t1 * t1 + t2 * t2);

    if (sin_sigma == 0) break;  // prevent divide by zero

    cos_sigma = sin_U1 * sin_U2 + cos_U1 * cos_U2 * cos_lamda;
    sigma = atan2f(sin_sigma, cos_sigma);

    sin_alpha = (cos_U1 * cos_U2 * sin_lamda) / sin_sigma;
    cos2_alpha = 1 - sin_alpha * sin_alpha;

    cos2sigmam = cos_sigma - 2 * sin_U1 * sin_U2 / cos2_alpha;
    C = (AZ_F / 16) * cos2_alpha * (4 + AZ_F * (4 - 3 * cos2_alpha));

    lamda_prev = lamda;
    lamda = L + (1 - C) * AZ_F * sin_alpha *
            (sigma + C * sin_sigma *
            (cos2sigmam + C * cos_sigma *
            (-1 + 2 * cos2sigmam * cos2sigmam)));

    if (fabsf(lamda - lamda_prev) <= tol) break;
  }

  /* ---- Distance ---- */
  float u2 = cos2_alpha * (AZ_A * AZ_A - b * b) / (b * b);
  float A = 1 + u2 / 16384 *
            (4096 + u2 * (-768 + u2 * (320 - 175 * u2)));
  float B = u2 / 1024 *
            (256 + u2 * (-128 + u2 * (74 - 47 * u2)));

  float delta_sigma = B * sin_sigma *
    (cos2sigmam + B / 4 *
    (cos_sigma * (-1 + 2 * cos2sigmam * cos2sigmam) -
     B / 6 * cos2sigmam *
     (-3 + 4 * sin_sigma * sin_sigma) *
     (-3 + 4 * cos2sigmam * cos2sigmam)));

  s = b * A * (sigma - delta_sigma);

  result.S = s;
  result.ForwardAzimuth =
    atan2f(cos_U2 * sin_lamda,
           cos_U1 * sin_U2 - sin_U1 * cos_U2 * cos_lamda)
    * 180.0f / PI;

  /* ---- Elevation angle ---- */
  float d = result.S;

  result.ElevationAngle =
  atan2f(h2 - h1, d) * 180.0f / PI;

  /*
  float d = sqrtf((AZ_A + h1) * (AZ_A + h1) + (AZ_A + h2) * (AZ_A + h2) - 2 * (AZ_A + h1) * (AZ_A + h2) * cos(sigma) );
  result.ElevationAngle =
    atan2f(h2 - h1, d) * 180.0f / PI;
*/
  return result;
}

#endif

