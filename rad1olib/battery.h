#ifndef BATTERY_H
#define BATTERY_H

void batteryVoltageCheck(void);
uint32_t batteryGetVoltage(void);
bool batteryCharging(void);
void batteryInit(void);

#endif
