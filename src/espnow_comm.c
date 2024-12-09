#ifndef ESPNOW_COMMON_H
#define ESPNOW_COMMON_H

#include <string.h>
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "config.h"
#include "espnow_comm.h"

// 全域變數在config.h中宣告，在此定義初始值
sensor_message_t myData = {0};
state_message_t group = {0};
bool motor_state = false; 
bool emerg = false;      
bool group_flag = false; 

static const char *TAG = "ESPNOW_COMM";

static uint8_t **broadcastAddress = NULL;
static int Macaddr_num = 0;
static char MACaddr[512] = {0};

extern bool get_group_info(char *groupname, int *mac_count, char *mac_list); // network.c中實作

// ESP-NOW接收回呼函數
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             info->src_addr[0], info->src_addr[1], info->src_addr[2], 
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

    ESP_LOGI(TAG, "收到來自 %s 的封包", macStr);

    if (info->src_addr[0] == 0x80) {  // 群組閥門
        memcpy(&group, data, sizeof(group));
        ESP_LOGI(TAG, "群組狀態: %s", group.dangerous ? "危險" : "安全");
    }
    
    if (info->src_addr[0] == 0x48) {  // 重力感測器
        memcpy(&myData, data, sizeof(myData));
        ESP_LOGI(TAG, "重量: %f, 地震: %s", myData.weight, myData.earthquake ? "是" : "否");
    }
}

// ESP-NOW傳送回呼函數
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], 
             mac_addr[3], mac_addr[4], mac_addr[5]);
    ESP_LOGI(TAG, "發送至 %s %s", macStr, status == ESP_NOW_SEND_SUCCESS?"成功":"失敗");
}

void espnow_init(void) {
    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

static uint8_t** espnow_updateMAC(const char *mac_list, int mac_count)
{
    if(broadcastAddress != NULL) {
        for(int i=0;i<mac_count;i++){
            esp_now_del_peer(broadcastAddress[i]);
            free(broadcastAddress[i]);
        }
        free(broadcastAddress);
    }

    broadcastAddress = (uint8_t**)malloc(sizeof(uint8_t*)*mac_count);
    for(int i=0;i<mac_count;i++)
        broadcastAddress[i] = (uint8_t*)malloc(sizeof(uint8_t)*6);

    char mac_str[512];
    strcpy(mac_str, mac_list);

    char *saveptr1;
    char *token_str = strtok_r(mac_str, ",", &saveptr1);
    int i=0;
    while(token_str != NULL && i<mac_count){
        char *saveptr2;
        int j=0;
        char *octet = strtok_r(token_str, ":", &saveptr2);
        while(octet != NULL && j<6) {
            broadcastAddress[i][j]= (uint8_t)strtoul(octet,NULL,16);
            octet = strtok_r(NULL, ":", &saveptr2);
            j++;
        }

        esp_now_peer_info_t peerInfo = {0};
        memcpy(peerInfo.peer_addr, broadcastAddress[i], 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) == ESP_OK)
            ESP_LOGI(TAG, "成功加入peer");
        else
            ESP_LOGE(TAG, "加入peer失敗");

        token_str = strtok_r(NULL, ",", &saveptr1);
        i++;
    }
    return broadcastAddress;
}

void update_group_peers(void) {
    char groupname[64]={0};
    int mac_count=0;
    char mac_list[512]={0};

    if(get_group_info(groupname, &mac_count, mac_list)) {
        if(mac_count>0) {
            espnow_updateMAC(mac_list, mac_count);
        }
    } else {
        ESP_LOGI(TAG,"不在群組或取得群組資訊失敗");
    }
}

#endif
