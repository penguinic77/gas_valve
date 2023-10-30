/*WIFI與ESP-NOW*/
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <WiFi.h>
#define WIFI_SSID "realme"
#define WIFI_PASSWORD "0934257928"
/*馬達*/
#include <TB6612FNG.h>
Tb6612fng motor(14, 18, 19, 21, 27, 26, 25);
// 14 - Standby pin
// 27 - AIN1 pin
// 26 - AIN2 pin
// 25 - PWMA pin
/*ADC*/
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1015 ads;
/*溫度*/
#include "DHT.h"
#define DHTPIN  32
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float temp;
float realtemp;

/*使用者相關資訊*/
String token="678E82D907D3E6E71F81D5CF3DDACC3671DC618C38A1B7A9F9393A83D025B296";
String gasname="gas2";
uint8_t newMACAddress[] = {0x50, 0x94, 0x54, 0x24, 0x85, 0x0C}; //新的MAC位址

/*ESP-NOW相關設定，*/
/*以下為訊息的結構，必須和符合發送方的結構*/
//接收感測器訊息的結構
typedef struct sensor_message {
  int id;
  float weight;
  bool earthquake;
}sensor_message;
//發送和接收緊急狀況的結構
typedef struct state_message {
  bool dangerous;
}state_message;


// 宣告一個儲存感測器資料和緊急狀況的變數
sensor_message myData;
state_message group;
esp_now_peer_info_t peerInfo;
// 接收到訊息時會回傳接收到的內容在終端
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  /*判斷是哪種裝置傳過來的訊息，由MAC位址開頭來判斷，50為閥門、30為重力板*/
  String str = String(mac_addr[0]);
  //其他同群組的閥門
  if(str == "80") {//50轉10進位為80
    memcpy(&group, incomingData, sizeof(group));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("狀態: ");
    Serial.println(group.dangerous);
  }
  //重力
  if(str == "48"){//30轉10進位為48
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.print("weight value:");
    Serial.println(myData.weight);
    Serial.print("earthquake value:");
    Serial.println(myData.earthquake);
  }
}

// 發送到訊息時會顯示是否成功在終端
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

void setupWifi() {//WIFI
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
}
/*取得WIFI頻道號碼*/
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
  //初始化 ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // 註冊接收訊息的function
  esp_now_register_send_cb(OnDataSent);
  // 註冊接收訊息的function
  esp_now_register_recv_cb(OnDataRecv);
}
/*發送HTTP訊息，取得資料庫回應的內容*/
String Get_DBdata(String path)
{
  String message="";
  HTTPClient http;
  http.begin(path.c_str());
  http.GET();
  message = http.getString();
  http.end();
  return message;
}

String MACaddr;
String groupname;//閥門所在的群組
int Macaddr_num;//MAC筆數
bool Get_groupdata(){
  //取得閥門所在的群組名稱
  String groupdata= "https://gaxer.ddns.net/groupcheck?tok="+token+"&dev="+gasname;
  groupname=Get_DBdata(groupdata);
  Serial.print("群組名: ");
  Serial.println(groupname);
  if(groupname!="nan")
  {
    //取得群組中MAC的數量
    groupdata = "https://gaxer.ddns.net/groupsimple?tok="+token+"&group="+groupname;
    String num = Get_DBdata(groupdata);
    Macaddr_num=num.toInt();
    Serial.print("MAC數量: ");
    Serial.println(Macaddr_num);
    //取得群組中MAC的數量
    groupdata = "https://gaxer.ddns.net/groupdetail?tok="+token+"&group="+groupname;
    MACaddr = Get_DBdata(groupdata);
    Serial.print("MAC位址: ");
    Serial.println(MACaddr);
    return true;
  }
  else
  {return false;}
}
int flag=0;//第一次為0
uint8_t **broadcastAddress;
/*更新群組的MAC位址*/
uint8_t** espnow_updateMAC()
{
  //刪除原先的MAC位址
  if(flag){
    for(int i =0;i<Macaddr_num;i++){
      const uint8_t *peer_addr=broadcastAddress[i];
      if (esp_now_del_peer(peer_addr) == ESP_OK){
      Serial.println("Success to del peer");
      }
      else if (esp_now_del_peer(peer_addr)==ESP_ERR_ESPNOW_NOT_FOUND){
      Serial.println("peer is not found");
      }
      else
      {Serial.println("Failed to del peer");}
      delete [] broadcastAddress[i];//釋放記憶體
    }
    delete [] broadcastAddress;//釋放記憶體
  }
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  //宣告二維動態陣列用來存放MAC位置
  broadcastAddress = new uint8_t*[Macaddr_num];
  for(int i=0;i<Macaddr_num;i++)
  {broadcastAddress[i] = new uint8_t[6];}
  //解析MAC位置並儲存到陣列中
  int MAC_len=MACaddr.length()+1;
  char a[MAC_len];
  MACaddr.toCharArray(a,MAC_len);//將String轉為char
  char addr[MAC_len]={0};
  strcpy(addr,a);
  char *split1;
  int i=0;
  split1 = strtok(addr, ",");//以','分割字串
  while(split1 != NULL) 
  {   
    char temp[MAC_len]={0};
    strcpy(temp,split1);
    Serial.println("第一切:");
    Serial.println(split1);
    split1 = strtok(NULL, ",");
    char *split2;
    int j=0;
    split2 = strtok(temp, ":");//以':'分割字串
    Serial.println("第二切:");
    while(split2 != NULL) {
      broadcastAddress[i][j]=strtoul (split2, NULL, 16);//將字串中的文字轉為16進制並存入陣列
      peerInfo.peer_addr[j]=broadcastAddress[i][j];
      Serial.println(broadcastAddress[i][j]);
      split2 = strtok(NULL, ":");
      j++;
    }
    //新增peer資訊
    if (esp_now_add_peer(&peerInfo) == ESP_OK)
    {Serial.println("Success to add peer");}
    else if (esp_now_add_peer(&peerInfo)==ESP_ERR_ESPNOW_EXIST)
    {Serial.println("peer has existed");}
    else
    {Serial.println("Failed to add peer");}
    i++;
  }
  flag=1;
  return broadcastAddress;
}
/*溫度感測*/
void tempsenser()
{
  realtemp = 0;
  for(int i=0;i<5;i++){
    temp = dht.readTemperature();
    realtemp += temp;
  }
  temp=0;
  realtemp = realtemp/5-2.5;
  if(isnan(realtemp))
  {realtemp=0;}
}
/*量測電壓*/
double vin = 0.0;
double vout = 0.0;
void vinsenser()
{
  vin = 0;
  vout = 0;
  for(int i=0;i<5;i++){
    //vin = (adcSensor.getSingleEnded(0)*3.95)/265;
    vin = (ads.readADC_SingleEnded(0)*3.2*3)/1000.0;
    Serial.print("vin:");
    Serial.println(vin);
    vout += vin;
  }
  vout /= 5;
  Serial.print("vout:");
  Serial.println(vout);
  vout=((vout-11.1)/2.25)*100;//百分比換算
  if(vout<0)
  {vout=0;}
  if(vout>100)
  {vout=100;}
}
/*火焰*/
float realfire;
void firesenser()
{
  realfire = 0;
  for(int j=0;j<5;j++){
    temp = ads.readADC_SingleEnded(3);
    realfire += temp;
  }
  temp=0;
  realfire /= 5;
}
/*MQ5*/
#define RL 20  //The value of resistor RL is 20K
#define m -0.611 //Enter calculated Slope 
#define b 1.33 //Enter calculated intercept
#define Ro 100 //Enter found Ro value 100
#define MQ_sensor 33 //Sensor is connected to 33
float VRL; //Voltage drop across the MQ sensor
float Rs; //Sensor resistance at gas concentration
float ratio; //Define variable for ratio
float ppm;
void mqsenser() 
{
  ppm =0;
  float adc=0;
  adc=ads.readADC_SingleEnded(1);
  VRL = adc *(3.0)/(1000.0);
  Rs = ((5.0*RL)/VRL)-RL;
  ratio = Rs/Ro;
  //ppm = pow(10, ((log10(ratio)-b)/m));
  ppm = pow(6, ((log10(ratio)-b)/m));
  if (ppm < 0){ 
    ppm =0;}
}
/*取得資料庫開關狀態*/
bool Get_sw(String token,String gasname)
{
  String sw_state="";
  bool valve;
  HTTPClient httpv;
  String serverPath= "https://gaxer.ddns.net/swstatus?tok="+token+"&dev="+gasname;
  httpv.begin(serverPath.c_str());
  int httpResponseCode = httpv.GET();
  if(httpResponseCode>0)
  {
    sw_state = httpv.getString();
    Serial.print("開關狀態:");
    Serial.println(sw_state);
  }
  else
  {Serial.println("Http Error");
  Serial.println(httpResponseCode);}
  httpv.end();
  if(sw_state=="True")
  {valve=true;}
  else
  {valve=false;}
  return valve;
}
/*變更資料庫開關狀態*/
void Update_sw(String token,String sw_state,bool emerg,String gasname)
{
  String message="";
  HTTPClient httpv;
  String swupdate= "https://gaxer.ddns.net/swupdate?tok="+ token +"&sw="+sw_state+"&dev="+gasname;
  httpv.begin(swupdate.c_str());
  int httpResponseCode = httpv.GET();
  message = httpv.getString();
  if(sw_state=="True")
  {
    Serial.print("手動開啟:");
    Serial.println(message);
  }
  else if(sw_state=="False"&& emerg==false)
  {
    Serial.print("手動關閉:");
    Serial.println(message);
  }
  else if(sw_state=="False"&& emerg==true)
  {
    Serial.print("緊急關閉:");
    Serial.println(message);
  }
  httpv.end();
}

String up_state = "";
bool emerg = false;
bool motor_state = false;
bool dbvalve;

void setup() {
  Serial.begin(9600);
  //ADC
  Wire.begin();
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  //初始化WIFI，並設定頻道號碼和隱藏SSID
  WiFi.softAP("Gas_valve2",NULL,1,1);
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);//設定新的MAC位址，必須在設置WIFI前使用
  setupWifi();
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  int32_t channel = getWiFiChannel(WIFI_SSID);
  Serial.print("頻道號碼:");
  Serial.println(channel);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);//設定頻道號碼，ESP-NOW配對雙方必須在相同頻道
  espnow_setup();
  //馬達
  motor.begin();
  //溫度
  dht.begin();
  pinMode(2, OUTPUT);
  delay(300);
}
bool group_flag=false;//是否是自己觸發危險狀態

void loop() {
  tempsenser(); //溫度
  vinsenser();  //電壓
  firesenser(); //火焰
  mqsenser(); //MQ5
  
  /*危險情況判斷*/
  up_state = "";

  if(group.dangerous){/*群組*/
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  if(myData.earthquake){/*晃動*/
    emerg = true;
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  if(realfire < 1000 ){/*火焰*/
    emerg = true;
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  if(vin < 1  ){/*電量不足*/
    emerg = true;
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  if(ppm >= 1900 ){/*瓦斯濃度*/
    emerg = true;
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  if(realtemp > 45 ){/*溫度*/
    emerg = true;
    up_state += "1";
  }
  else{
    up_state += "0";
  }
  
  if(up_state== "100000" && group_flag)//當自己觸發群組危險時，若自己無其他危險狀態則將狀態切換為安全
  {emerg = false;}
  else if (up_state=="000000")
  {emerg = false;}

  /*感測資料上傳*/
  String Datasend="https://gaxer.ddns.net/upload?tok="+token+"&battery="+String(vout)+"&fire="+String(realfire)+"&temp="+String(realtemp)+"&gas="+String(ppm)+"&remaining="+String(myData.weight)+"&safe="+up_state+"&dev="+gasname;
  String reply=Get_DBdata(Datasend);
  Serial.println(reply);

  if(emerg)
  {
    for(int j=0;j<2;j++){
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(2,false); 
        delay(200);
      }  
  }

  /*群組警報*/
  if(emerg && Get_groupdata())//若為危險狀態且有在群組則發送危險情況給所有同群組的閥門
  {
   group_flag=true;
   group.dangerous=true;
   uint8_t **macaddr = espnow_updateMAC();
   for(int i=0;i<Macaddr_num;i++)
   {
    esp_err_t result1 = esp_now_send(
    macaddr[i],
    (uint8_t *) &group,
    sizeof(state_message));
    if (result1 == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
    delay(1000); 
   }
  }
  else if(group_flag && !emerg && Get_groupdata())//通知群組的其他閥門自己安全了
  {
    group_flag=false;
    group.dangerous=false;
    for(int i=0;i<Macaddr_num;i++)
    {
      esp_err_t result1 = esp_now_send(
      broadcastAddress[i],
      (uint8_t *) &group,
      sizeof(state_message));
      if (result1 == ESP_OK) {
      Serial.println("Sent with success");
      }
      else {
      Serial.println("Error sending the data");
      }
      delay(1000); 
    }
  }
  else{
  Serial.println("安全或不在群組中");
  }

  
  dbvalve = Get_sw(token,gasname);//從資料庫取得開關狀態
  
  if(group.dangerous && motor_state)//群組危險狀態下則強制關閉開關
  {
    Update_sw(token,"False",true,gasname);
    motor_state = false;           
    //關閉馬達
    for(int j=0;j<2;j++){
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(2,false); 
        delay(200);
      }  
    motor.drive(1,8000);  
  }
  else if(emerg && motor_state)//閥門緊急狀態下則強制關閉開關
  {
    //將資料庫開關關閉
    Update_sw(token,"False",true,gasname);
    motor_state = false;           
    //關閉馬達
    for(int j=0;j<2;j++){
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(2,false); 
        delay(200);
      }  
    motor.drive(1,8000);     
  }
  //安全狀態下，若實際馬達狀態與資料庫的不符則進行調整
  else if(emerg == false && (dbvalve != motor_state))
  {
      for(int j=0;j<2;j++){
         digitalWrite(2,HIGH);
         delay(200);
         digitalWrite(2,false); 
         delay(200);
      }
      if(dbvalve){//開啟馬達
        motor_state = true; 
        Update_sw(token,"True",false,gasname);
        motor.drive(-1,8000);
      }
      else{//關閉馬達
        motor_state = false; 
        Update_sw(token,"False",false,gasname);
        motor.drive(1,8000); 
      }
  }
  motor.brake();//停止馬馬達

  
  Serial.println("t="+String(realtemp)+" V="+String(vout)+" g="+String(myData.weight)); 
  Serial.println("mq="+String(ppm)+" f="+String(realfire)+",dbv="+String(dbvalve)+" up_state="+up_state);
  
  
  delay(2500);
}
