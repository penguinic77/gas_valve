//EPS NOW 發送者
/*引入ESP-NOW函式庫*/
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
/*引入ADXL345函式庫*/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(1);
/*引入HX711函式庫*/
#include <HX711.h>
const int LOADCELL_DOUT_PIN = 17;
const int LOADCELL_SCK_PIN = 16;
HX711 scale;
float realweight;

/*ESP-NOW相關設定*/
// 接收方的mac位置
uint8_t broadcastAddress[] = {0x34, 0x94, 0x54, 0x24, 0x98, 0x70};
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

// 訊息發送時會回傳是否已經傳送
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


//取得wifi的Channel ID
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
/*震動感測相關設定*/
sensors_event_t event;
float X_raw, Y_raw, Z_raw; 
float X_avg, Y_avg, Z_avg;

void xyz_read()
{
  accel.getEvent(&event);
  X_raw = event.acceleration.x;
  Y_raw = event.acceleration.y;
  Z_raw = event.acceleration.z;
  delay(500);
}

void espnow_setup()
{
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
//震動感測設定
void earthqk_setup()
{
  while(!accel.begin())
  {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    accel.begin();
    Serial.println("Retry again...");
  }
  //設定感測範圍
  accel.setRange(ADXL345_RANGE_2_G);
  X_avg = X_raw;
  Y_avg = Y_raw;
  Z_avg = Z_raw;
  //取平均值
  for (byte i=0; i<10; i++) {
    xyz_read();
    X_avg = (X_raw + 9*X_avg)/10;
    Y_avg = (Y_raw + 9*Y_avg)/10;
    Z_avg = (Z_raw + 9*Z_avg)/10;
  }
}
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-20);
  earthqk_setup();
  espnow_setup();
  
}

void loop() {
  
  realweight = scale.get_units(30);
  if(realweight<0)
  {
    realweight=0;
  }
  xyz_read();//讀取數值
  if (abs(X_avg-X_raw)>2.00 || abs(Y_avg-Y_raw)>5.00 || abs(Z_avg-Z_raw)>6.00) {
     Serial.println("偵測到地震!!!");
     myData.earthquake = true;
  } 
  else {
     Serial.println("Safe");
     myData.earthquake = false;
     X_avg = (X_raw + 19*X_avg)/20;
     Y_avg = (Y_raw + 19*Y_avg)/20;
     Z_avg = (Z_raw + 19*Z_avg)/20;
  }
  myData.id = 1;
  myData.weight= realweight;
  
  // 透過ESP-NOW發送訊息
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  Serial.print("重量:");
  Serial.println(realweight);
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}
