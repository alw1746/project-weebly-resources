/*
 Display JPG and BMP images on a 0.96" 128x32 OLED, 3.5" 480x320 TFT LCD and 1.3" 240x240 IPS LCD for comparison.

 Example for library:
 https://github.com/Bodmer/TJpg_Decoder

 This example renders a Jpeg file that is stored in an array within Flash (program) memory
 see image_jpg.h

To convert JPG file to an array,
http://tomeko.net/online_tools/file_to_hex.php?lang=en

To generate bmp image array for OLED displays
https://javl.github.io/image2cpp/

 uncomment line in S:\SOE\arduino-1.8.13\portable\sketchbook\libraries\TFT_eSPI\User_Setup_Select.h to select the LCD type.

*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Include the image array
#include "image_jpg.h"
#include "image_bmp.h"

// Include the jpeg decoder library
#include <TJpg_Decoder.h>

// Include the TFT library https://github.com/Bodmer/TFT_eSPI
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

void setup()
{

  Serial.begin(115200);
  Serial.println("TJpg_Decoder library");

  // Initialise the TFT
  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(TFT_BLACK);
  if (tft.width() == 240 && tft.height() == 240) {
    tft.setRotation(0);       //0 for 240x240, 1 for 480x320 (landscape)
  }
  else if (tft.width() == 320 && tft.height() == 480) {
    tft.setRotation(1);       //0 for 240x240, 1 for 480x320 (landscape)
  }
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setSwapBytes(true);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);

  // Get the width and height in pixels of the jpeg if you wish
  uint16_t w = 0, h = 0;
  TJpgDec.getJpgSize(&w, &h, image_jpg, sizeof(image_jpg));

  // Draw the image, top left at 0,0
  TJpgDec.drawJpg(0, 0, image_jpg, sizeof(image_jpg));

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    display.clearDisplay();
    display.drawBitmap(0,0,image_bmp,128,64, 1);
    display.display();  
  }
}

void loop()
{
}
