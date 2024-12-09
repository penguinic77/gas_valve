#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define WIFI_SSID "LIN"
#define WIFI_PASSWORD "bigsmart"

int32_t Macaddr_num;
uint8_t broadcastAddress2[] = {0xA8, 0x48, 0xFA, 0x0A, 0x7E, 0xF0};//重力
uint8_t broadcastAddress1[]= {0x50, 0x94, 0x54, 0x24, 0x85, 0x0C};//群組
//uint8_t broadcastAddress3[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[2][6]= {
{0xA8, 0x48, 0xFA, 0x0A, 0x7E, 0xF0},{0x34, 0x94, 0x54, 0x24, 0x98, 0x70}
};

typedef struct test_struct {
  int x;
  int y;
} test_struct;

typedef struct state_message {
  bool dangerous;
}state_message;



test_struct myData;
state_message state;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}



void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  String str = String(mac_addr[0]);
  //群組
  if(str == "80") {
    memcpy(&state, incomingData, sizeof(state));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("狀態: ");
    Serial.println(state.dangerous);
    Serial.println();
  }
  //重力
  if(mac_addr == broadcastAddress2){
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("x: ");
    Serial.println(myData.x);
    Serial.print("y: ");
    Serial.println(myData.y);
    Serial.println();
  }
}


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

void setupWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("正在連線至WIFI...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("連線成功!IP位置為: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

}
uint8_t newMACAddress[]= {0x50, 0x94, 0x54, 0x24, 0x98, 0x70};
void setup() {
  Serial.begin(9600);
  setupWifi();
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  int32_t channel = getWiFiChannel("LIN");
  Serial.print("頻道號碼:");
  Serial.println(channel);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // 增加多個發送端
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
  }
  else
  {Serial.println("Success to add peer");}
  
}
 
void loop() {
 state.dangerous = true;

 esp_err_t result1 = esp_now_send(
    broadcastAddress1, 
    (uint8_t *) &state,
    sizeof(state_message));
    if (result1 == ESP_OK) {
    Serial.println("Sent with success");
    }
    else {
    Serial.println("Error sending the data");
    }
  delay(2000); 
}
