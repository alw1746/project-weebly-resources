// A fun MATRIX-like screen demo of scrolling
// Screen will flicker initially until fully drawn
// then scroll smoothly
// Needs GLCD font.
// AW 11/11/21 - Customised to fit 320x480 screen.

/*
 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
*/

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define ILI9341_VSCRDEF 0x33
#define ILI9341_VSCRSADD 0x37
#define TEXT_SIZE_MULTIPLIER 2   //font size multiplier
#define TEXT_HEIGHT 8* TEXT_SIZE_MULTIPLIER// Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0  // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 0  // Number of lines in top fixed area (lines counted from top of screen)

uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = 480 - TOP_FIXED_AREA - BOT_FIXED_AREA;
uint16_t yDraw = 480 - BOT_FIXED_AREA - TEXT_HEIGHT;
byte     pos[42];
uint16_t xPos = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(TEXT_SIZE_MULTIPLIER);
  tft.fillScreen(TFT_BLACK);
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
}

void loop(void) {
  // First fill the screen with random streaks of characters
  for (int j = 0; j < 600; j += TEXT_HEIGHT) {
    for (int i = 0; i < 40; i++) {
      if (pos[i] > 20) pos[i] -= 3; // Rapid fade initially brightness values
      if (pos[i] > 0) pos[i] -= 1; // Slow fade later
      if ((random(20) == 1) && (j<400)) pos[i] = 63; // ~1 in 20 probability of a new character
      tft.setTextColor(pos[i] << 5, TFT_BLACK); // Set the green character brightness
      if (pos[i] == 63) tft.setTextColor(TFT_WHITE, TFT_BLACK); // Draw white character
      xPos += tft.drawChar(random(32, 128), xPos, yDraw, 1); // Draw the character
    }
    yDraw = scroll_slow(TEXT_HEIGHT, 14); // Scroll, 14ms per pixel line
    xPos = 0;
  }

  //tft.setRotation(2);
  //tft.setTextColor(63 << 5, TFT_BLACK);
  //tft.drawCentreString("MATRIX",120,60,4);
  //tft.setRotation(0);

  // Now scroll smoothly forever
  while (1) {yield(); yDraw = scroll_slow(480,5); }// Scroll 480 lines, 5ms per line

}

void setupScrollArea(uint16_t TFA, uint16_t BFA) {
  tft.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
  tft.writedata(TFA >> 8);
  tft.writedata(TFA);
  tft.writedata((480 - TFA - BFA) >> 8);
  tft.writedata(480 - TFA - BFA);
  tft.writedata(BFA >> 8);
  tft.writedata(BFA);
}

int scroll_slow(int lines, int wait) {
  int yTemp = yStart;
  for (int i = 0; i < lines; i++) {
    yStart++;
    if (yStart == 480 - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA;
    scrollAddress(yStart);
    delay(wait);
  }
  return  yTemp;
}

void scrollAddress(uint16_t VSP) {
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling start address
  tft.writedata(VSP >> 8);
  tft.writedata(VSP);
}
