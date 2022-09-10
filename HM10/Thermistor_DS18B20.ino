/*
 * Temperature sensor accuracy comparison of RTC, thermistor and DS18B20. Results are sent to PC with HM-10 BLE 4.0 transmitter.
 * On PC, a serial port emulator is created and putty used to read the sensor data. Data is also recorded on a microsd card.
 * The ADC on an ESP32 is not linear so a function is used to return adjusted values.
 */
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "SD.h"
#include "FS.h"
#include <SPI.h>
#include "Serial_printf.h"
#include <Wire.h>
#include <uRTCLib.h>
#define ONE_WIRE_BUS 13     //DS18B20 sensor pin
#define RXD2 16             //HM-10 RX pin
#define TXD2 17             //HM-10 TX pin

uRTCLib rtc(0x68);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const char *filename="/data.csv";     //SD card data file 

unsigned long timer1=0;               //loop timer
unsigned int period1 = 1000;          //interval
bool timer1Enabled=true;

#define ADC_RESOLUTION 12            //resolution in bits
#define ThermistorPin1 36            //thermistor between ADC and GND
#define fixedResistance 67000.0      //fixed resistor between ADC and VCC
#define refResistance   33000.0

float maxadc=(float)pow(2,ADC_RESOLUTION);    //2**resolution bits
float a1=3.354016e-3, b1=2.519107e-4, c1=3.510939e-6, d1=1.105179e-7;        //Vishay datasheet for 2322 640 6.333 (B-4090) (P/N NTCLE100E3333JB0)
float logR,tmp1,adc,thermistorResistance;

// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message) {
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// Function for the linear adjustment of the ADC.
double analogReadAdjusted(byte pinNumber){

  // Specify the adjustment factors.
  const double f1 = 1.7111361460487501e+001;
  const double f2 = 4.2319467860421662e+000;
  const double f3 = -1.9077375643188468e-002;
  const double f4 = 5.4338055402459246e-005;
  const double f5 = -8.7712931081088873e-008;
  const double f6 = 8.7526709101221588e-011;
  const double f7 = -5.6536248553232152e-014;
  const double f8 = 2.4073049082147032e-017;
  const double f9 = -6.7106284580950781e-021;
  const double f10 = 1.1781963823253708e-024;
  const double f11 = -1.1818752813719799e-028;
  const double f12 = 5.1642864552256602e-033;

  // Specify the number of loops for one measurement.
  const int loops = 10;

  // Specify the delay between the loops.
  const int loopDelay = 20;

  // Initialize the used variables.
  int counter = 1;
  int inputValue = 0;
  double totalInputValue = 0;
  double averageInputValue = 0;

  // Loop to get the average of different analog values.
  for (counter = 1; counter <= loops; counter++) {

    // Read the analog value.
    inputValue = analogRead(pinNumber);

    // Add the analog value to the total.
    totalInputValue += inputValue;

    // Wait some time after each loop.
    delay(loopDelay);
  }

  // Calculate the average input value.
  averageInputValue = totalInputValue / loops;

  // Calculate and return the adjusted input value.
  return f1 + f2 * pow(averageInputValue, 1) + f3 * pow(averageInputValue, 2) + f4 * pow(averageInputValue, 3) + f5 * pow(averageInputValue, 4) + f6 * pow(averageInputValue, 5) + f7 * pow(averageInputValue, 6) + f8 * pow(averageInputValue, 7) + f9 * pow(averageInputValue, 8) + f10 * pow(averageInputValue, 9) + f11 * pow(averageInputValue, 10) + f12 * pow(averageInputValue, 11);
}

void setup()
{ 
  Serial.begin(115200);     //serial monitor
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);        //HM-10 BLE
  Serial.println("Thermistor DS18B20 compare v1.0");
  sensors.begin();
  sensors.setResolution(12);
  analogRead(ThermistorPin1);                         //always drop 1st read
  Wire.begin(21, 22); // GPIO21,GPIO22 on ESP32       //init RTC
  rtc.set_model(URTCLIB_MODEL_DS3231);
  if (!SD.begin()) {                                  //init SD reader
    Serial.println("Card Mount Failed");
    return;
  }
  writeFile(SD, filename, "Timestamp,RtcTemp,IntTemp,ExtTemp,DS18B20\r\n");
}

void loop()
{
  char sbuf[64];

  if (millis() > timer1 && timer1Enabled) {       //heartbeat timer expired?
    timer1 = millis()+period1;                    //reset HB timer expiry

    adc = analogReadAdjusted(ThermistorPin1);     //get thermistor ADC value
    thermistorResistance = fixedResistance /(maxadc/adc -1);
    logR = log(thermistorResistance/refResistance);
    tmp1 = (1.0 / (a1 + b1*logR + c1*logR*logR + d1*logR*logR*logR));   //int sensor
    tmp1 = (tmp1 - 273.15);

    sensors.requestTemperatures();
    float ds18b20=sensors.getTempCByIndex(0);     //get DS18B20 reading

    rtc.refresh();
    int tempInt=rtc.temp() / 100;                 //get RTC temp
    int tempFrac=rtc.temp() % 100;
    
    sprintf(sbuf,"20%d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d,%d.%d,%.2f,%.2f\r\n",rtc.year(),rtc.month(),rtc.day(),rtc.hour(),rtc.minute(),rtc.second()
      ,tempInt,tempFrac,tmp1,ds18b20);
    appendFile(SD, filename, sbuf);               //log to SD card
    Serial_printf(Serial,"%s",sbuf);              //log to serial console
    Serial_printf(Serial2,"%s",sbuf);             //log to HM-10
  }

}
