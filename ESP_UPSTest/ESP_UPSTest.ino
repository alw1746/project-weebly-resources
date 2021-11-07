/*
 * Test UPS solutions for the ESP32 using supercapacitors or battery backup. These provide
 * short-duration power allowing critical data to be saved to micro SD card before shutdown.
 * 
 *   Attach an interrupt to the Vin (5V) pin.
 *   When USB power is applied or removed,a CHANGE interrupt is generated.
 *   1-sec delay to confirm a state change has occurred.
 *   Read state of VIN_SENSE pin.
 *   On powerup(HIGH),
 *     enable battery backup with BATT_EN HIGH.
 *     Open micro SD card data file and write 1 rec/sec.
 *     Monitor BATT_SENSE for battery voltage.
 *     Enable wifi.
 *   On powerdown(LOW),
 *     turn off wifi.
 *     turn on LED.
 *     close data file and unmount micro SD card.
 *     10-sec delay before disable battery backup with BATT_EN LOW.
 *     When LED goes off, it indicates power has been removed.
 *
 *   Note: while the steps refer to battery, the supercapacitors have the same outcome except
 *   power runs out automatically without the need for explicit shutdown of battery.
 *   ESP32 pin levels: LOW 0.25x3.3V=0.825V, HIGH 0.75x3.3V=2.475V
 */
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#define MOSI 23
#define MISO 19
#define SCK 18
#define SD_CSPIN SS          //CS for SD card
#define SD_CD 17             //Card Detect

#define VIN_SENSE 27         //monitor Vin(5V).
#define BATT_EN 26           //enable/disable battery backup
#define BATT_SENSE 36        //monitor battery voltage
#define PWRDWN_DELAY 1000    //allow for power fluctuations(msecs).
volatile uint32_t startOutageTime=0;    //start of interrupt
bool powerUp,powerDown;

unsigned long endTimer1=0;   //heartbeat timer
unsigned int period1 = 1000;
bool timer1Enabled=true;

unsigned long endTimer2=0;    //power cut delay timer
unsigned int period2 = 10000;
bool timer2Enabled=false;

int counter=0,status,sdcounter;
uint32_t brown_reg_temp;
char buf[16];

const char* ssid     = "ESPAP24";          //setup ESP32 to be a standalone AP.
const char* password = "******";
WiFiServer server(80);

File dataFile;
const char *filename="/data.txt";          //SD card data file 
bool writeEnabled=false;

//power interrupt service routine
void IRAM_ATTR power_isr() {
  startOutageTime=millis();  //start of Vin change
}

//Adjust ADC read to compensate for non-linearity of conversion by ESP32.
//https://bitbucket.org/Blackneron/esp32_adc/src/master/
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
  const int loops = 20;

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

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(VIN_SENSE, INPUT_PULLDOWN);
  pinMode(BATT_EN, OUTPUT);
  digitalWrite(BATT_EN,HIGH);                       //enable battery backup
  attachInterrupt(VIN_SENSE, power_isr, CHANGE);    //interrupt on pin state change(HIGH or LOW)
  startOutageTime=0;
  powerUp=digitalRead(VIN_SENSE);                   //current state of Vin
  powerDown=false;
  sdcounter=0;
  timer2Enabled=false;
  timer1Enabled=true;
}

void openFile() {
  writeEnabled=false;                              //disable SD write by default
  if (SD.begin()) {
    if (!SD.exists(filename)) {
      dataFile = SD.open(filename, FILE_WRITE);    //overwrite existing file
      if (dataFile) {
        dataFile.println("Data file open:write");
        Serial.printf("Data file open:write\r\n");
        writeEnabled=true;
      }
      else {
        Serial.printf("SD open:write fail\r\n");
      }
    }
    else {
      dataFile = SD.open(filename, FILE_APPEND);
      if (dataFile) {
        dataFile.println("Data file open:append");
        Serial.printf("Data file open:append\r\n");
        writeEnabled=true;
      }
      else {
         Serial.printf("SD open:append fail\r\n");
      }
    }
  }
  else {
    Serial.println("SD begin Failed");
  }
}

void loop() {
  if (startOutageTime > 0) {       //power interrupted?
    if (millis() > startOutageTime+PWRDWN_DELAY) {    //wait timeout?
      status=digitalRead(VIN_SENSE);                  //Vin state
      if (!status) { 
        powerDown=true;           //Vin LOW
      }
      else {
        powerUp=true;             //Vin HIGH
      }
      startOutageTime=0;          //reset start time.
    }
  }

  if (powerDown) {
    WiFi.mode(WIFI_OFF);         //shutdown wifi
    Serial.printf("Handling power down now. Wifi disabled\r\n");
    digitalWrite(LED_BUILTIN,HIGH);      //turn on LED
    counter=0;
    if (writeEnabled) {
      Serial.printf("Closing data file...\r\n");
      dataFile.close();
      SD.end();
      Serial.printf("Data file closed, SD unmounted.\r\n");
      writeEnabled=false;
    }
    timer2Enabled=true;         //initiate power cut delay
    endTimer2=millis()+period2;
    powerDown=false;
  }
  
  if (powerUp) {
    Serial.printf("Handling power up now.\r\n");
    counter=0;
    digitalWrite(BATT_EN,HIGH);       //enable battery backup
    digitalWrite(LED_BUILTIN,LOW);    //turn off LED
    timer2Enabled=false;
    openFile();
    
    WiFi.softAP(ssid);     //default Access Point IP 192.168.4.1
    server.begin();
    Serial.printf("Wifi server started, IP address: %s\r\n",WiFi.softAPIP().toString().c_str());
    powerUp=false;
  }

  if (millis() > endTimer1 && timer1Enabled) {       //heartbeat timer expired?
    endTimer1 = millis()+period1;                    //reset HB timer expiry
    counter++;
    float adjustedInputValue = analogReadAdjusted(BATT_SENSE);
    float adjustedInputVoltage = 3.3 / 4096 * adjustedInputValue;
    Serial.printf("status: %d elapsed: %d battery: %f\r\n",status,counter,adjustedInputVoltage);
    if (writeEnabled) {
      sdcounter++;
      sprintf(buf,"%d",sdcounter);
      dataFile.println(buf);                         //log text to SD card
    }
  }

  if (millis() > endTimer2 && timer2Enabled) {       //delay timer expired?
    digitalWrite(BATT_EN,LOW);                       //power cut
  }
  
  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");
            client.print("Click <a href=\"/R\">here</a> to read data.txt.<br>");
            client.print("Click <a href=\"/A\">here</a> to open data.txt for append.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        else if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
        }
        else if (currentLine.endsWith("GET /R")) {      //read data.txt
          writeEnabled=false;
          dataFile.close();
          dataFile = SD.open(filename,FILE_READ);
          if(!dataFile){
            Serial.printf("Failed to open %s\r\n",filename);
          }
          else {
            while(dataFile.available()){
              Serial.write(dataFile.read());
            }
            dataFile.close();
          }
        }
        else if (currentLine.endsWith("GET /A")) {      //open data.txt for append
          dataFile = SD.open(filename, FILE_APPEND);
          if (dataFile) {
            dataFile.println("Data file open:append");
            Serial.printf("Data file open:append\r\n");
            writeEnabled=true;
          }
          else {
             Serial.printf("SD open:append fail\r\n");
          }
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }

}
