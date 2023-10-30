//EPS NOW接收者
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define WIFI_SSID "LIN"
#define WIFI_PASSWORD "bigsmart"

// 接收訊息的結構，必須和符合發送方的結構
typedef struct struct_message {
  int id;
  float weight;
  bool earthquake;
}struct_message;

// 宣告一個mydata變數
struct_message myData;

// 建立板子的變數，使接收到的資料可以分配到相應的板子
struct_message board1;
struct_message board2;
struct_message board3;

// 宣告一個一維陣列來存放發送端的板子
struct_message boardsStruct[3] = {board1, board2, board3};

// 接收到訊息時會回傳接收到的內容在終端
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data
  boardsStruct[myData.id-1].weight = myData.weight;
  boardsStruct[myData.id-1].earthquake = myData.earthquake;
  Serial.print("weight value:");
  Serial.println(boardsStruct[myData.id-1].weight);
  Serial.print("earthquake value:");
  Serial.println(boardsStruct[myData.id-1].earthquake);
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
  //Initialize Serial Monitor
  Serial.begin(9600);
  setCpuFrequencyMhz(80);
  //將ESP32設為WIFI AP Station
  WiFi.mode(WIFI_AP_STA);
  setupWifi();
  
  //初始化 ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // 當ESP NOW完成初始設置, 要註冊接收訊息的function
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {


  delay(10000);  
}
