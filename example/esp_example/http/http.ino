#include <HTTPClient.h>
#include <WiFi.h>
#define WIFI_SSID "LIN"
#define WIFI_PASSWORD "bigsmart"

String token="678E82D907D3E6E71F81D5CF3DDACC3671DC618C38A1B7A9F9393A83D025B296";
String gasname="gas1";

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

String MACaddr;
int Macaddr_num;
String groupname="";

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

void Get_groupdata(String token,String gasname){
  //取得閥門所在的群組名稱
  String groupdata= "https://gaxer.ddns.net/groupcheck?tok="+token+"&dev="+gasname;
  groupname=Get_DBdata(groupdata);
  Serial.print("群組名: ");
  Serial.println(groupname);
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
}


void setup() {
  Serial.begin(9600);
  setupWifi();
  WiFi.mode(WIFI_AP_STA);

 Get_groupdata(token,gasname);
 
 uint8_t **broadcastAddress;
 broadcastAddress = new uint8_t*[Macaddr_num];
 
 MACaddr
 
 for(int i =0;i<Macaddr_num;i++){
  broadcastAddress[i] = new uint8_t[6];
  
 }
  
}

void loop() {
 

 
 delay(3000);
}
