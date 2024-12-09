#ifndef ESPNOW_COMM_H
#define ESPNOW_COMM_H

#include "config.h"
#include "esp_now.h"

void espnow_init(void);
void update_group_peers(void);

// 這兩個函式在espnow_comm.c中定義
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

#endif
