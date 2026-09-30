#include <Adafruit_QMC5883P.h>
#include <math.h>

Adafruit_QMC5883P qmc;

// Look up your local magnetic declination (NOAA calculator), in degrees.
// East is positive.
const float DECLINATION_DEG = 0.0;

// Hard-iron calibration offsets (Gauss), filled in by calibration below
float offX = 0, offY = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  if (!qmc.begin()) {
    Serial.println("Failed to find QMC5883P chip");
    while (1) delay(10);
  }

  qmc.setMode(QMC5883P_MODE_NORMAL);
  qmc.setODR(QMC5883P_ODR_50HZ);
  qmc.setOSR(QMC5883P_OSR_4);
  qmc.setDSR(QMC5883P_DSR_2);
  qmc.setRange(QMC5883P_RANGE_8G);   // fixed range, don't change it
  qmc.setSetResetMode(QMC5883P_SETRESET_ON);

  calibrate();
}

void calibrate() {
  Serial.println("Calibrating: slowly rotate the sensor through all directions for 15 s...");
  float minX = 1e6, maxX = -1e6, minY = 1e6, maxY = -1e6;
  unsigned long start = millis();

  while (millis() - start < 15000) {
    float gx, gy, gz;
    if (qmc.isDataReady() && qmc.getGaussField(&gx, &gy, &gz)) {
      minX = min(minX, gx); maxX = max(maxX, gx);
      minY = min(minY, gy); maxY = max(maxY, gy);
    }
    delay(20);
  }

  offX = (maxX + minX) / 2.0;
  offY = (maxY + minY) / 2.0;
  Serial.println("Calibration done.");
}

void loop() {
  float gx, gy, gz;

  if (qmc.isDataReady() && qmc.getGaussField(&gx, &gy, &gz)) {
    float x = gx - offX;
    float y = gy - offY;

    float heading = atan2(y, x) * 180.0 / PI;   // -180..180
    heading += DECLINATION_DEG;

    if (heading < 0)    heading += 360.0;
    if (heading >= 360) heading -= 360.0;

    Serial.print("Azimuth: ");
    Serial.print(heading, 1);
    Serial.println(" deg");
  }

  delay(100);
}