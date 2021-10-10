/***************************************************************
Touchscreen paint: screen painting application derived from Adafruit
touchpaint example in https://github.com/adafruit/Adafruit_ILI9341.

Enhanced with more colour selection, clear screen, variable pen width selection, line or icon drawing.
TFT_eSPI\examples\Generic\Touch_calibrate sketch must be run first to obtain calibration data used by tft.setTouch.

Prerequisites:
  3.5" Raspberry Pi tft lcd display (SPI interface)
  STM32L432KC/ESP32/ESP8266 mcu
  Arduino Core for STM32/ESP32
  TFT_eSPI library https://github.com/Bodmer/TFT_eSPI
  Customised TFT_eSPI\User_Setup.h 
  Additional fonts and icon include files
***************************************************************/
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include "Alert.h"
#include "Close.h"
#include "Info.h"
#include "Ferrari.h"

#define BOXSIZE 40                //colour box size
#define HALFBOX BOXSIZE/2
#define MIN_PENRADIUS 2
#define MAX_PENRADIUS 20
#define ICON_X (BOXSIZE*7) + HALFBOX + 10   //xcoord of icon home is 10px to right of midpoint
#define ICON_Y HALFBOX                      //midpoint ycoord of icon home
#define ICON_RADIUS 20                      //icon size

#define BUTTON_X 40       //CLR button specs
#define BUTTON_Y 100
#define BUTTON_W 50
#define BUTTON_H 40
#define BUTTON_TEXTSIZE 2

TFT_eSPI tft = TFT_eSPI();
TFT_eSPI_Button buttonCLR;

#define TS_TEXT_ROTATE 1  //same as Touch_calibrate. LCD header top left corner
uint16_t calData[5]={240, 3632, 240, 3632, 3};         //data from Touch_calibrate. LCD header top left corner

//#define TS_TEXT_ROTATE 3  //same as Touch_calibrate. LCD header bottom right corner
//uint16_t calData[5] = { 240, 3632, 240, 3632, 5 };   //data from Touch_calibrate. LCD header bottom right corner

//iSym=line drawing mode, others=icon drawing mode. Star must be terminate the list and all numbers must be contiguous.
const int iSym=0,Smile=1,Sad=2,Alert=3,Close=4,Info=5,Ferrari=6,Star=7;

bool iconMode=false;
int oldcolor, currentcolor,PSZx,PSZy,penradius,ts_rotate,icon;
int upt,upb,upl,upr,dnt,dnb,dnl,dnr;

void setup()
{
  int x,y;
  
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.println("Start TSpaint 1.1");
  tft.init();
  tft.setRotation(TS_TEXT_ROTATE);   //text display orientation
  tft.fillScreen(TFT_BLACK);
  tft.setTouch(calData);

  x=(tft.width()/2) - 180;
  y=(tft.height()/2)-100;
  tft.fillRect(x, y, 360, 200, TFT_YELLOW);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(FF48);
  tft.setCursor(x+60, y+60);
  tft.print("TSPaint v1.1");
  tft.setCursor(x+160, y+110);
  tft.print("by");
  tft.setCursor(x+80, y+160);
  tft.print("Alex Wong");
  delay(2000);
  tft.fillRect(x, y, 360, 200, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(GLCD);

//draw the color selection boxes
//set default color as TFT_RED
  currentcolor = TFT_RED;
  icon=iSym;                  //icon for line drawing mode
  iconMode=false;
  
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, TFT_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, TFT_YELLOW);
  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, TFT_GREEN);
  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, TFT_CYAN);
  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, TFT_BLUE);
  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, TFT_MAGENTA);
  tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
  drawIcon(&tft,icon,ICON_X, ICON_Y, ICON_RADIUS, currentcolor);   //start off in line drawing mode
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, TFT_WHITE);   //draw white outline around default box

  penradius=MIN_PENRADIUS;     //default pen width
  PSZx=BOXSIZE*9;              //xcoord of Pressure Sensing Zone(PSZ) on screen
  PSZy=HALFBOX;                //ycoord of Pressure Sensing Zone(PSZ) on screen

  tft.drawCircle(PSZx,PSZy,HALFBOX,TFT_LIGHTGREY);      //draw circular outline of PSZ
  tft.fillCircle(PSZx,PSZy,penradius,currentcolor);     //fill PSZ with pen colour
 
//setup up and down arrow zone boundaries to detect manual pen width selection. Boundaries 
//are relative to the size and coordinates of the arrows.
  upt=0;                //up arrow top bound
  upb=BOXSIZE-22;       //up arrow bottom bound
  upl=(BOXSIZE*10)-10;  //up arrow left bounds
  upr=(BOXSIZE*10)+10;  //up arrow right bounds

  dnt=BOXSIZE-18;       //down arrow top bound
  dnb=BOXSIZE;          //down arrow bottom bound
  dnl=(BOXSIZE*10)-10;  //down arrow left bound
  dnr=(BOXSIZE*10)+10;  //down arrow right bound

//draw up/down arrows according to the boundaries specified.
  tft.fillTriangle(BOXSIZE*10,0,upl,upb,upr,upb,TFT_WHITE);        //up arrow
  tft.fillTriangle(BOXSIZE*10,BOXSIZE,dnl,dnt,dnr,dnt,TFT_WHITE);  //down arrow
  
  buttonCLR.initButton(&tft,(BOXSIZE*11)+10,20,BUTTON_W,BUTTON_H,TFT_WHITE,TFT_DARKGREY,TFT_WHITE,"CLR",BUTTON_TEXTSIZE);
  buttonCLR.drawButton();
}

//draw icon.
void drawIcon(TFT_eSPI *gfx,int icon,int x,int y,int penradius,uint16_t currentcolor) {
  int xoffset=3;
  int yoffset=4;
  int featureColor=TFT_BLACK;
  if (currentcolor == TFT_BLACK)
    featureColor=TFT_WHITE;
  switch (icon) {
    case iSym:      //i-symbol
      tft.fillRect(x-5, y-3, 10, 23, currentcolor);
      tft.fillCircle(x, y-15, 5, currentcolor);
      break;
    case Smile:     //smiley face
      gfx->fillCircle(x,y,penradius,currentcolor);    //fill the face
      gfx->fillCircle(x-(penradius/xoffset),y-(penradius/yoffset),3,featureColor);  //left eye
      gfx->fillCircle(x+(penradius/xoffset),y-(penradius/yoffset),3,featureColor);  //right eye
      for (int roffset=7; roffset < 10; roffset++) {     //draw several times for thicker line
        for (double c = -1.15 ; c < 1.2 ; c += 0.05) {   //arc at bottom of circle
          gfx->drawPixel(sin(c)*(penradius-roffset)+x,cos(c)*(penradius-roffset)+y,featureColor);
        }
      }
      break;
    case Sad:       //sad face
      gfx->fillCircle(x,y,penradius,currentcolor);    //fill the face
      gfx->fillCircle(x-(penradius/xoffset),y-(penradius/yoffset),3,featureColor);  //left eye
      gfx->fillCircle(x+(penradius/xoffset),y-(penradius/yoffset),3,featureColor);  //right eye
      for (int roffset=7; roffset < 10; roffset++) {   //draw several times for thicker line
        for (double c = 2.25; c < 4.0; c += 0.05) {    //arc at top of circle shifted downwards
          gfx->drawPixel(sin(c)*(penradius-roffset)+x,cos(c)*(penradius-roffset)+y+20,featureColor);
        }
      }
      break;
    case Alert:
     tft.setSwapBytes(true);
     tft.pushImage(x-(alertWidth/2), y-(alertHeight/2), alertWidth, alertHeight, alert);
     tft.setSwapBytes(false);
     break;
    case Close:
     tft.setSwapBytes(true);
     tft.pushImage(x-(closeWidth/2), y-(closeHeight/2), closeWidth, closeHeight, closeX);
     tft.setSwapBytes(false);
     break;
    case Info:
     tft.setSwapBytes(true);
     tft.pushImage(x-(infoWidth/2), y-(infoHeight/2), infoWidth, infoHeight, info);
     tft.setSwapBytes(false);
     break;
    case Ferrari:
     tft.setSwapBytes(true);
     tft.pushImage(x-(ferrariWidth/2), y-(ferrariHeight/2), ferrariWidth, ferrariHeight, ferrari);
     tft.setSwapBytes(false);
     break;
    case Star:       //5-point star.
      gfx->fillTriangle(x,y-18,x-4,y-8,x+4,y-8,currentcolor);     //top 
      gfx->fillTriangle(x-18,y-8,x-4,y-8,x-8,y+1,currentcolor);   //left
      gfx->fillTriangle(x+18,y-8,x+4,y-8,x+8,y+1,currentcolor);   //right
      gfx->fillTriangle(x-13,y+15,x-8,y+2,x,y+8,currentcolor);    //bottom left
      gfx->fillTriangle(x+13,y+15,x+8,y+2,x,y+8,currentcolor);    //bottom right
      gfx->fillCircle(x,y,7,currentcolor);
      break;
  }
}

void loop(void) {
  uint16_t x, y, z;

  if (tft.getTouch(&x, &y))
    delay(20);      //debounce touch press
  if (tft.getTouch(&x, &y)) {
    //Serial.print("touch x y: "); Serial.print(x); Serial.print(" "); Serial.println(y);
 
    //handle CLR button press
    if (buttonCLR.contains(x,y)) {     
      buttonCLR.press(true);
    }
    if (buttonCLR.justPressed()) {
      buttonCLR.drawButton(true);    //invert button colours
      tft.fillRect(0, BOXSIZE+1, tft.width(), tft.height(), TFT_BLACK);   //clear out drawing zone
      buttonCLR.press(false);
      if (buttonCLR.justReleased())
        buttonCLR.drawButton();      //normal colours
    }

    //handle up arrow press
    if ((x > upl && x < upr) && (y > upt && y < upb)) {
      if (penradius >= MAX_PENRADIUS)
        penradius=MAX_PENRADIUS;
      else
        penradius++;
      tft.fillCircle(PSZx,PSZy,penradius,currentcolor);    //refresh pen size
      tft.drawCircle(PSZx,PSZy,HALFBOX,TFT_WHITE);
      delay(200);
    }
  
    //handle down arrow press
    if ((x > dnl && x < dnr) && (y > dnt && y < dnb)) {
      if (penradius <= MIN_PENRADIUS)
        penradius=MIN_PENRADIUS;
      else
        penradius--;
      tft.fillCircle(PSZx,PSZy,HALFBOX,TFT_BLACK);
      tft.fillCircle(PSZx,PSZy,penradius,currentcolor);   //refresh pen size
      tft.drawCircle(PSZx,PSZy,HALFBOX,TFT_WHITE);
      delay(200);
    }

    //handle icon press
    if (y < BOXSIZE && x > BOXSIZE*7 && x < BOXSIZE*8) {        //if touch within icon bounds
      iconMode=true;        //touched i symbol
      penradius=ICON_RADIUS;
      icon++;               //scroll thru icons
      if (icon > Star) icon=Smile;    //wraparound last icon
      tft.fillRect(BOXSIZE*7, 0, BOXSIZE+11, BOXSIZE+1, TFT_BLACK);    //clear icon. ICON_X starts at 10px to right of midpoint. Add 1 for extra. 
      //tft.fillCircle(ICON_X,ICON_Y,ICON_RADIUS,TFT_BLACK);       //clear icon
      drawIcon(&tft,icon,ICON_X, ICON_Y, penradius, currentcolor);   //draw icon
      tft.fillCircle(PSZx,PSZy,penradius,currentcolor);        //refresh pen size
      //Serial.print("icon "); Serial.println(icon);
    }

    //handle colour box press
    if (y < BOXSIZE && x < BOXSIZE*7) {        //if touch within colour box bounds
       oldcolor = currentcolor;
       if (x < BOXSIZE) {                                    //xcoord shows which box was pressed
         currentcolor = TFT_RED;                             //new colour selected
         tft.drawRect(0, 0, BOXSIZE, BOXSIZE, TFT_WHITE);    //draw new outline around selected box
       } else if (x < BOXSIZE*2) {
         currentcolor = TFT_YELLOW;
         tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       } else if (x < BOXSIZE*3) {
         currentcolor = TFT_GREEN;
         tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       } else if (x < BOXSIZE*4) {
         currentcolor = TFT_CYAN;
         tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       } else if (x < BOXSIZE*5) {
         currentcolor = TFT_BLUE;
         tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       } else if (x < BOXSIZE*6) {
         currentcolor = TFT_MAGENTA;
         tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       } else if (x < BOXSIZE*7) {
         currentcolor = TFT_WHITE;
         tft.drawRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
       }                
       iconMode=false;       //color selection forces line drawing mode.
       icon=iSym;            //show i symbol, 
       tft.fillCircle(ICON_X,ICON_Y,ICON_RADIUS,TFT_BLACK);             //clear icon
       drawIcon(&tft,icon,ICON_X, ICON_Y, ICON_RADIUS, currentcolor);   //draw i

       if (oldcolor != currentcolor) {      //new colour selected?
        if (oldcolor == TFT_RED) 
          tft.fillRect(0, 0, BOXSIZE, BOXSIZE, TFT_RED);      //redraw old colour box to clear previous outline
        if (oldcolor == TFT_YELLOW) 
          tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, TFT_YELLOW);
        if (oldcolor == TFT_GREEN) 
          tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, TFT_GREEN);
        if (oldcolor == TFT_CYAN) 
          tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, TFT_CYAN);
        if (oldcolor == TFT_BLUE) 
          tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, TFT_BLUE);
        if (oldcolor == TFT_MAGENTA) 
          tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, TFT_MAGENTA);
        if (oldcolor == TFT_WHITE) 
          tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, TFT_WHITE);
        tft.fillCircle(PSZx,PSZy,HALFBOX,TFT_BLACK);        //clear PSZ
        tft.drawCircle(PSZx,PSZy,HALFBOX,TFT_WHITE);        //redraw PSZ circle
        tft.fillCircle(PSZx,PSZy,penradius,currentcolor);   //refresh PSZ color
      }
    }

    //draw a circle representing the point x,y returned by the tft sensor.
    //Check circle lies within the drawing area bounds.
    if (((y-penradius) > BOXSIZE) && ((y+penradius) < tft.height())) {
      if (iconMode)         //icon mode
        drawIcon(&tft,icon,x, y, ICON_RADIUS, currentcolor);
      else                  //line drawing mode
        tft.fillCircle(x, y, penradius, currentcolor);
    }
  }
}
