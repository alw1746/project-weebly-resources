/* Draw grid lines and text on a 0.96" 128x32 OLED, 3.5" 480x320 TFT LCD and 1.3" 240x240 IPS LCD for comparison.

 uncomment line in S:\SOE\arduino-1.8.13\portable\sketchbook\libraries\TFT_eSPI\User_Setup_Select.h to select the LCD type.
*/
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

TFT_eSPI tft = TFT_eSPI();        //display width,height

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void gridOLED128x32() {
  int row=display.height();
  int col=display.width();

  display.drawFastHLine(0,0,col,WHITE);
  display.drawFastHLine(0,15,col,WHITE);
  display.drawFastHLine(0,31,col,WHITE);

  display.drawFastVLine(0,0,row,WHITE);
  display.drawFastVLine(31,0,row,WHITE);
  display.drawFastVLine(63,0,row,WHITE);
  display.drawFastVLine(95,0,row,WHITE);
  display.drawFastVLine(127,0,row,WHITE);

  display.setCursor(0,0);
  display.println("123456789 123456789 1");
  display.println(" 2");
  display.println(" 3");
  display.println(" 4");

  display.setCursor(30,12);
  display.setTextSize(2);
  display.print("aAzZ0-9");
  display.display();
  
}

void grid240x240() {
  int row=tft.height();
  int col=tft.width();

  tft.drawFastHLine(0,0,col,TFT_WHITE);
  tft.drawFastHLine(0,79,col,TFT_LIGHTGREY);
  tft.drawFastHLine(0,159,col,TFT_LIGHTGREY);
  tft.drawFastHLine(0,239,col,TFT_WHITE);

  tft.drawFastVLine(0,0,row,TFT_WHITE);
  tft.drawFastVLine(79,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(159,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(239,0,row,TFT_WHITE);

  tft.setCursor(0,0);
  tft.println("123456789 1234567");
  tft.println("2");
  tft.println("3");
  tft.println("4");
  tft.println("5");
  tft.println("6");
  tft.println("7");
  tft.println("8");
  tft.println("9");

  tft.setCursor(50,30);
  tft.print(col);
  tft.print("Wx");
  tft.print(row);
  tft.print("H");
  tft.setCursor(50,70);
  tft.print("Font2: ");
  tft.setTextFont(2);
  tft.print("aAzZ0-9");
  tft.setCursor(50,120);
  tft.setTextFont(4);
  tft.print("Font4: aAzZ0-9");
  tft.setCursor(50,170);
  tft.print("Font6: ");
  tft.setTextFont(6);
  tft.print("0-9");
}

void grid480x320() {
  int row=tft.height();
  int col=tft.width();

  tft.drawFastHLine(0,0,col,TFT_WHITE);
  tft.drawFastHLine(0,79,col,TFT_LIGHTGREY);
  tft.drawFastHLine(0,159,col,TFT_LIGHTGREY);
  tft.drawFastHLine(0,239,col,TFT_LIGHTGREY);
  tft.drawFastHLine(0,319,col,TFT_WHITE);

  tft.drawFastVLine(0,0,row,TFT_WHITE);
  tft.drawFastVLine(79,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(159,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(239,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(319,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(399,0,row,TFT_LIGHTGREY);
  tft.drawFastVLine(479,0,row,TFT_WHITE);

  tft.setCursor(0,0);
  tft.println("123456789 123456789 123456789 123456");
  tft.println("2");
  tft.println("3");
  tft.println("4");
  tft.println("5");
  tft.println("6");
  tft.println("7");
  tft.println("8");
  tft.println("9");
  tft.println("10");
  tft.println("11");
  tft.println("12");

  tft.setCursor(160,50);
  tft.print(col);
  tft.print("Wx");
  tft.print(row);
  tft.print("H");
  tft.setCursor(160,100);
  tft.print("Font2: ");
  tft.setTextFont(2);
  tft.print("aAzZ0-9");
  tft.setCursor(160,150);
  tft.setTextFont(4);
  tft.print("Font4: aAzZ0-9");
  tft.setCursor(160,200);
  tft.print("Font6: ");
  tft.setTextFont(6);
  tft.print("0-9");

}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.println("ESP_TFT_GRID 1.0");
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (tft.width() == 240 && tft.height() == 240) {
    tft.setRotation(0);       //0 for 240x240, 1 for 480x320 (landscape)
    grid240x240();
  }
  else if (tft.width() == 320 && tft.height() == 480) {
    tft.setRotation(1);       //0 for 240x240, 1 for 480x320 (landscape)
    grid480x320();
  }
  else {
    tft.setCursor(0,0);
    tft.print(tft.width());
    tft.print("Wx");
    tft.print(tft.height());
    tft.print("H");
  }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    gridOLED128x32();
  }
}

void loop(void) {
}
