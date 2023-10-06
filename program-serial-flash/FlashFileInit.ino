/*
 * Erase a file in serial flash. Zero filesize field in EEPROM.
 */
#include <SerialFlash.h>
#include <SPI.h>
#include "Serial_printf.h"
#include <EEPROM.h>
#define VREF_EEPROM_ADDR (E2END - 3)      //E2END is an Arduino constant. Store 4 bytes at end of EEPROM.
uint16_t address = VREF_EEPROM_ADDR;

union MemStore {
  byte bytebuf[4];
  uint32_t longbuf;
} eepromdata;

const int FlashChipSelect = 10; // digital pin for flash chip CS pin
const char *filename={"tdata.csv"};
SerialFlashFile file;

/*
 * clear bytes in EEPROM.
*/
void clearEEPROM(uint16_t eaddr,int blen) {
  for (int i=0; i < blen; i++) {
    EEPROM.write(eaddr+i, 0);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for Serial monitor to open
  delay(100);

  Serial.println(F("*** Erase file in serial flash chip v1.0 ***"));

  if (!SerialFlash.begin(FlashChipSelect)) {
    Serial.println("Unable to access SPI Flash chip");
    return;
  }
  
  Serial_printf(Serial,"Erased file %s.\r\n",filename);
  file = SerialFlash.open(filename);
  file.erase();
  clearEEPROM(address,4);
/*
  SerialFlash.remove(filename);
  Serial_printf(Serial,"Removed file %s.\r\n",filename);

  Serial_printf(Serial,"Erase chip.\r\n");
  SerialFlash.eraseAll();
  while (SerialFlash.ready() == false) {}
  Serial_printf(Serial,"Erase done.\r\n");
  clearEEPROM(address,4);
*/
}

void loop() {
}
