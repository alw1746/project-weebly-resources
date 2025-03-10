/*
 * 3-way wireless speaker controller. Switch tweeter, midrange, woofer speakers on or off by
 * clicking on 3 buttons on a web page. Relays on the speaker cables make/break the
 * connection. Requires a ESP8266 or ESP32 module and a 4-ch relay board.
 * SSID WhrfSpkr,<mypasswd>
 * mDNS WhrfSpkr.local
 * Ipaddr 192.168.5.10
 * After connecting to SSID, browse to http://WhrfSpkr.local
 * Wire 4-ch relay module connectors as below for NC(Normal Close ie poweroff=connect).
 * GPIO HIGH=connect(led off), LOW=disconnect(led on)
 * 
 *     +-------+
 *     | Relay |
 *     | top   |
 *     | view  |
 *     +-------+
 *    [NO][X][NC]
 *         |  |__Load (speaker out)
 *         |_____Common(speaker in)
 */

#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPmDNS.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

//GPIO pins for controlling relays. D5=14(ESP8266), D6=12, D7=13
#define WOOF 13
#define MIDR 12
#define TWEE 14

struct Config {
  char mode[5];
  char ssid[20];
  char password[20];
  char mdnsName[20];
  int port;
  char ipaddress[20];
  char outputFile[20];
};
Config config;                           //global configuration object

char ipaddress[20];
AsyncWebServer *server;
uint8_t WOOFstatus,MIDRstatus,TWEEstatus;
IPAddress local_ip;

//main web page is autorefreshed every 4 mins to defeat softAP inactivity disconnect timer(5mins).
String SendHTML(uint8_t twee, uint8_t midr,uint8_t woof){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Speaker Connection Switch</title>\n";
  ptr +="<meta http-equiv=\"refresh\" content=\"240\">\n";
  ptr +="<style>html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 120px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 8px;}\n";
  ptr +=".button-on {background-color: #FF0000;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #00FF00;}\n";
  ptr +=".button-off:active {background-color: #2980b9;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>3-way Speaker Controller</h1>\n";

  if(twee)
    {ptr +="<a class=\"button button-off\" title=\"Click to mute\" href=\"/TWEEoff\">TWEETER</a>\n";}
  else
    {ptr +="<a class=\"button button-on\" title=\"Click to unmute\" href=\"/TWEEon\">TWEETER</a>\n";}
  ptr +="<br>\n";
  
  if(midr)
    {ptr +="<a class=\"button button-off\" title=\"Click to mute\" href=\"/MIDRoff\">MIDRANGE</a>\n";}
  else
    {ptr +="<a class=\"button button-on\" title=\"Click to unmute\" href=\"/MIDRon\">MIDRANGE</a>\n";}
  ptr +="<br>\n";

  if(woof)
    {ptr +="<a class=\"button button-off\" title=\"Click to mute\" href=\"/WOOFoff\">WOOFER</a>\n";}
  else
    {ptr +="<a class=\"button button-on\" title=\"Click to unmute\" href=\"/WOOFon\">WOOFER</a>\n";}
  ptr +="<br>\n";

  //ptr +="Click to mute-unmute speakers.<br>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void setup() {
  Serial.begin(115200);
  pinMode(WOOF, OUTPUT);
  pinMode(MIDR, OUTPUT);
  pinMode(TWEE, OUTPUT);
  WOOFstatus = 1;                  //connect on powerup
  MIDRstatus = 1;
  TWEEstatus = 1;
  digitalWrite(WOOF,WOOFstatus);   //HIGH=connected(wired to NC),led is off
  digitalWrite(MIDR,MIDRstatus);
  digitalWrite(TWEE,TWEEstatus);

  strncpy(config.mode,"AP",sizeof(config.mode));        //or STA 
  strncpy(config.ssid,"WhrfSpkr",sizeof(config.ssid));
  strncpy(config.password,"<mypasswd>",sizeof(config.password));    //min pwd length 8.
  strncpy(config.mdnsName,"WhrfSpkr",sizeof(config.mdnsName));
  config.port = 80;
  strncpy(config.ipaddress,"192.168.5.10",sizeof(config.ipaddress));    //fixed ip address for AP
  strncpy(config.outputFile,"",sizeof(config.outputFile));

  if (strncmp(config.mode,"AP",2) == 0) {    //Access Point mode config
    Serial.printf("Setting up AP with ssid %s\r\n",config.ssid);
    WiFi.mode(WIFI_AP);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);      //disable inactivity light sleep
    local_ip.fromString(config.ipaddress);
    IPAddress gateway=local_ip;
    IPAddress subnet(255,255,255,0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(config.ssid,config.password);
    delay(200);   //required!
    local_ip=WiFi.softAPIP();
    Serial.print("IP address: ");
    Serial.println(local_ip);
  }
  else if (strncmp(config.mode,"STA",3) == 0) {    //Station mode config
    Serial.printf("Connecting to ssid %s\r\n",config.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(config.ssid, config.password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
    WiFi.localIP().toString().toCharArray(ipaddress,sizeof(ipaddress));
    Serial.printf("IP address: %s\r\n",ipaddress);
  }
  else {
     Serial.printf("Invalid wifi mode found in config: %s\r\n",config.mode);
  }

  if (!MDNS.begin(config.mdnsName)) {         //NB: append .local in browser
        Serial.printf("Error setting up MDNS responder %s\r\n",config.mdnsName);
  }
  else {
    Serial.printf("mDNS responder %s started\r\n",config.mdnsName);  //Windows cmd: dns-sd -B or -Z
  }

  server = new AsyncWebServer(config.port);   //create web server
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){    //respond to home page request
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus));
  });

  server->on("/TWEEon", HTTP_GET, [](AsyncWebServerRequest *request){    //handle tweeter
    TWEEstatus=1;
    digitalWrite(TWEE, TWEEstatus);       //turn relay on
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });

  server->on("/TWEEoff", HTTP_GET, [](AsyncWebServerRequest *request){
    TWEEstatus = 0;
    digitalWrite(TWEE, TWEEstatus);       //turn relay off
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });

  server->on("/MIDRon", HTTP_GET, [](AsyncWebServerRequest *request){    //handle midrange
    MIDRstatus=1;
    digitalWrite(MIDR, MIDRstatus);       //turn relay on
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });

  server->on("/MIDRoff", HTTP_GET, [](AsyncWebServerRequest *request){
    MIDRstatus = 0;
    digitalWrite(MIDR, MIDRstatus);       //turn relay off
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });

  server->on("/WOOFon", HTTP_GET, [](AsyncWebServerRequest *request){    //handle woofer
    WOOFstatus=1;
    digitalWrite(WOOF, HIGH);       //turn relay on
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });

  server->on("/WOOFoff", HTTP_GET, [](AsyncWebServerRequest *request){
    WOOFstatus = 0;
    digitalWrite(WOOF, LOW);       //turn relay off
    request->send(200, "text/html", SendHTML(TWEEstatus,MIDRstatus,WOOFstatus)); 
  });


  server->onNotFound([](AsyncWebServerRequest *request) {           //respond to invalid request
    request->send(404, "text/plain", "Not found");
  });

  server->begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", config.port);    //Add service to MDNS-SD
  Serial.printf("After ssid connect, browse to ip address or http://%s.local for home page.\r\n",config.mdnsName); 
}

void loop() {
  #if defined(ESP8266)
    MDNS.update();
  #endif
}
