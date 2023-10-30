#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

void displaySensorDetails(void)
{
sensor_t sensor;
accel.getSensor(&sensor);
Serial.println("------------------------------------");
Serial.print ("Sensor: "); Serial.println(sensor.name);
Serial.print ("Driver Ver: "); Serial.println(sensor.version);
Serial.print ("Unique ID: "); Serial.println(sensor.sensor_id);
Serial.print ("Max Value: "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
Serial.print ("Min Value: "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
Serial.print ("Resolution: "); Serial.print(sensor.resolution); Serial.println(" m/s^2");
Serial.println("------------------------------------");
Serial.println("");
delay(500);
}

void displayDataRate(void)
{
Serial.print ("Data Rate: ");

switch(accel.getDataRate())
{
case ADXL345_DATARATE_3200_HZ:
Serial.print ("3200 ");
break;
case ADXL345_DATARATE_1600_HZ:
Serial.print ("1600 ");
break;
case ADXL345_DATARATE_800_HZ:
Serial.print ("800 ");
break;
case ADXL345_DATARATE_400_HZ:
Serial.print ("400 ");
break;
case ADXL345_DATARATE_200_HZ:
Serial.print ("200 ");
break;
case ADXL345_DATARATE_100_HZ:
Serial.print ("100 ");
break;
case ADXL345_DATARATE_50_HZ:
Serial.print ("50 ");
break;
case ADXL345_DATARATE_25_HZ:
Serial.print ("25 ");
break;
case ADXL345_DATARATE_12_5_HZ:
Serial.print ("12.5 ");
break;
case ADXL345_DATARATE_6_25HZ:
Serial.print ("6.25 ");
break;
case ADXL345_DATARATE_3_13_HZ:
Serial.print ("3.13 ");
break;
case ADXL345_DATARATE_1_56_HZ:
Serial.print ("1.56 ");
break;
case ADXL345_DATARATE_0_78_HZ:
Serial.print ("0.78 ");
break;
case ADXL345_DATARATE_0_39_HZ:
Serial.print ("0.39 ");
break;
case ADXL345_DATARATE_0_20_HZ:
Serial.print ("0.20 ");
break;
case ADXL345_DATARATE_0_10_HZ:
Serial.print ("0.10 ");
break;
default:
Serial.print ("???? ");
break;
}
Serial.println(" Hz");
}

void displayRange(void)
{
Serial.print ("Range: +/- ");

switch(accel.getRange())
{
case ADXL345_RANGE_16_G:
Serial.print ("16 ");
break;
case ADXL345_RANGE_8_G:
Serial.print ("8 ");
break;
case ADXL345_RANGE_4_G:
Serial.print ("4 ");
break;
case ADXL345_RANGE_2_G:
Serial.print ("2 ");
break;
default:
Serial.print ("?? ");
break;
}
Serial.println(" g");
}

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

void setup(void)
{
Serial.begin(9600);
Serial.println("Accelerometer Test"); Serial.println("");

//若未成功啟動則發出訊息
while(!accel.begin())
{
Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
accel.begin();
Serial.println("Retry again...");
}

/*設定感測範圍*/
accel.setRange(ADXL345_RANGE_2_G);
// displaySetRange(ADXL345_RANGE_8_G);
// displaySetRange(ADXL345_RANGE_4_G);
// displaySetRange(ADXL345_RANGE_2_G);

/* Display some basic information on this sensor */
displaySensorDetails();

/* Display additional settings (outside the scope of sensor_t) */
displayDataRate();
displayRange();
Serial.println("");

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

void loop(void)
{
/* 取得最新的感測資料 */
xyz_read();
Serial.print("X: "); Serial.print(X_raw); Serial.print(" ");
Serial.print("Y: "); Serial.print(Y_raw); Serial.print(" ");
Serial.print("Z: "); Serial.print(Z_raw); Serial.print(" ");Serial.println("m/s^2 ");
/* Display the results (acceleration is measured in m/s^2) */
Serial.print("X2: "); Serial.print(abs(X_avg-X_raw)); Serial.print(" ");
Serial.print("Y2: "); Serial.print(abs(Y_avg-Y_raw)); Serial.print(" ");
Serial.print("Z2: "); Serial.print(abs(Z_avg-Z_raw)); Serial.print(" ");Serial.println("m/s^2 ");


if (abs(X_avg-X_raw)>2.00 || abs(Y_avg-Y_raw)>5.00 || abs(Z_avg-Z_raw)>6.00) {
     Serial.println(" E A R T H Q U A R K   Detected!");
  } else {
     Serial.println("Safe");
     X_avg = (X_raw + 19*X_avg)/20;
     Y_avg = (Y_raw + 19*Y_avg)/20;
     Z_avg = (Z_raw + 19*Z_avg)/20;
  }

delay(1000);
}
