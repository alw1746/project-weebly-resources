/* FM receiver using TEA5767 radio chip controlled by ATTINY85.
 * Use ATTinyCore library with USBtinyISP programmer.
 * Board: ATtiny25/45/85(No bootloader)
 * Chip: ATTiny85
 * Clock source: 16MHz(PLL)
 * Programmer: USBtinyISP FAST
 * If using USBtinyISP to program chip, always burn bootloader the 1st time to set fuses for clock, BOD, etc. Subsequently just upload sketch.
 * Builtin Serial port uses PB0(TX), PB1(RX). This conflicts with I2C SDA so cannot use Serial. If necessary, use SoftwareSerial.
 * LED_BUILTIN uses PB1 so conflicts I2C SCL and Serial as well.
 * 

Arduino port | TEA5767 signal
------------ | ---------------
          5V | VCC
         PB0 | SDA
         PB2 | SCL
         GND | GND

  +--- antenna stereo_out ---+
  |                          |
  |     TEA5767 PCB top      |
  |                          |
  +----- 5V SDA SCL GND -----+
*/

#include <Wire.h>
#include <radio.h> 
#include <TEA5767.h>

/// The band that will be tuned by this sketch is FM world band 78-108Mhz
#define FIX_BAND RADIO_BAND_FMWORLD

/// The station that will be tuned by this sketch.
#define FIX_STATION 8780

TEA5767 radio;    // Create an instance of Class for radio Chip

void setup() {
  // NB: Serial port will conflict with I2C.
  //Serial.begin(19200);
  //Serial.println("Radio...");
  //delay(200);

  // Initialize the Radio 
  radio.init();

  // HERE: adjust the frequency to a local sender
  radio.setBandFrequency(FIX_BAND, FIX_STATION); //
  radio.setVolume(2);
  radio.setMono(true);
}

void loop() {
}
