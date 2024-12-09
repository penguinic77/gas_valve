#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "config.h"
#include "network.h"

static const char *TAG = "NETWORK";

bool get_group_info(char *groupname, int *mac_count, char *mac_list)
{
    // 取得群組名
    char url[256];
    snprintf(url, sizeof(url), "https://gaxer.ddns.net/groupcheck?tok=%s&dev=%s", TOKEN, GASNAME);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(esp_http_client_perform(client)==ESP_OK) {
        int len = esp_http_client_get_content_length(client);
        char buffer[64]={0};
        esp_http_client_read(client, buffer, len);
        buffer[len]='\0';
        strcpy(groupname, buffer);
    } else {
        esp_http_client_cleanup(client);
        return false;
    }
    esp_http_client_cleanup(client);

    if(strcmp(groupname,"nan")==0) {
        return false;
    }

    // mac 數量
    snprintf(url, sizeof(url), "https://gaxer.ddns.net/groupsimple?tok=%s&group=%s", TOKEN, groupname);
    config.url = url;
    client = esp_http_client_init(&config);
    if(esp_http_client_perform(client)==ESP_OK){
        int len = esp_http_client_get_content_length(client);
        char buffer[32]={0};
        esp_http_client_read(client, buffer, len);
        buffer[len]='\0';
        *mac_count = atoi(buffer);
    } else {
        esp_http_client_cleanup(client);
        return false;
    }
    esp_http_client_cleanup(client);

    snprintf(url, sizeof(url), "https://gaxer.ddns.net/groupdetail?tok=%s&group=%s", TOKEN, groupname);
    config.url = url;
    client = esp_http_client_init(&config);
    if(esp_http_client_perform(client)==ESP_OK){
        int len = esp_http_client_get_content_length(client);
        esp_http_client_read(client, mac_list, len);
        mac_list[len]='\0';
    } else {
        esp_http_client_cleanup(client);
        return false;
    }
    esp_http_client_cleanup(client);

    return true;
}

bool get_sw_state(const char *token, const char *gasname) {
    char url[256];
    snprintf(url, sizeof(url),"https://gaxer.ddns.net/swstatus?tok=%s&dev=%s", token, gasname);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    bool valve = false;
    if(esp_http_client_perform(client) == ESP_OK) {
        int len = esp_http_client_get_content_length(client);
        char buffer[16]={0};
        esp_http_client_read(client, buffer, len);
        buffer[len]='\0';
        if(strcmp(buffer,"True")==0) valve = true;
        else valve = false;
    } 
    esp_http_client_cleanup(client);
    return valve;
}

void update_sw_state(const char *token, const char *gasname, bool sw_state, bool emergency) {
    char url[512];
    snprintf(url,sizeof(url),"https://gaxer.ddns.net/swupdate?tok=%s&sw=%s&dev=%s", token, sw_state?"True":"False", gasname);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(esp_http_client_perform(client)==ESP_OK){
        int len = esp_http_client_get_content_length(client);
        char buffer[64]={0};
        esp_http_client_read(client, buffer, len);
        buffer[len]='\0';
        if(sw_state && !emergency) {
            ESP_LOGI(TAG,"手動開啟:%s",buffer);
        } else if(!sw_state && !emergency) {
            ESP_LOGI(TAG,"手動關閉:%s",buffer);
        } else if(!sw_state && emergency) {
            ESP_LOGI(TAG,"緊急關閉:%s",buffer);
        }
    } else {
        ESP_LOGE(TAG,"更新開關狀態失敗");
    }
    esp_http_client_cleanup(client);
}

void upload_sensor_data(float temp, float voltage, float fire, float gas) {
    char url[512];
    snprintf(url,sizeof(url),"https://gaxer.ddns.net/upload?tok=%s&battery=%.2f&fire=%.2f&temp=%.2f&gas=%.2f&remaining=%.2f&dev=%s",
             TOKEN, voltage, fire, temp, gas, myData.weight, GASNAME);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(esp_http_client_perform(client)==ESP_OK){
        int len = esp_http_client_get_content_length(client);
        char buffer[128]={0};
        esp_http_client_read(client,buffer,len);
        buffer[len]='\0';
        ESP_LOGI(TAG,"上傳回應:%s",buffer);
    } else {
        ESP_LOGE(TAG,"上傳失敗");
    }
    esp_http_client_cleanup(client);
}

void network_tasks(void *pvParameters) {
    while(1) {
        // 可定期更新群組狀態或MAC位址等
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
