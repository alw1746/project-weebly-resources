/*
 * Display bitmap patterns from an array on a single 7-segment display.
 * -Alphanumeric characters based on ASCII table.
 * -Snake-like rotating display for fun.
 * Pushbutton selects modes of display. 1-click reverses direction of rotation.
 * 2-clicks changes display to ASCII chars. Undefined chars are indicated by decimal pt(.)
 * Linear potentiometer vary delay between each bitmap display.
 * 
 Requirements:
 https://github.com/mathertel/OneButton
*/
#include "OneButton.h"
#include "bmp7seg.h"

#define PIN_INPUT 11
#define PIN_LED LED_BUILTIN
#define analogPin A3

unsigned long timer1=0;               //loop timer
unsigned int period1 = 1000;          //interval
bool timer1Enabled=true;

OneButton button(PIN_INPUT, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
int idx,startidx,endidx,state;

void click1() {
  Serial.println("clicks 1");
  switch (state) {
    case 0:
      state=1;  //change snake to clockwise rotation
      break;
    case 1:
      state=0;  //change snake to anti-clockwise rotation
      break;
    case 2:
      state=0;                //reset to snake display
      startidx=0;             //array start index = 0
      endidx=bmpSnakeLen-1;   //array end index = length-1
      idx=startidx;
      break;
  }
}

void click2() {
  Serial.println("clicks 2");
  state=2;      //ASCII display
  startidx=48;  //0
  endidx=121;   //y
  idx=startidx;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED,LOW);
  state=0;                   //start with clockwise snake rotation
  startidx=0;                //array start index = 0
  endidx=bmpSnakeLen-1;      //array end index = length-1
  idx=startidx;
  DDRD = DDRD | B11111100;   //set PORTD bit 2-7(preserve 0-1) as output
  DDRB = DDRB | B00000011;   //set PORTB bit 0-1(preserve 2-7) as output
  button.setDebounceTicks(50);
  button.setClickTicks(150);
  button.attachClick(click1);          //handle 1 click
  button.attachDoubleClick(click2);    //handle doubleclick
  analogRead(analogPin);               //always drop 1st read
  timer1 = millis();                   //init timer1 delay
}

void loop() {
  button.tick();   //process button clicks
  if (millis() > timer1 && timer1Enabled) {       //timer delay
    period1 = analogRead(analogPin);              //get pot value for delay
    if (state == 2) {
      timer1 = millis()+period1;                  //set timer delay expiry
      printIndex(idx);                            //print ASCII table
      idx++;
      if (idx > endidx) idx=startidx;             //wraparound
    }
    else {
      period1=map(period1,0,1023,25,150);         //reduce to a more useful range
      timer1 = millis()+period1;                  //set timer delay expiry
      printSnake(idx);      //print snake
      if (state == 0) {     //anti-clockwise
        idx++;
        if (idx > endidx) idx=startidx;     //wraparound
      } else if (state == 1) {              //clockwise
        idx--;
        if (idx < startidx) idx=endidx;     //wraparound
      }
    }
  }
}
