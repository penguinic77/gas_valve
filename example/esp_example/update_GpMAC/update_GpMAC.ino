#include <HTTPClient.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#define WIFI_SSID "LIN"
#define WIFI_PASSWORD "bigsmart"

String token="678E82D907D3E6E71F81D5CF3DDACC3671DC618C38A1B7A9F9393A83D025B296";
String gasname = "gas1";

typedef struct test_struct {
  int x;
  int y;
} test_struct;


test_struct myData;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddressv[2][6]= {
{168, 72, 250, 10, 253, 145},{0x34, 0x94, 0x54, 0x24, 0x85, 0x0C}
};

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
int flag =0;


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
int Macaddr_num;//MAC筆數
String MACaddr;
String groupname;//閥門所在的群組
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

uint8_t **broadcastAddress;
uint8_t** espnow_updateMAC()
{
  Get_groupdata(token,gasname);
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
  {
    broadcastAddress[i] = new uint8_t[6];
   }
  
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


void setup() {
  Serial.begin(9600);
  setupWifi();
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
 uint8_t **macaddr = espnow_updateMAC();
 test_struct test;
 test.x = random(0,20);
 test.y = random(0,20);
 
 for(int i=0;i<Macaddr_num;i++)
  {
   esp_err_t result1 = esp_now_send(
    macaddr[i],
    (uint8_t *) &test,
    sizeof(test_struct));
    if (result1 == ESP_OK) {
    Serial.println("Sent with success");
    }
    else {
    Serial.println("Error sending the data");
    }
  delay(2000); 
  }
delay(3000);
}
