/*
 * Digital variable power supply using a LM317 and a MCP4725 DAC to control the output.
 * An INA219 is used to sense the output voltage and show on an OLED display. Note, INA219
 * introduces a burden on the measurement and hence voltage returned do not match values from 
 * a DMM. An offset value is applied to the returned voltage to correct the discrepancy.
 * The output range of the power supply is 1.3V - 6.3V, 1A max with 1.22mV resolution.
 * 4 buttons are used to adjust the output:
 * UP     increase voltage
 * DOWN   decrease voltage
 * ADJ    voltage step(1mV,10mV,01.V,0.5V), fixed 3.3V,5V output.
 * SEL    select the ADJ voltage.
 * Prerequisites:
 *   LM317 power supply, Arduino Nano(5V), MCP4725 12-bit DAC, INA219 current/voltage sensor, 0.96" OLED display.
*/
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include "OneButton.h"
#include "Serial_printf.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>

#define LED_PIN 2               //power on indicator
#define UP_PIN 3                //increase voltage switch
#define DN_PIN 4                //decrease voltage switch
#define ADJ_VOLT_PIN 6          //voltage adj switch
#define ADJ_SEL_PIN 11          //select adj value switch
#define ADJ_XPOS 40             //x-pos for delta adj value
#define VOUT_XPOS 40            //x-pos for Vout/Iout value
#define IOUT_XPOS 40            //x-pos for Iout value
#define CLR_WIDTH 50            //x-pos to clear line
#define LINE_SPACING 10         //spacing between line
#define VOLTAGE_OFFSET 32       //compensate for inaccuracies introduced by INA219(mV)

OneButton up(UP_PIN, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
OneButton down(DN_PIN, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
OneButton adjVoltage(ADJ_VOLT_PIN, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
OneButton adjSelect(ADJ_SEL_PIN, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.

Adafruit_SSD1306 display(-1);
Adafruit_MCP4725 dac;
int dacval=0,line=0;
int deltamV[]={1,10,100,500,3300,5000};       //1,10,100,500mV,3.3V,5V (fixed 3.3 & 5V output)
int delidx=0;
float stepmV,targetmV;
char deltaV[][8]={
  "1 mV","10 mV","0.1 V","0.5 V","3.3 V","5 V"     //adj display values
};
bool ledOn=false;

unsigned long timer1=0;           //loop timer
unsigned int period1 = 1000;       //interval
bool timer1Enabled=true;

unsigned long timer2=0;           //loop timer
unsigned int period2 = 250;       //interval
bool timer2Enabled=false;

Adafruit_INA219 monitor;
/*
void adjSelectClick() {
  if (deltamV[delidx] > 1) {            //delta > 1mV?
    if (deltamV[delidx] > 1000) {       //delta > 3.3V/5V?
      dacval=deltamV[delidx]/stepmV;    //new dac value
      if (dacval > 4095) dacval=4095;
      targetmV=(float)deltamV[delidx]-VOLTAGE_OFFSET;   //slightly reduce targetmV so that auto-adj will correct it.
      dac.setVoltage(dacval, false);    //set DAC to output fixed 3.3V/5V
      Serial_printf(Serial,"dacval %d, targetmV %.3f\r\n",dacval,targetmV);
      timer2Enabled=true;
    }
  }
  display.invertDisplay(true);
  delay(500);
  display.invertDisplay(false);
}
*/

//Select the voltage determined by the ADJ buttton. If voltage=3.3 or 5V, set the DAC output directly.
//Invert(flash) the display for 0.5s to confirm selection.
void adjSelectClick() {
  if (deltamV[delidx] == 3300) {       //3.3V selected
    dacval=1680;                       //compensated for INA219 burden
    dac.setVoltage(dacval, false);     //set DAC to output fixed 3.3V
  }
  else if (deltamV[delidx] == 5000) {  //5V selected.
    dacval=3073;                       //compensated for INA219 burden
    dac.setVoltage(dacval, false);     //set DAC to output fixed 5V
  }
  Serial_printf(Serial,"dacval %d, voltage %.3f\r\n",dacval);
  display.invertDisplay(true);        //flash the OLED display to confirm selection.
  delay(500);
  display.invertDisplay(false);
}

//Step through and display all the adjustment voltages in the array.
void adjVoltageClick() { 
  int maxidx=sizeof(deltamV)/sizeof(deltamV[0]);
  display.fillRect(ADJ_XPOS, 0, CLR_WIDTH, 8,BLACK);   //x,y,width,height,color
  display.setCursor(ADJ_XPOS,0);
  delidx++;
  if (delidx == maxidx)
    delidx=0;
  //Serial_printf(Serial,"maxidx %d delidx %d\r\n",maxidx,delidx);
  display.print(deltaV[delidx]);
  display.display();
}

//Add adjustment voltage to output.
void clickUp() {
  float voltage,current,power;

  if (dacval < 4095) {
    getPowerInfo(&voltage,&current,&power);       //in mV,mA,mW
    targetmV=voltage+(float)deltamV[delidx];      //target voltage
    if (deltamV[delidx] == 1)                     //1mV selected.
      dacval++;
    else
      dacval += deltamV[delidx]/stepmV;           //convert mV to dac value.
    if (dacval > 4095) dacval=4095;
    dac.setVoltage(dacval, false);
  }
  Serial_printf(Serial,"click up %d\r\n",dacval);
}

//Subtract adjustment voltage from output.
void clickDown() {
  float voltage,current,power;

  if (dacval > 0) {
    getPowerInfo(&voltage,&current,&power);       //in mV,mA,mW
    targetmV=voltage-(float)deltamV[delidx];      //target voltage
    if (deltamV[delidx] == 1)                     //1mV selected
      dacval--;
    else
      dacval -= deltamV[delidx]/stepmV;           //convert mV to dac value.
    if (dacval < 0) dacval=0;
    dac.setVoltage(dacval, false);
  }
  Serial_printf(Serial,"click down %d\r\n",dacval);
}

void getPowerInfo(float *voltage,float *current,float *power) {
  float shuntvoltage = monitor.getShuntVoltage_mV();
  float busvoltage = monitor.getBusVoltage_V();
  float current_mA = monitor.getCurrent_mA();
  *power = monitor.getPower_mW();
  *voltage = (busvoltage*1000) + shuntvoltage;    //in mV
  if (current_mA < 0) current_mA=0.0;
  *current=current_mA;
  //Serial_printf(Serial,"volt %.3f, current %.3f, power %.3f\r\n",*voltage,*current,*power);
}

void setup(void) {
  float voltage,current,power;
  
  Serial.begin(115200);
  Serial.println("Digital P/S");
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,LOW);
  dac.begin(0x60);
    
  up.setDebounceTicks(50);
  up.setClickTicks(150);
  up.attachClick(clickUp);
  
  down.setDebounceTicks(50);
  down.setClickTicks(150);
  down.attachClick(clickDown);
  
  adjVoltage.setDebounceTicks(50);
  adjVoltage.setClickTicks(150);
  adjVoltage.attachClick(adjVoltageClick);
  
  adjSelect.setDebounceTicks(50);
  adjSelect.setClickTicks(150);
  adjSelect.attachClick(adjSelectClick);

  stepmV=(float)5000/4096;   //5V VCC/12-bit ADC=1.221mV per division
  dacval=1;                  //start at minimum voltage
  dac.setVoltage(dacval, false);
  delay(50);

  monitor.begin();
  //monitor.setCalibration_32V_2A();        //default
  //monitor.setCalibration_32V_1A();
  monitor.setCalibration_16V_400mA();
  getPowerInfo(&voltage,&current,&power);       //return mV,mA,mW
  targetmV=voltage;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  line=0;
  display.setCursor(0,line);
  display.print("Adj");
  display.setCursor(ADJ_XPOS,line);
  display.print("1 mV");
  line += LINE_SPACING;
  display.setCursor(0,line);
  display.print("Vout");
  line += LINE_SPACING;
  display.setCursor(0,line);
  display.print("Iout");
  display.display();
}

void loop(void) {
  char vbuf[10],ibuf[10];
  float voltage,current,power;

  up.tick();
  down.tick();
  adjVoltage.tick();
  adjSelect.tick();
/*
//auto-adjust LM3127 output voltage (from INA219) to match desired target voltage.
  if (millis() > timer2 && timer2Enabled) {       //heartbeat timer expired?
    timer2 = millis()+period2;                     //reset HB timer expiry
    timer2Enabled=false;

    getPowerInfo(&voltage,&current,&power);       //return mV,mA,mW
    if (voltage > targetmV+stepmV*5) {            //voltage exceed target+margin?
      dacval -= (voltage - targetmV)/stepmV;      //calc new dac value 
      if (dacval < 0) dacval=0;
      dac.setVoltage(dacval, false);              //set new voltage
    }
    else if (voltage < targetmV-stepmV*5) {       //voltage below target+margin?
      dacval += (targetmV - voltage)/stepmV;      //calc new dac value
      if (dacval > 4095) dacval=4095;
      dac.setVoltage(dacval, false);              //set new voltage
    }
    Serial_printf(Serial,"target %.3f, voltage %.3f, dacval %d, stepmV %.3f\r\n",targetmV,voltage,dacval,stepmV);
  }
*/
//display LM317 output voltage, current.
  if (millis() > timer1 && timer1Enabled) {       //heartbeat timer expired?
    timer1 = millis()+period1;                     //reset HB timer expiry

    getPowerInfo(&voltage,&current,&power);       //in mV,mA,mW
    voltage = (voltage+VOLTAGE_OFFSET)/1000;      //add voltage offset to match actual output(measured by DMM).
    if (current < 0) current=0.0;
    Serial_printf(Serial,"dacval %d, Vout %.3f, Iout %.3f\r\n",dacval,voltage,current);
    if (current > 0 && !ledOn) {                  //if current detected, turn on LED
      digitalWrite(LED_PIN,HIGH);
      ledOn=true;
    }
    else if (current < 1 && ledOn) {              //if no current detected, turn off LED
      digitalWrite(LED_PIN,LOW);
      ledOn=false;
    }

    line=10;
    dtostrf(voltage,5,3,vbuf);
    dtostrf(current,5,3,ibuf);
    display.fillRect(VOUT_XPOS, line, CLR_WIDTH, 8,BLACK);  //x,y,width,height,color
    display.setCursor(VOUT_XPOS,line);
    display.print(vbuf);
    display.print(" V");
    line += LINE_SPACING;
    display.fillRect(IOUT_XPOS, line, 88, 8,BLACK);         //x,y,width,height,color
    display.setCursor(IOUT_XPOS,line);
    display.print(ibuf);
    display.print(" mA");
    display.display();

  }
}
