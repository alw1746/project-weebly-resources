/*
  Control analog joystick using the HID driver of the o/s ie no specific device driver required.
  Joystick simulates pressing WASD keys for gaming purposes, typically moving camera view.
  Handles simultaneous WD,WA,SA,SD presses for NE,NW,SW,SE movements.
  Status led lights up when switch is press-n-held to indicate change of game profile.
  Joystick switch and 4 buttons send programmable strings to host when pressed.
  Games profiles are configured in json strings for future expansion.
  Board support: ATmega32U4(Leonardo,Micro), Digispark(ATTiny85). STM32 not working, unknown USB descriptor.
  For STM32, Tools -> USB Support: HID (keyboard and mouse).
  
  Note: game engines process keypresses(key down) from the keyboard, not characters (keypress codes are not the same
  as character codes). When 2 keys such as W and D are pressed simultaneously, the game engine detects 2 keypresses and
  will move the camera in a NE direction. When W is released, the D keypress remains in effect with camera moving E.

  Add Digispark cursor key defs to ...\packages\digistump\hardware\avr\1.6.7\libraries\DigisparkKeyboard\DigiKeyboard.h
  #define KEY_UP_ARROW 0x52
  #define KEY_DOWN_ARROW 0x51
  #define KEY_LEFT_ARROW 0x50
  #define KEY_RIGHT_ARROW 0x4F

  MIT License
  Copyright (c) 2020 Alexs Wong
  
 */
#include "Button.h"
#include <ArduinoJson.h>

#define DEBUG_LEVEL 1       //0=off,1=verbose,2=verbosier,3=verbosiest
#define MAX_BUFLEN 16

#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_ARCH_STM32)
  #include "Serial_printf.h"
  #include "Keyboard.h"
  #if defined(ARDUINO_AVR_MICRO)
    #define LED_STATUS 10     //alternative RXLED=17 
    #define ANALOGX A1
    #define ANALOGY A2
    #define JOYBTN 21
    #define BTN1 6
    #define BTN2 7
    #define BTN3 8
    #define BTN4 9
  #elif defined(ARDUINO_ARCH_STM32)
    #define LED_STATUS PC13
    #define ANALOGX PA0
    #define ANALOGY PA1
    #define JOYBTN PA2
    #define BTN1 PA8
    #define BTN2 PA9
    #define BTN3 PB1
    #define BTN4 PB2
  #endif
#elif defined(ARDUINO_AVR_DIGISPARK)
  #include "DigiKeyboard.h"
  #define LED_STATUS 1
  #define ANALOGX 0    //PB5
  #define ANALOGY 1    //PB2
  #define JOYBTN 0
  #define BTN1 2
  #define BTN2 3
  #define BTN3 4
  #define BTN4 5
  
  #include <SoftSerial_INT0.h>
  #define DEBUG_TX_RX_PIN 1
  //SoftSerial Serial(DEBUG_TX_RX_PIN, DEBUG_TX_RX_PIN);
#endif

ButtonCB joybtn(JOYBTN, Button::INTERNAL_PULLUP);  //joystick button with callback
ButtonCB btn1(BTN1, Button::INTERNAL_PULLUP);      //button with callback
ButtonCB btn2(BTN2, Button::INTERNAL_PULLUP);      //button with callback
ButtonCB btn3(BTN3, Button::INTERNAL_PULLUP);      //button with callback
ButtonCB btn4(BTN4, Button::INTERNAL_PULLUP);      //button with callback

const char* Homeworld = "{\"name\":\"Homeworld\",\"up\":218,\"down\":217,\"left\":216,\"right\":215,\"joybtn\":210,\"btn1_data\":\" \",\"btn2_data\":\"f\",\"btn3_data\":\"b\",\"btn4_data\":\"j\"}";
const char* WorldInConflict = "{\"name\":\"World of Conflict\",\"up\":119,\"down\":115,\"left\":97,\"right\":100,\"joybtn\":32,\"btn1_data\":\"f\",\"btn2_data\":\"g\",\"btn3_data\":\"y\",\"btn4_data\":\"u\"}";
char buf[200];

//ADC values at various joystick positions: W1023 E0, N1023 S0, center X520 Y498
//Joystick orientation: board pins pointing to right.

//If analogX or Y ADC value is within(or exceed) threshold limits below, the joystick position is confirmed.
#define W_X  550
#define E_X  450
#define N_Y  550
#define S_Y  450
#define NE_X 100
#define NE_Y 900
#define NW_X 900
#define NW_Y 900
#define SE_X 100
#define SE_Y 100
#define SW_X 900
#define SW_Y 100
int x,y;
int pos=0,prevpos=0;
bool configMode=false;

struct Profile {
  char name[32];
  byte up;
  byte down;
  byte left;
  byte right;
  byte joybtn;
  char btn1_data[MAX_BUFLEN];
  char btn2_data[MAX_BUFLEN];
  char btn3_data[MAX_BUFLEN];
  char btn4_data[MAX_BUFLEN];
};
Profile profile;    //game profile

void initProfile(char *json) {
  StaticJsonDocument<200> doc;     //arduinojson.org/v6/assistant to compute the capacity.
  DeserializationError error = deserializeJson(doc, json);    //zero-copy mode(requires writable input)
  if (error) {
    #if DEBUG_LEVEL >= 1
      Serial_printf(Serial,"Deserialize error, %s\n",error.c_str());
    #endif
  }
  else {
    strlcpy(profile.name,doc["name"],sizeof(profile.name));
    profile.up=doc["up"];
    profile.down=doc["down"];
    profile.left=doc["left"];
    profile.right=doc["right"];
    profile.joybtn=doc["joybtn"];
//    strlcpy(profile.joybtn_data,doc["joybtn_data"],sizeof(profile.joybtn_data));
    strlcpy(profile.btn1_data,doc["btn1_data"],sizeof(profile.btn1_data));
    strlcpy(profile.btn2_data,doc["btn2_data"],sizeof(profile.btn2_data));
    strlcpy(profile.btn3_data,doc["btn3_data"],sizeof(profile.btn3_data));
    strlcpy(profile.btn4_data,doc["btn4_data"],sizeof(profile.btn4_data));
    #if DEBUG_LEVEL >= 1
      Serial_printf(Serial,"name %s, up %d, btn1_data %s\n",profile.name,profile.up,profile.btn1_data);
    #endif
  }
}

void onHoldButton(const Button& btn) {
  if (configMode) {
    setLed(LED_STATUS,0);
    configMode=false;
    strcpy(buf,WorldInConflict);    //create writable input
    initProfile(buf);
  }
  else {
    setLed(LED_STATUS,1);
    configMode=true;
    strcpy(buf,Homeworld);         //create writable input
    initProfile(buf);
  }
}

void onPressButton(const Button& btn) {
  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"Btn %d press\n",btn.pin());
  #endif
  switch (btn.pin()) {
    case BTN1:
      sendKeystroke(profile.btn1_data);
      break;
    case BTN2:
      sendKeystroke(profile.btn2_data);
      break;
    case BTN3:
      sendKeystroke(profile.btn3_data);
      break;
    case BTN4:
      sendKeystroke(profile.btn4_data);
      break;
    default:
      break;
  }
}

void onClickButton(const Button& btn) {
  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"Btn %d click\n",btn.pin());
  #endif
  switch (btn.pin()) {
    case JOYBTN:
      sendKeystroke(profile.joybtn);
      break;
    default:
      break;
  }
}

//send key press to HID driver. Key is pressed and held.
void sendKeypress(byte keychr) {
  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"keypress %c %x\n",keychr,keychr);
  #endif
#if defined(__AVR_ATmega32U4__)  || defined(ARDUINO_ARCH_STM32)
  Keyboard.press(keychr);
#elif defined(ARDUINO_AVR_DIGISPARK)
  DigiKeyboard.sendKeyPress(keychr);
#endif
}

//send keystroke to HID driver. Key is pressed and released.
void sendKeystroke(byte keychr) {
  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"keystroke %c %x\n",keychr,keychr);
  #endif
#if defined(__AVR_ATmega32U4__)  || defined(ARDUINO_ARCH_STM32)
  Keyboard.write(keychr);
#elif defined(ARDUINO_AVR_DIGISPARK)
  DigiKeyboard.sendKeyStroke(keychr);
#endif
}

//send string of keystrokes to HID driver. Keys are pressed and released.
void sendKeystroke(char *keystring) {
  char *p=keystring;
  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"keystring %s\n",p);
  #endif
  while (*p != '\0') {
  #if defined(__AVR_ATmega32U4__)  || defined(ARDUINO_ARCH_STM32)
    Keyboard.write(*(p++));
  #elif defined(ARDUINO_AVR_DIGISPARK)
    DigiKeyboard.sendKeyStroke(*(p++));
  #endif
  }
}

//send key release to HID driver.
void freeKey(byte keychr) {
#if defined(__AVR_ATmega32U4__)  || defined(ARDUINO_ARCH_STM32)
  Keyboard.release(keychr);
#elif defined(ARDUINO_AVR_DIGISPARK)
  DigiKeyboard.sendKeyPress(0,0);
#endif
}

//send all key release to HID driver.
void freeKeyAll() {
#if defined(__AVR_ATmega32U4__)  || defined(ARDUINO_ARCH_STM32)
  Keyboard.releaseAll();
#elif defined(ARDUINO_AVR_DIGISPARK)
  DigiKeyboard.sendKeyPress(0,0);
#endif
}

//turn LED on/off. 
void setLed(int ledpin,int state) {
  digitalWrite(ledpin,state);
}

void setup() {
  #if DEBUG_LEVEL >= 1
    Serial.begin(9600);
  #endif
  pinMode(LED_STATUS, OUTPUT);        //led
  setLed(LED_STATUS,0);
  #if defined(__AVR_ATmega32U4__) || defined(ARDUINO_AVR_DIGISPARK)
    ADCSRA =  bit (ADEN);
    ADCSRA |= bit (ADPS2);             //1mhz ADC clock
  #endif
  #if defined(__AVR_ATmega32U4__)
    Keyboard.begin();
  #endif
  analogRead(ANALOGX);  //always discard first analogread
  analogRead(ANALOGY);

  joybtn.setHoldHandler(onHoldButton);    //define button callback
  joybtn.setClickHandler(onClickButton);  //define button callback
  btn1.setPressHandler(onPressButton);    //define button callback
  btn2.setPressHandler(onPressButton);    //define button callback
  btn3.setPressHandler(onPressButton);    //define button callback
  btn4.setPressHandler(onPressButton);    //define button callback
  
  strcpy(buf,WorldInConflict);
  initProfile(buf);

  #if DEBUG_LEVEL >= 1
    Serial_printf(Serial,"Default game profile %s\n",profile.name);
  #endif
}

void loop() {
  joybtn.process();
  btn1.process();
  btn2.process();
  btn3.process();
  btn4.process();

  x = analogRead(ANALOGX);          //Read X  idle 522
  y = analogRead(ANALOGY);          //Read Y  idle 498
  #if DEBUG_LEVEL >= 2
    Serial_printf(Serial,"X: %d Y: %d\n",x,y);
  #endif
  if ((x < W_X && x > E_X) && (y > S_Y && y < N_Y)) {     //compare ADC values against threshold limits.
    pos=0;     //center
  }
  else if ((x < W_X && x > E_X) && (y > N_Y)) {
    pos=1;     //N
  } else if ((x < E_X) && (y > S_Y && y < N_Y)) {
    pos=2;     //E
  } else if ((x < W_X && x > E_X) && (y < S_Y)) {
    pos=3;     //S
  } else if ((x > W_X) && (y > S_Y && y < N_Y)) {
    pos=4;     //W
  } else if (x < NE_X && y > NE_Y) {
    pos=5;     //NE
  }
  else if (x < SE_X && y < SE_Y) {
    pos=6;     //SE
  }
  else if (x > SW_X && y < SW_Y) {
    pos=7;     //SW
  }
  else if (x > NW_X && y > NW_Y) {
    pos=8;     //NW
  }

  #if DEBUG_LEVEL >= 2
    Serial_printf(Serial,"pos: %d\n",pos);
  #endif
  if (pos != prevpos) {     //change direction if joystick position has changed
    freeKeyAll();
    switch (pos) {
      case 0:
        break;
      case 1:  //N
        sendKeypress(profile.up);
        break;
      case 2:  //E
        sendKeypress(profile.right);
        break;
      case 3:  //S
        sendKeypress(profile.down);
        break;
      case 4:  //W
        sendKeypress(profile.left);
        break;
      case 5:  //NE
        #if EMULATE_ARROW_KEYS != 1
          sendKeypress(profile.right);  //NB: digispark does not support multiple keypresses.
          sendKeypress(profile.up);     //so it will only send the last press ie up.
        #endif
        break;
      case 6:  //SE
        #if EMULATE_ARROW_KEYS != 1
          sendKeypress(profile.right);
          sendKeypress(profile.down);
        #endif
        break;
      case 7:  //SW
        #if EMULATE_ARROW_KEYS != 1
          sendKeypress(profile.left);
          sendKeypress(profile.down);
        #endif
        break;
      case 8:  //NW
        #if EMULATE_ARROW_KEYS != 1
          sendKeypress(profile.left);
          sendKeypress(profile.up);
        #endif
        break;
      default:
        break;
    }
    prevpos=pos;
  }

  #if defined(ARDUINO_AVR_DIGISPARK)
     //DigiKeyboard.delay(500);
     DigiKeyboard.update();
  #endif
  delay(100);
}
