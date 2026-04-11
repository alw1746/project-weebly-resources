/*
Measure li-ion battery voltage and current(mahConsumed,Voltage) with INA219 sensor and log to serial
port. These values are copied to a csv file for processing by generate_lut.py to create 
C++ initialisation code for voltage and current lookup tables. If capacity reaches 0, 
the load is disconnected by P-chan MOSFET to prevent damage. 
*/
#include <Wire.h>
#include <INA219_WE.h>
#define CUTOFF_PIN 12
#define CUTOFF_VOLTAGE 3.5f
#define CHANGE_THRESHOLD 10

INA219_WE ina219 = INA219_WE();

float totalConsumedMAH = 0;
float prevConsumedMAH = 0;
unsigned long lastUpdate = 0;
int cutoffCounter=0;

void setup() {
  pinMode(CUTOFF_PIN,OUTPUT);
  digitalWrite(CUTOFF_PIN,HIGH);
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  if (!ina219.init()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  
  // Use high-resolution mode for better table data
  ina219.setPGain(INA219_PG_40); // choose gain and uncomment for change of default
  ina219.setBusRange(INA219_BRNG_16); // choose range and uncomment for change of default
  ina219.setShuntSizeInOhms(0.1); 

  Serial.println("--- BATTERY CHARACTERIZATION START ---");
  Serial.println("Wait until battery reaches CUTOFF_VOLTAGE...");
  Serial.println("Format: mahConsumed,Voltage");
  Serial.println("mahConsumed,Voltage");
  float voltage = ina219.getBusVoltage_V();
  Serial.print(totalConsumedMAH,3);
  Serial.print(",");
  Serial.println(voltage,3);
  lastUpdate = millis();
}

void loop() {
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA();
  
  // 1. Calculate mAh drained
  unsigned long now = millis();
  float hours = (now - lastUpdate) / 3600000.0;
  totalConsumedMAH += current * hours;
  lastUpdate = now;
  float diff=totalConsumedMAH - prevConsumedMAH;
  if (diff >= 1.0f) {
    prevConsumedMAH = totalConsumedMAH;
    Serial.print(totalConsumedMAH,3);
    Serial.print(",");
    Serial.println(voltage,3);
  }
  //Serial.printf("diff:%.3f totalConsumedMAH:%.3f current:%.3f hours:%f voltage:%.3f\r\n",diff,totalConsumedMAH,current,hours,voltage);
  // 4. Safety Cutoff
  if (voltage <= CUTOFF_VOLTAGE) {
    cutoffCounter++;
    if (cutoffCounter > CHANGE_THRESHOLD) {
      Serial.print(totalConsumedMAH,3);
      Serial.print(",");
      Serial.println(CUTOFF_VOLTAGE,3);
      Serial.println("--- CHARACTERIZATION COMPLETE ---");
      digitalWrite(CUTOFF_PIN,LOW);
      while(1); // Stop
    }
  } else {
    cutoffCounter=0;
  }
  
  delay(1000); 
}
