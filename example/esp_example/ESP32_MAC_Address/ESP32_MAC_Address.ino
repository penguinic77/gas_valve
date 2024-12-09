#include <WiFi.h>

void setup(){
  Serial.begin(9600);
  
}
 
void loop(){
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  delay(3000);
}
