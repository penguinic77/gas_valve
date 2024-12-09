#ifndef NETWORK_H
#define NETWORK_H

bool get_group_info(char *groupname, int *mac_count, char *mac_list);
bool get_sw_state(const char *token, const char *gasname);
void update_sw_state(const char *token, const char *gasname, bool sw_state, bool emergency);
void upload_sensor_data(float temp, float voltage, float fire, float gas);
void network_tasks(void *pvParameters);

#endif
