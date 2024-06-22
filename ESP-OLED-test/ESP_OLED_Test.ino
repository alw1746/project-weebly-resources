/* Draw grid lines and text on a 0.96" OLED to verify if you have a 128x32 or 128x64 device.
 * Use I2C scanner to obtain the OLED device address.
 * https://learn.adafruit.com/scanning-i2c-addresses/arduino
 * Take note of comment at SCREEN_ADDRESS.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//NB: this is not strictly true, 0x3C is used for 128x64 displays too. Test OLED to verify.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Test function to check if OLED display is 128x64 or 128x32 pixels.
//display grid lines with row/col markings. Change SCREEN_HEIGHT to test display.
//128x64 = 21 cols, 8 rows of text
//128x32 = 21 cols, 4 rows of text
void gridOLED128xNN() {
  int row=display.height();   //height/width returns pixel count
  int col=display.width();

  display.drawFastHLine(0,0,col,WHITE);     //x,y,length(pixel count)
  display.drawFastHLine(0,row*0.5,col,WHITE);
  display.drawFastHLine(0,row-1,col,WHITE);   //row is pixel count so -1 for coord

  display.drawFastVLine(0,0,row,WHITE);     //x,y,length(pixel count)
  display.drawFastVLine(col*0.25,0,row,WHITE);
  display.drawFastVLine(col*0.50,0,row,WHITE);
  display.drawFastVLine(col*0.75,0,row,WHITE);
  display.drawFastVLine(col-1,0,row,WHITE);   //col is pixel count so -1 for coord

  display.setCursor(0,0);
  display.println("123456789 123456789 1");
  for (int y=2; y < 32; y++) {                //line count
    display.print(" ");
    display.println(y);
  }
  display.setCursor(col*0.2,row*0.4);
  display.setTextSize(2);
  display.print("aAzZ0-9");                 //font size test
  display.display();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP_OLED_Test 1.0");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  delay(2000);
  display.clearDisplay();
  display.setRotation(0);     //0=(landscape,pins top),1=(portrait,pins left),2=(land,pins bot)3=(port,pins right)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  gridOLED128xNN();
}

void loop(void) {
}
