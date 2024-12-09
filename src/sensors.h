#ifndef SENSORS_H
#define SENSORS_H

float read_fire_sensor();
float read_battery_voltage();
float read_gas_ppm();
float read_temperature();
void i2c_master_init(void);

#endif
