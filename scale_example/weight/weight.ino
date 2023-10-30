#include <HX711.h>
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 17;
const int LOADCELL_SCK_PIN = 16;
HX711 scale;
float weight;

void setup() {

 Serial.begin(9600);
 scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
 scale.set_scale(-20);
}

void loop() {
  // put your main code here, to run repeatedly:


weight = scale.get_units(30);
if(weight<0)
{
  weight=0;
  }
Serial.print("重量");
Serial.println(weight);
}
