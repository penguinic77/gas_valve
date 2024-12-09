#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define WIFI_SSID "LIN"
#define WIFI_PASSWORD "bigsmart"

int32_t Macaddr_num;
uint8_t broadcastAddress1[] = {0xA8, 0x48, 0xFA, 0x0A, 0x7E, 0xF0};
uint8_t broadcastAddress2[]= {0x34, 0x94, 0x54, 0x24, 0x98, 0x70};
uint8_t broadcastAddress3[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

double* a = new double[n];
uint8_t broadcastAddress[2][6]= {
{0xA8, 0x48, 0xFA, 0x0A, 0x7E, 0xF0},{0x34, 0x94, 0x54, 0x24, 0x98, 0x70}
};

typedef struct test_struct {
  int x;
  int y;
} test_struct;

test_struct test;
test_struct myData;
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



void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(myData.x);
  Serial.print("y: ");
  Serial.println(myData.y);
  Serial.println();
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

void setup() {
  Serial.begin(9600);
  setupWifi();
  WiFi.mode(WIFI_AP_STA);
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
  // 
  Macaddr_num=2;
  // 增加多個接收端
  for(int i=0;i<Macaddr_num;i++)
  {
    memcpy(peerInfo.peer_addr, broadcastAddress[i], sizeof(broadcastAddress[i]));
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
    else
    {Serial.println("Success to add peer");}
  }
}
 
void loop() {
  test_struct test;
  test_struct test2;
  test.x = random(0,20);
  test.y = random(0,20);
  test2.x = random(0,20);
  test2.y = random(0,20);

 for(int i=0;i<2;i++)
 {
   esp_err_t result1 = esp_now_send(
    broadcastAddress[i], 
    (uint8_t *) &test,
    sizeof(test_struct));
    if (result1 == ESP_OK) {
    Serial.println("Sent with success");
    }
    else {
    Serial.println("Error sending the data");
    }
  delay(5000); 
}
   
  delay(2000);
}
