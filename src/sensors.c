#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "dht.h"
#include "config.h"
#include "sensors.h"

static const char *TAG="SENSORS";

#define CONFIG_REGISTER 0x01
#define CONVERSION_REGISTER 0x00

static esp_err_t ads1115_set_config(uint16_t config) {
    uint8_t data[3] = {
        CONFIG_REGISTER,
        (uint8_t)(config >> 8),
        (uint8_t)(config & 0xFF)
    };
    return i2c_master_write_to_device(I2C_MASTER_NUM, ADS1115_ADDR,
                                      data, sizeof(data),
                                      pdMS_TO_TICKS(1000));
}

static int16_t ads1115_read_conversion(void) {
    uint8_t data[2];
    i2c_master_write_to_device(I2C_MASTER_NUM, ADS1115_ADDR,
                                (uint8_t[]){CONVERSION_REGISTER}, 1,
                                pdMS_TO_TICKS(1000));

    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, ADS1115_ADDR,
                                                data, sizeof(data),
                                                pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "讀取ADC失敗: %s", esp_err_to_name(ret));
        return 0;
    }
    return ((int16_t)data[0] << 8) | data[1];
}

float read_fire_sensor() {
    uint16_t config = 0xC383; 
    ads1115_set_config(config);
    vTaskDelay(pdMS_TO_TICKS(10)); 
    int16_t raw = ads1115_read_conversion();
    float voltage = raw * 4.096 / 32768.0;
    return voltage * 1000;
}

float read_battery_voltage() {
    uint16_t config = 0xC083; 
    ads1115_set_config(config);
    vTaskDelay(pdMS_TO_TICKS(10)); 
    int16_t raw = ads1115_read_conversion();
    float v = raw * 4.096 / 32768.0; 
    float vout = (v - 11.1)/2.25*100.0;
    if(vout<0) vout=0;
    if(vout>100) vout=100;
    return vout;
}

float read_gas_ppm() {
    uint16_t config = 0xC183; 
    ads1115_set_config(config);
    vTaskDelay(pdMS_TO_TICKS(10)); 
    int16_t raw = ads1115_read_conversion();
    float voltage = raw * 4.096 / 32768.0;

    float RL = 20.0; 
    float Ro = 100.0; 
    float Rs = ((5.0 * RL) / voltage) - RL; 
    float ratio = Rs / Ro;
    float ppm = pow(10, ((log10(ratio) - 1.33) / -0.611));
    return ppm;
}

float read_temperature() {
    float temperature = 0.0;
    float humidity = 0.0;
    esp_err_t ret = dht_read_float_data(DHT_TYPE_AM2301, DHTPIN, &humidity, &temperature);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "讀取 DHT22 失敗: %s", esp_err_to_name(ret));
        return 0; 
    }
    return temperature;
}

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                       0, 0, 0);
}
