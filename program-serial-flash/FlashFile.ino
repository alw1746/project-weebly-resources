/*
 * Read/write records to serial flash similar to file operations. A limitation of the SerialFlash
 * API is it does not allow for subsequent record appends to the file. To workaround this, the file
 * size is written to EEPROM before file close. On open for write, it is read from EEPROM and a seek
 * done to EOF before appends. An erasable file is created with a multiple of 64KB.
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
const int PAGESIZE=256;

const char *filename={"tdata.csv"};
SerialFlashFile file;
uint32_t filesize;
char buffer[256];
char line[]={"123456789 123456789 123456789 123456789 123456789 12345678\r\n"
             };
/*
 * read long value from EEPROM
*/
uint32_t readEEPROM(uint16_t eaddr) {
  for (int i=0; i < 4; i++) {
    eepromdata.bytebuf[i]=EEPROM.read(eaddr+i);
  }
  return eepromdata.longbuf;
}

/*
 * write long value into EEPROM
*/
void writeEEPROM(uint16_t eaddr,uint32_t longvar) {
  eepromdata.longbuf=longvar;
  for (int i=0; i < 4; i++) {
    EEPROM.write(eaddr+i, eepromdata.bytebuf[i]);
  }
}

/*
 * clear bytes in EEPROM.
*/
void clearEEPROM(uint16_t eaddr,int blen) {
  for (int i=0; i < blen; i++) {
    EEPROM.write(eaddr+i, 0);
  }
}

void fclose() { 
  writeEEPROM(address,file.position());
  file.close();
}

void fopen(const char *filename,char mode) {
  file = SerialFlash.open(filename);
  filesize=readEEPROM(address);
  if (mode=='w' || mode=='W') {
    file.seek(filesize);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for Serial monitor to open
  delay(100);

  Serial.println(F("*** Read/Write file to serial flash chip v1.0 ***"));

  if (!SerialFlash.begin(FlashChipSelect)) {
    Serial.println("Unable to access SPI Flash chip");
    return;
  }
  if (SerialFlash.exists(filename)) {
    Serial_printf(Serial,"open for read %s.\r\n",filename);
    fopen(filename,'r');
    Serial_printf(Serial,"file size: %l\r\n",filesize);
    Serial_printf(Serial,"block size: %l\r\n",SerialFlash.blockSize());

    for (int i=0; i < filesize/PAGESIZE; i++) {
      file.read(buffer, 256);
      for (int i=0; i < 256; i++) {
        Serial_printf(Serial,"%c",buffer[i]);
      }
    }
    int remainder=filesize%PAGESIZE;
    file.read(buffer, remainder);
    for (int i=0; i < remainder; i++) {
      Serial_printf(Serial,"%c",buffer[i]);
    }
    Serial_printf(Serial,"file closed.\r\n");
    file.close();

  }
  else {
    Serial_printf(Serial,"create erasable file %s.\r\n",filename);
    SerialFlash.createErasable(filename, 8*1024);
  }

  Serial_printf(Serial,"open for write %s.\r\n",filename);
  fopen(filename,'w');
  Serial_printf(Serial,"allocated size: %l\r\n",file.size());
  Serial_printf(Serial,"block size: %l\r\n",SerialFlash.blockSize());

  for (int i=0; i < 5; i++) {
    sprintf(line,"%04d",i+1);
    line[4]=' ';
    //Serial_printf(Serial,"%s",line);
    file.write(line,strlen(line));
  }
  Serial_printf(Serial,"5 records written, file closed.\r\n");
  Serial_printf(Serial,"file size: %l\r\n",file.position());
  fclose();
}

void loop() {

}
