#include "Globals.h"
float pitchFiltered = 0;
float xAccFiltered = 0;
bool spinning = 0;
void imuLogic() {
  float xAcc, yAcc, zAcc, xGyro, yGyro, zGyro;
  IMU.readAcceleration(xAcc, yAcc, zAcc);
  IMU.readGyroscope(xGyro, yGyro, zGyro);
  filter.updateIMU(xGyro, yGyro, -zGyro, xAcc, yAcc, -zAcc);
  float pitch = filter.getPitch();
  //pitchFiltered = pitchFiltered * 0.99 + pitch * 0.01;
  pitchFiltered = pitchFiltered * 0.9 + pitch * 0.1;
  //xAccFiltered  = xAccFiltered  * 0.99 +  xAcc * 0.01;
  xAccFiltered  = xAccFiltered  * 0.9 +  xAcc * 0.1;
}
