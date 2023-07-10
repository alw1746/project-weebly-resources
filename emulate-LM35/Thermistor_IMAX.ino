/*
Emulate an LM35 temperature sensor using a thermistor and DAC. 
The sensor outputs 10mV per degree of temperature change and is
emulated by sensing the the temperature with a cheap thermistor
and output the corresponding voltage.
Hardware:
  Arduino Nano
  33K@25C Vishay thermistor (NTCLE100E3333JB0)
  MCP4725 12-bit DAC
 */
#include <math.h>
#include <Adafruit_MCP4725.h>
#include "Serial_printf.h"
#include <Wire.h>

Adafruit_MCP4725 dac;
float stepmv_DAC;
int dacval=0;

unsigned long timer1=0;               //loop timer
unsigned int period1 = 1000;          //interval msec
bool timer1Enabled=true;

#define ADC_RESOLUTION 10            //ADC resolution in bits
#define DAC_RESOLUTION 12            //DAC resolution in bits
#define ThermistorPin1 A1            //thermistor between analog pin and GND
#define fixedResistance 32600.0      //33K resistor between analog pin and VCC
#define refResistance   33000.0      //thermistor resistance 33K at 25C

int maxadc=pow(2,ADC_RESOLUTION)-1;    //2**ADC resolution bits
int maxdac=pow(2,DAC_RESOLUTION)-1;    //2**DAC resolution bits
float a1=3.354016e-3, b1=2.519107e-4, c1=3.510939e-6, d1=1.105179e-7;        //Vishay datasheet for 2322 640 6.333 (B-4090) (P/N NTCLE100E3333JB0)
float logR,trmtmp,adc,thermistorResistance;

void setup()
{ 
  Serial.begin(115200);     //serial monitor
  //Serial.println("Thermistor IMAX v1.0");
  analogRead(ThermistorPin1);      //always drop 1st read  
  dac.begin(0x60);
  stepmv_DAC=(float)5000/maxdac;   //VCC/DAC resolution=mV per step
}

void loop()
{
  int sample,NUMSAMPLES=5;
  
  if (millis() > timer1 && timer1Enabled) {       //heartbeat timer expired?
    timer1 = millis()+period1;                    //reset HB timer expiry

    adc=0;
    for (byte i=0; i< NUMSAMPLES; i++) {
      sample = analogRead(ThermistorPin1);
      //Serial_printf(Serial,"sample:%d\r\n",sample);
      adc += sample;
      delay(10);
    }
    adc /= NUMSAMPLES;          //average the ADC readings
    thermistorResistance = fixedResistance*adc/(maxadc-adc);
    logR = log(thermistorResistance/refResistance);
    trmtmp = (1.0 / (a1 + b1*logR + c1*logR*logR + d1*logR*logR*logR));   //int sensor
    trmtmp = (trmtmp - 273.15);
    //Serial_printf(Serial,"ADC:%f maxadc:%d FR:%f TR:%f\r\n",adc,maxadc,fixedResistance,thermistorResistance);
    int mV=round(trmtmp*10);    //10mV per degree C
    dacval=mV/stepmv_DAC;
    if (dacval < 0)
      dacval=0;
    else if (dacval > maxdac)
      dacval=maxdac;
    //Serial_printf(Serial,"Temperature:%.2fC DAC:%d mV:%d Step:%.2f\r\n",trmtmp,dacval,mV,stepmv_DAC);
    dac.setVoltage(dacval, false);    //output mV from DAC
  }
}
