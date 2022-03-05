#include "Globals.h"
float pitchRemoteFiltered = 0;
float xAccRemoteFiltered = 0;
bool spinning = 0;
void imuLogic() {
  float xAcc, yAcc, zAcc, xGyro, yGyro, zGyro;
  IMU.readAcceleration(xAcc, yAcc, zAcc);
  IMU.readGyroscope(xGyro, yGyro, zGyro);
  filter.updateIMU(xGyro, yGyro, -zGyro, xAcc, yAcc, -zAcc);
  float pitchRemote = filter.getPitch();
  pitchRemoteFiltered = pitchRemoteFiltered * 0.9 + pitchRemote * 0.1;
  xAccRemoteFiltered  = xAccRemoteFiltered  * 0.9 +  xAcc * 0.1;
}
