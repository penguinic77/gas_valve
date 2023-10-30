//EPS NOW 發送者
/*引入ESP-NOW函式庫*/
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define WIFI_SSID "realme"
/*引入ADXL345所需項目*/
#include<Wire.h>
#include<ADXL345_WE.h>
#define ADXL345_I2CADDR 0x53 // 0x1D if SDO = HIGH
ADXL345_WE myAcc = ADXL345_WE(ADXL345_I2CADDR);
/*引入HX711函式庫*/
#include <HX711.h>
const int LOADCELL_DOUT_PIN = 17;
const int LOADCELL_SCK_PIN = 16;
HX711 scale;
RTC_DATA_ATTR float realweight = 0;
RTC_DATA_ATTR int channel = 2;
RTC_DATA_ATTR bool wifi_setchannel = false;

volatile boolean messageSent; 

#define uS_TO_S_FACTOR 1000000  /* 微秒轉換為秒 */
#define TIME_TO_SLEEP  10        /* 睡眠時間(秒為單位) */

/*ESP-NOW相關設定*/
// 接收方的mac位置
uint8_t broadcastAddress[] = {0x50, 0x94, 0x54, 0x24, 0x85, 0x0C};
uint8_t newMACAddress[] = {0x30, 0x94, 0x54, 0x24, 0x98, 0x70};//新的MAC位址
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
  messageSent = true;
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

void espnow_setup()
{
  // 取得wifi的channel，並設置
  if(!wifi_setchannel)
  {channel = getWiFiChannel(WIFI_SSID);
  wifi_setchannel=true;}
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
void earthquk_setup(){
  while(!myAcc.init()) {
    Serial.println("ADXL345 not connected!");
    myAcc.init();
  }
  myAcc.setCorrFactors(-256.0, 270.0, -270.0, 256.0, -220.0, 298.0);
  myAcc.setDataRate(ADXL345_DATA_RATE_25);
  myAcc.setRange(ADXL345_RANGE_4G);
  myAcc.setActivityParameters(ADXL345_DC_MODE, ADXL345_0YZ, 2.20);
  myAcc.setInterrupt(ADXL345_ACTIVITY, INT_PIN_2);
  myAcc.readAndClearInterrupts();
}
//掃瞄重力
void scan_weight(){
  scale.power_up();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(20);
  realweight = scale.get_units(30)-26540;
  if(realweight<0)
  {realweight=0;}
  myData.weight = realweight;
  scale.power_down();
}
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
void setup() {
  Wire.begin();
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  myAcc.readAndClearInterrupts();
  espnow_setup();
  print_wakeup_reason();
  earthquk_setup();//震動感測設定
  //設定timer讓esp32在一定時間內醒來
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //設定外部中斷讓esp32在感測到晃動醒來
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2,1);
  myData.id = 1;
  scan_weight();//執行重力掃描
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason=esp_sleep_get_wakeup_cause();//取得被喚醒的原因
  esp_err_t result;
   /* 透過ESP-NOW發送訊息*/
  if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0){//若為外部喚醒則為危險狀態
   myData.earthquake = true;
   result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER){//因timer而喚醒則為安全狀態
   myData.earthquake=false;
   result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  else//因其他狀態而喚醒則為危險狀態
  {
   myData.earthquake=false;
   result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  Serial.println("重量: " + String(realweight));
  Serial.print("emerg:");
  Serial.println(myData.earthquake);
}

void loop() {
  if(messageSent){
    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
  }
}
