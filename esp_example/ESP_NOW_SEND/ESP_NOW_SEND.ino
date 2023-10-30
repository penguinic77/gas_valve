//EPS NOW 發送者
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
volatile boolean messageSent; 

// 接收方的mac位置
uint8_t broadcastAddress[] = {0xA8, 0x48, 0xFA, 0x0A, 0x7E, 0xF0};

// 傳送訊息的結構，必須和符合接收方的結構
typedef struct struct_message {
    int id;
    float weight;
    bool earthquake;
} struct_message;

// 宣告一個mydata變數
struct_message myData;
// 宣告一個變數，會儲存有關訊息交換的資料
esp_now_peer_info_t peerInfo;


int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}



// 訊息發送時會回傳是否已經傳送
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  messageSent = true;
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(9600);
  // 將ESP32設為WIFI Station
  WiFi.mode(WIFI_STA);
  // 取得wifi的channel，並設置
  int32_t channel = getWiFiChannel("LIN");
  Serial.print("頻道號碼:");
  Serial.println(channel);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  // 初始化ESP NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // 當ESP NOW完成初始設置, 要註冊傳送訊息的function
  esp_now_register_send_cb(OnDataSent);
  
  // 增加一個接收端
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }


  
}
 
void loop() {
  myData.id = 1;
  myData.weight= random(0,100);
  myData.earthquake = true;
  // 透過ESP-NOW發送訊息
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  //esp_sleep_enable_timer_wakeup(5000000);
  Serial.print("重量:");
  Serial.println(myData.weight);
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(5000);
  /*
  if(messageSent){
    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
  */
  
}
