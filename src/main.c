#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "driver/gpio.h"
#include "config.h"
#include "espnow_comm.h"
#include "sensors.h"
#include "network.h"
#include "motor_control.h"
#include "safety_check.h"

static const char *TAG = "MAIN";

static uint8_t newMACAddress[6] = {0x50, 0x0A, 0xC4, 0x59, 0xA5, 0x44};

void wifi_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG,"WiFi連線完成");
}

void sensor_read_tasks(void *pvParameters) {
    while(1) {
        float temperature = read_temperature();
        float battery_voltage = read_battery_voltage();
        float fire_voltage = read_fire_sensor();
        float gas_ppm = read_gas_ppm();

        upload_sensor_data(temperature, battery_voltage, fire_voltage, gas_ppm);
        ESP_LOGI(TAG,"Sensor: fire=%.2f,battery=%.2f,gas=%.2f,temp=%.2f",
                 fire_voltage,battery_voltage,gas_ppm,temperature);

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_base_mac_addr_set(newMACAddress));


     // 網路相關初始化
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta(); // 建立預設的 STA netif
    
    wifi_init();
    i2c_master_init();
    espnow_init();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_2),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_2,0);

    xTaskCreate(sensor_read_tasks, "sensor_read", 4096, NULL, 5, NULL);
    xTaskCreate(safety_check_task, "safety_check", 4096, NULL, 5, NULL);
    xTaskCreate(network_tasks, "network_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
