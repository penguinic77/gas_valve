#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define WIFI_SSID "NicAP"
#define WIFI_PASSWORD "12348765"
#define DHTPIN GPIO_NUM_32
#define MQ_SENSOR GPIO_NUM_33
#define TOKEN "678E82D907D3E6E71F81D5CF3DDACC3671DC618C38A1B7A9F9393A83D025B296"
#define GASNAME "gas1"

#define I2C_MASTER_SCL_IO 19
#define I2C_MASTER_SDA_IO 18
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define ADS1115_ADDR 0x48

// 感測器訊息結構
typedef struct {
    int id;
    float weight;
    bool earthquake;
} sensor_message_t;

typedef struct {
    bool dangerous;
} state_message_t;

extern sensor_message_t myData; 
extern state_message_t group;

extern bool motor_state; 
extern bool emerg;      
extern bool group_flag; 

#endif
