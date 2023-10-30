#define uS_TO_S_FACTOR 1000000  /* 微秒轉換為秒 */
#define TIME_TO_SLEEP  8        /* 睡眠時間(秒為單位) */
/* 睡眠時間(秒為單位) */
#include <HX711.h>
const int LOADCELL_DOUT_PIN = 17;
const int LOADCELL_SCK_PIN = 16;
HX711 scale;
/* 引入ADXL345所需項目*/
#include<Wire.h>
#include<ADXL345_WE.h>
#define ADXL345_I2CADDR 0x53 // 0x1D if SDO = HIGH
ADXL345_WE myAcc = ADXL345_WE(ADXL345_I2CADDR);



RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float realweight = 0;
RTC_DATA_ATTR bool emerg = false;



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
//掃瞄重力
void scan_weight(){
  scale.power_up();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-20);
  realweight = scale.get_units(30);
  if(realweight<0)
  {realweight=0;}
  scale.power_down();
}
//晃動感測初始設定
void earthquk_setup(){
  while(!myAcc.init()) {
    Serial.println("ADXL345 not connected!");
    myAcc.init();
  }
  myAcc.setCorrFactors(-242.0, 272.0, -262.0, 260.0, -272.0, 228.0);
  myAcc.setDataRate(ADXL345_DATA_RATE_25);
  myAcc.setRange(ADXL345_RANGE_4G);
  myAcc.setActivityParameters(ADXL345_DC_MODE, ADXL345_0YZ, 2.80);
  myAcc.setInterrupt(ADXL345_ACTIVITY, INT_PIN_2);
  myAcc.readAndClearInterrupts();
}
void setup(){
  Wire.begin();
  Serial.begin(9600);
  ++bootCount;
  
  print_wakeup_reason();
  //設定timer讓esp32在一定時間內醒來
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //設定外部中斷讓esp32在感測到晃動醒來
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2,1);
  
  //讀取被喚醒的原因
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason=esp_sleep_get_wakeup_cause();
  //若為外部喚醒則為危險狀態
  if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {emerg=true;
  //sendata
  }
  else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  {emerg=false;
   scan_weight();
  //sendata
  }
  delay(5000);
  Serial.println("Boot number: " + String(bootCount));
  Serial.println("重量: " + String(realweight));
  earthquk_setup();
  Serial.print("emerg:");
  Serial.println(emerg);
  
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop(){
  //This is not going to be called
}
