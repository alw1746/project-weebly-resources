/*

Examples of read/write/update config data in serial flash chip.

-init struct with test data
-write struct to flash
-init struct with 0
-read struct from flash
-update ipaddress in struct
-write struct to flash
-init struct with 0
-read struct from flash

*/

#include<SPIMemory.h>

struct Config {
  char mode[5];
  char ssid[17];
  char password[17];
  int port;
  char ipaddress[17];
  char outputFile[17];
};

SPIFlash spiflash;
unsigned long pageAddr = 256;      //use 2nd page of 1st sector for storage
Config config;                     //global configuration object

// read the configuration from flash.
void readConfiguration(SPIFlash &flash, Config &config) {
  if (!flash.readAnything(pageAddr, config)) {
    Serial.println(F("Flash readConfiguration failed."));
  }
}

// write the configuration to flash.
void writeConfiguration(SPIFlash &flash, const Config &config) {
  while (!flash.eraseSector(pageAddr));     //must erase sector(4KB) first
  if (!flash.writeAnything(pageAddr, config, true)) {
    Serial.println(F("Flash writeConfiguration failed."));
  }
}

void initConfiguration(Config &config) {
  strlcpy(config.mode,"AP",sizeof(config.mode));
  strlcpy(config.ssid,"Wifi_SSID",sizeof(config.ssid));
  strlcpy(config.password,"Wifi_Pwd",sizeof(config.password));
  config.port = 80;
  strlcpy(config.ipaddress,"192.168.1.20",sizeof(config.ipaddress));
  strlcpy(config.outputFile,"/data.csv",sizeof(config.outputFile));
}

void printConfiguration(const Config &config) {
  Serial.print(" mode: ");
  Serial.println(config.mode);
  Serial.print(" ssid: ");
  Serial.println(config.ssid);
  Serial.print(" password: ");
  Serial.println(config.password);
  Serial.print(" port: ");
  Serial.println(config.port);
  Serial.print(" ipaddress: ");
  Serial.println(config.ipaddress);
  Serial.print(" outputfile: ");
  Serial.println(config.outputFile);
  Serial.println("");
}

void setup() {
  // Initialize serial port
  Serial.begin(115200);
  while (!Serial) ; // Wait for Serial monitor to open
  delay(100);

  Serial.println(F("*** Read/Write struct to serial flash chip v1.0 ***"));
  spiflash.begin();
  spiflash.setClock(8000000);          //set to 8MHz if garbage data returned.
  Serial.print(F("Flash size: "));
  Serial.print((long)(spiflash.getCapacity()/1000));
  Serial.println(F("Kb"));
  Serial.println("");

  initConfiguration(config);      //fill struct with test data
  Serial.println(F("Config after init:"));
  printConfiguration(config);
  
  Serial.println(F("Writing config to flash..."));
  writeConfiguration(spiflash, config);   //write struct to flash
  Serial.println("");
  
  memset(&config,0,sizeof(config));      //init struct with 0
  Serial.println(F("Config after memset to 0:"));
  printConfiguration(config);
  
  readConfiguration(spiflash, config);    //read struct from flash
  Serial.println(F("Config after reading from flash:"));
  printConfiguration(config);

  strlcpy(config.ipaddress,"192.168.5.140",sizeof(config.ipaddress));    //update ipaddress in struct
  Serial.print("New ipaddress: ");
  Serial.println(config.ipaddress);
  Serial.println(F("Updating config in flash..."));
  writeConfiguration(spiflash, config);   //write struct to flash
  Serial.println("");

  memset(&config,0,sizeof(config));      //init struct with 0
  readConfiguration(spiflash, config);    //read struct from flash
  Serial.println(F("Config after update:"));
  printConfiguration(config);

}

void loop() {
}
