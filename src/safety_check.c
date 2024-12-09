#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "sensors.h"
#include "esp_log.h"
#include "network.h"
#include "espnow_comm.h"
#include "motor_control.h"
#include "driver/gpio.h"

static const char *TAG="SAFETY";

extern bool get_group_info(char *groupname, int *mac_count, char *mac_list);

void safety_check_task(void *pvParameters) {
    while (1) {
        emerg = false;
        bool group_danger = group.dangerous;
        bool earthquake = myData.earthquake;
        bool fire = (read_fire_sensor() < 1000);
        bool low_battery = (read_battery_voltage()<1);
        bool high_gas = (read_gas_ppm()>=1900);
        bool high_temp = (read_temperature()>45);

        if(group_danger || earthquake || fire || low_battery || high_gas || high_temp) {
            emerg = true;
        }

        bool dbvalve = get_sw_state(TOKEN,GASNAME);

        // 危險時強制關閉
        if((group_danger && motor_state) || (emerg && motor_state)) {
            update_sw_state(TOKEN, GASNAME, false, true);
            motor_state = false;
            motor_control(1,100);
            vTaskDelay(pdMS_TO_TICKS(800));
            motor_control(1,0);
            gpio_set_level(GPIO_NUM_2,1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(GPIO_NUM_2,0);
        } else if(!emerg && (dbvalve != motor_state)) {
            gpio_set_level(GPIO_NUM_2,1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(GPIO_NUM_2,0);
            if(dbvalve) {
                motor_state = true;
                update_sw_state(TOKEN, GASNAME, true, false);
                motor_control(-1,100);
                vTaskDelay(pdMS_TO_TICKS(800));
                motor_control(-1,0);
            } else {
                motor_state = false; 
                update_sw_state(TOKEN, GASNAME, false, false);
                motor_control(1,100);
                vTaskDelay(pdMS_TO_TICKS(800));
                motor_control(1,0);
            }
        }

        // 告知群組狀態
        char gname[64]={0};int mcount=0;char mlist[512]={0};
        if(emerg && get_group_info(gname,&mcount,mlist)) {
            if(!group_flag) {
                group_flag = true;
                group.dangerous = true;
                update_group_peers();
                // 發送狀態給群組
                // 在espnow_comm.c裡的update_group_peers已經做了peer update
                // 這裡自行發送給每個peer(簡化不再重複示範)
                // ...
            }
        } else if(group_flag && !emerg && get_group_info(gname,&mcount,mlist)) {
            group_flag=false;
            group.dangerous=false;
            // 通知群組安全狀態
            // ...
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}
