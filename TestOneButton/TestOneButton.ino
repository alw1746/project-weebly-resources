/*
 Test sketch for OneButton library. Works for ATMEGA328P,STM32,ESP32/8266 and cover use cases: 
 click, doubleclick, multiclick, long press. Change pin definitions to suit. Connect button
 to GND and use internal pullup. If internal pullup not available, connect a 10K resistor
 between pin and VCC.
 Requirements:
 https://github.com/mathertel/OneButton
*/
#include "OneButton.h"

#define PIN_INPUT 21
#define PIN_LED 19

OneButton button(PIN_INPUT, true, true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
bool ledRepeat=false;
uint32_t ledStartTime=0;
uint32_t ledPeriod=1000;
int ledCounter=0;
int elapsed=0;

unsigned long endTimer1=0;
unsigned int period1 = 1000;
bool timer1Enabled=true;

//initiate led blink with count,interval and repeatability. call this on button click.
void blinks_begin(int pin,int count,int interval,bool repeat=false) {
  ledStartTime=millis();      //save start time
  ledCounter=(count*2)-1;     //calc countdown counter
  ledPeriod=interval;         //blink interval
  ledRepeat=repeat;           //repeat entire blink cycle flag
  digitalWrite(pin,HIGH);     //LED on
}

//on interval expiry, blink led and counts down the number. Call this from loop() function.
void blinks_countdown(int pin) {
  if (millis() > ledStartTime + ledPeriod && ledCounter > 0) {
    ledStartTime=millis();
    digitalWrite(pin, !digitalRead(pin));
    ledCounter--;
    if (ledCounter == 0 && ledRepeat) {     //if countdown complete and repeat=true
      ledCounter = 1;         //keep doing it
    }
  }
}

void click1() {
  Serial.println("clicks 1");
  blinks_begin(PIN_LED,1,200);
}

void click2() {
  Serial.println("clicks 2");
  blinks_begin(PIN_LED,2,200);
}

void clicks() {
  int numclicks=button.getNumberClicks();      //multiclicks
  Serial.print("clicks ");
  Serial.println(numclicks);
  blinks_begin(PIN_LED,numclicks,500);
}

void longPressStart() {
  Serial.println("longPressStart"); //blink on long press start
  blinks_begin(PIN_LED,2,100);
  elapsed=0;
  endTimer1 = millis()+period1;
}

void longPressDuring() {
  if (millis() > endTimer1 && timer1Enabled) {
    endTimer1 = millis()+period1;
    elapsed++;
    Serial.print("Elapsed ");
    Serial.println(elapsed);
    blinks_begin(PIN_LED,1,500);    //blink regularly during long press
 }
}

void longPressStop() {
  Serial.println("longPressStop");
  blinks_begin(PIN_LED,2,100);      //blink on long press release
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  ledCounter=0;
  button.setDebounceTicks(50);
  button.setClickTicks(150);
  button.attachClick(click1);                     //handle 1 click
  button.attachDoubleClick(click2);               //handle doubleclick
  button.attachMultiClick(clicks);                //handle > 2 clicks
  button.attachLongPressStart(longPressStart);    //handle start of long press
  button.attachDuringLongPress(longPressDuring);  //handle regular activation during long press
  button.attachLongPressStop(longPressStop);      //handle release of long press
}

void loop() {
  button.tick();
  blinks_countdown(PIN_LED);
}
