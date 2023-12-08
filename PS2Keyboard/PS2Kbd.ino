/*
 * A simple GUI to control GPIO pins on Arduino Nano using a USB keyboard with a PS/2 adapter.
 * Pins operations are:
 *   Digital Read
 *   Digital Write
 *   Analog Read
 *   Analog Write(PWM pin 5-6)
 * A 240x240 TFTLCD is used for display. A thermistor provides input for analogRead, a pushbutton
 * for digitalRead and an LED for digital/analogWrite. As the TFTLCD is a 3.3V device, a logic 
 * level converter is used on all LCD pins. The USB adapter draws a lot of current so a separate 
 * 5V supply powers the keyboard. PS2KeyAdvanced library installed with Arduino IDE.
 * Note: occasionally the keyboard locks up due to an unknown issue and the resolution is plug
 * keyboard into a PC and type something. Powercycle the Nano and reconnect keyboard.
 * 
  6-pin mini DIN socket (front facing):
  Data  o  o
  Gnd  o    o +5V
  CLK   o[]o
*/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <PS2KeyAdvanced.h>
#include "Serial_printf.h"

#define DATAPIN 3   //data pin
#define IRQPIN  2   //clock pin
#define LED 5       //test led
#define BUTTON 7    //test pushbutton switch
#define THERMISTOR 15       //test thermistor
#define THERMISTOR_PWR 14   //thermistor power
#define START_MENU_LINE 1   //skip menu header at line 0
#define MSG_Y 210           //Y-coord of msg line
#define ERROR_Y 170         //Y-coord of error line
#define IO_FIELD_X 130      //X-coord of input/output field
#define ESC_KEY -1          //ESC key detected
#define LINESPACING 10      //pixels between lines on TFTLCD
#define TFT_CS        10 //SS
#define TFT_RST       9  //***connect to mcu RESET does not work, must use a pin.***
#define TFT_DC        8  //data/command

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
PS2KeyAdvanced keyboard;
uint16_t menuLine,fontscale=4,fontx=6,fonty=8;
uint32_t startMillis;
char buf[12];
int buflen;
const char* main_menu_lines[]={
  "  GPIO",     //menu header
  "DG Read",
  "DG Write",
  "AN Read",
  "AN Write",
};
int main_menu_count=sizeof(main_menu_lines)/sizeof(main_menu_lines[0]);
enum stateName {START,PIN,VALUE,PIN_WRITE,EXIT};
enum eventName {INIT,READ_KBD,READ_COMPLETED,PIN_READ,SLEEP};
enum modeName {ANALOG,DIGITAL};
enum menuName {DIGITAL_READ=START_MENU_LINE,DIGITAL_WRITE,ANALOG_READ,ANALOG_WRITE};
stateName state;
eventName event;

bool isNumber(char* line) {
  char* p;
  strtol(line, &p, 10);
  return *p == 0;
}

int read_kbd(bool acceptData) {
//reads keyboard input into a global buffer until ENTER/ESC. ENTER terminates the
//input and string length returned. ESC indicates an EXIT condition detected.
//No line editing support, ESC and start again for errors.
//Parameters:
//  acceptData    true=return all input, false=return control keys only eg ESC.
//Return:
//  string length or EXIT condition.
  int length=0;
  
  while (keyboard.available() > 0) {
    uint16_t c=keyboard.read();     //get key code
    if (!(c & 0x8000)) {
      c = c & 0xFF;
      //Serial_printf(Serial,"%X",c);
      if (c == PS2_KEY_ESC) {       //EXIT condition
          length = ESC_KEY;
          buflen=0;     //reset buflen ready for next new request
          break;
      }
      else if (c == PS2_KEY_ENTER) {   //end of input
        length=buflen;
        buf[buflen]='\0';
        buflen=0;     //reset buflen ready for next new request
        break;
      }
      else if (acceptData && buflen < sizeof(buf)) { 
        buf[buflen++]=c;              //global buffer
        tft.print((char)c);
      }
    }
  }
  return length;
}

bool sleep(unsigned long periodMs,unsigned long startMillis) {
//non-blocking delay timer.
//Return:
//  true    in delay mode
//  false   delay completed

  bool timerEnabled=true,isDelayed=true;

  if ((millis() - startMillis > periodMs) && timerEnabled) {
    isDelayed=false;
  }
  return isDelayed;
}

void clrErrmsg() {
  tft.fillRect(0,ERROR_Y,tft.width(),fontscale*fonty,ST77XX_BLACK);
  tft.setTextSize(fontscale);
  tft.setTextColor(ST77XX_WHITE);
}

void showErrmsg(char* msg) {
  tft.setTextSize(fontscale-1);   //use smaller size for longer msgs.
  tft.setCursor(0,ERROR_Y);
  tft.setTextColor(ST77XX_RED,ST77XX_GREEN);
  tft.print(msg);
  //Serial_printf(Serial,"%s\r\n",msg);
}

void errPause(int periodMs) {
//sleep until timer completes, clear errmsg line and restart operation.
  int kbdlen=read_kbd(false);
  if (!sleep(periodMs,startMillis) || kbdlen == ESC_KEY) {
    clrErrmsg();
    event=INIT;
  }
}

void startScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(fontscale-1);
  tft.setCursor(0,MSG_Y);
  tft.print("ESC to exit");
  tft.setTextSize(fontscale);
  state=PIN;
  event=INIT;
}

int pinField(int line) {
//get pin number.
  int x,y,data,kbdlen;

  switch (event) {
    case READ_COMPLETED:
        //Serial_printf(Serial,"buf %s\r\n",buf);
      if (!isNumber(buf)) {
        showErrmsg("Bad number");
        startMillis=millis();
        event=SLEEP;
      }
      else {
        sscanf(buf,"%d",&data);   //convert input string to pin
        if (data < 2 || data > 19) {
          showErrmsg("Use 2..19 only");
          startMillis=millis();
          event=SLEEP;
        }
        else {
          state=VALUE;    //next state
          event=INIT;     //and event.
        }
      }
      break;
    case SLEEP:
      errPause(3000);
      break;
    case INIT:
      x=0;
      y=((line-1)*fonty*fontscale);
      tft.setCursor(x,y);
      tft.print("Pin ");
      x=IO_FIELD_X;
      tft.setCursor(x,y);
      tft.fillRect(x,y,tft.width()-x,fontscale*fonty,ST77XX_BLACK);
      y=line*fontscale*fonty;
      tft.drawLine(x,y,tft.width(),y,ST77XX_WHITE);
      event=READ_KBD;
      break;
    case READ_KBD:
      kbdlen=read_kbd(true);
      if (kbdlen == ESC_KEY) {
        state=EXIT;
      }
      else if (kbdlen > 0) {
        event=READ_COMPLETED;
      }
      break;
    default:
      break;
  }
  return data;
}

int valueField(int line,int minval,int maxval) {
//get value to write to pin.
  int x,y,data,kbdlen;
  char msg[30];

  switch (event) {
    case READ_COMPLETED:
      //Serial_printf(Serial,"buf %s\r\n",buf);
      if (!isNumber(buf)) {
        showErrmsg("Bad number");
        startMillis=millis();
        event=SLEEP;
      }
      else {
        sscanf(buf,"%d",&data);
        if (data < minval || data > maxval) {
          sprintf(msg,"Use %d..%d only",minval,maxval);
          showErrmsg(msg);
          startMillis=millis();
          event=SLEEP;
        }
        else {
          state=PIN_WRITE;    //next state
        }
      }
      break;
    case SLEEP:
      errPause(3000);
      break;
    case INIT:
      x=0;
      y=((line-1)*fonty*fontscale)+LINESPACING;
      tft.setCursor(x,y);
      tft.print("Value ");
      x=IO_FIELD_X;
      tft.setCursor(x,y);
      tft.fillRect(x,y,tft.width()-x,fontscale*fonty,ST77XX_BLACK);
      y=(line*fontscale*fonty)+LINESPACING;
      tft.drawLine(x,y,tft.width(),y,ST77XX_WHITE);
      event=READ_KBD;
      break;
    case READ_KBD:
      kbdlen=read_kbd(true);
      if (kbdlen == ESC_KEY) {
        state=EXIT;
      }
      else if (kbdlen > 0) {
        event=READ_COMPLETED;
      }
      break;
    default:
      Serial_printf(Serial,"state %d event %d\r\n",state,event);
      break;
  }
  return data;
}

void pinWrite(modeName mode,int minval,int maxval) {
//get pin number, pin output value and write to pin.
//Parameters:
//mode     ANALOG or DIGITAL
//minval   min value range
//maxval   max value range
  int pin,value;
  bool exit=false;

  state=START;
  while (!exit) {
    switch (state) {
      case START:
        startScreen();    //initialise screen
        break;
      case PIN:
        pin=pinField(1);  //get pin number at line 1
        break;
      case VALUE:
        value=valueField(2,minval,maxval);    //get pin value at line 2
        break;
      case PIN_WRITE:     //write value to pin
        if (mode == ANALOG)
          analogWrite(pin,value);
        else
          digitalWrite(pin,value);
        //Serial_printf(Serial,"mode %d, pin %d, value %d\r\n",mode,pin,value);
        state=START;      //repeat operation
        break;
      case EXIT:
        exit=true;        //return to prev menu
        break;
      default:
        break;
    }
  }
}

void showValue(modeName mode,int line,int pin) {
//read pin and display value in a loop until ESC entered.
  int msglen,value,x,y,kbdlen;
  static int xsave,ysave;
  static uint16_t wsave,hsave;
  char msg[5];

  switch (event) {
    case SLEEP:
      kbdlen=read_kbd(false);
      if (!sleep(250,startMillis))  {
        tft.fillRect(xsave,ysave,wsave,hsave,ST77XX_BLACK);     //clear output field
        tft.setCursor(xsave,ysave);     //reposition at start of field
        event=PIN_READ;                 //read pin
      }
      else if (kbdlen == ESC_KEY) {     //EXIT condition
        state=START;
        event=INIT;
      }
      break;
    case INIT:
      x=0;
      y=((line-1)*fonty*fontscale)+LINESPACING;
      tft.setCursor(x,y);
      tft.print("Value ");
      x=IO_FIELD_X;
      tft.setCursor(x,y);
      tft.fillRect(x,y,tft.width()-x,fontscale*fonty,ST77XX_BLACK);    //clear output field
      xsave=x;      //remember output field X
      ysave=y;      //remember output field Y
      hsave=fontscale*fonty;    //remember output field height
      event=PIN_READ;
      break;
    case PIN_READ:
      if (mode == ANALOG)
        value=analogRead(pin);
      else
        value=digitalRead(pin);
      sprintf(msg,"%d",value);        //convert value to string
      msglen=strlen(msg);             //get output string length
      wsave=fontscale*fontx*msglen;   //remember string width
      tft.print(msg);                 //display pin value
      //Serial_printf(Serial,"mode %d, pin %d, value %d,msg %s\r\n",mode,pin,value,msg);
      //Serial_printf(Serial,"xsave %d, ysave %d, wsave %d, hsave %d\r\n",xsave,ysave,wsave,hsave);
      startMillis=millis();
      event=SLEEP;
      break;
    default:
      break;
  }
}

void pinRead(modeName mode) {
//get pin number, read pin value and display in a loop until EXIT.
  int pin;
  bool exit=false;

  state=START;
  while (!exit) {
    switch (state) {
      case START:
        startScreen();    //initialise screen
        break;
      case PIN:
        pin=pinField(1);  //get pin number at line 1
        break;
      case VALUE:
        showValue(mode,2,pin);    //display value in a loop.
        break;
      case EXIT:
        exit=true;
        break;
      default:
        break;
    }
  }
}

void removeHighlight(int menuLine) {
//remove highlight from menu line
  int x=0,y=0;

  y=menuLine*fontscale*fonty;
  tft.fillRect(x,y,tft.width(),fontscale*fonty,ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(x,y);
  tft.print(main_menu_lines[menuLine]);
}

void highlight(int menuLine) {
//highlight menu line
  int x=0,y=0;

  y=menuLine*fontscale*fonty;
  tft.fillRect(x,y,tft.width(),fontscale*fonty,ST77XX_YELLOW);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(x,y);
  tft.print(main_menu_lines[menuLine]);
}

void mainMenu() {
//display main menu.
  tft.setTextSize(fontscale);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE);
  for (int i=0; i < main_menu_count; i++) {
    tft.println(main_menu_lines[i]);
  }
}

void setup() {
  Serial.begin( 115200 );
  pinMode(BUTTON,INPUT_PULLUP);
  pinMode(LED,OUTPUT);
  pinMode(THERMISTOR_PWR,OUTPUT);
  digitalWrite(THERMISTOR_PWR,HIGH);
  Serial_printf(Serial,"GPIO menu v1.0\r\n");
  keyboard.begin( DATAPIN, IRQPIN );
  //***must specify SPI_MODE3 for TFTLCD without CS pin otherwise black flickering screeen.***
  tft.init(240, 240, SPI_MODE3);           // Init ST7789 240x240.
  tft.setRotation(2);     //set origin to top left where lcd pins are
  tft.setTextWrap(false);
  menuLine=START_MENU_LINE;     //skip header line
  mainMenu();
  highlight(menuLine);
}

void loop() {
  if (keyboard.available()) {
    uint16_t chr=keyboard.read();     //read keyboard
    if (!(chr & 0x8000)) {            //reject key release detection
      chr = chr & 0xFF;               //get key ASCII code
      switch (chr) {
        case PS2_KEY_UP_ARROW:
          //Serial_printf(Serial,"UP\r\n");
          if (menuLine > START_MENU_LINE) {
            removeHighlight(menuLine);
            highlight(--menuLine);
          }
          break;
        case PS2_KEY_DN_ARROW:
          //Serial_printf(Serial,"DN\r\n");
          if (menuLine < (main_menu_count-START_MENU_LINE)) {
            removeHighlight(menuLine);
            highlight(++menuLine);
          }
          break;
        case PS2_KEY_ENTER:
          switch (menuLine) {
            case DIGITAL_READ:
              pinRead(DIGITAL);
              break;
            case DIGITAL_WRITE:
              pinWrite(DIGITAL,0,1);
              break;
            case ANALOG_READ:
              pinRead(ANALOG);
              break;
            case ANALOG_WRITE:
              pinWrite(ANALOG,0,255);
              break;
            default:
              break;
          }
          mainMenu();
          highlight(menuLine);
          break;
        default:
          break;
      }
    }
  }
}
