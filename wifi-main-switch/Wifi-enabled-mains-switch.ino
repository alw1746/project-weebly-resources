/*
Wifi-enabled Mains Switch. ESP32 hosts a web page with clickable button to switch AC power on/off via a thyristor. ESP32 wifi uses
AP mode to act as an access point with ssid REM_MAINSW so that clients such as laptops, smartphones can connect to it directly. For clients that
support mDNS, you can browse to rem_mainsw.local otherwise the ip address is 192.168.5.10. A pushbutton switch provides direct remote control.
 */
#include <OneButton.h>
#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <ESPmDNS.h>
  #define BUTTON1_PIN 12          //pushbutton pin
  #define LED1 LED_BUILTIN        //status LED
  #define SSRPIN 23               //solid state relay pin
#endif

OneButton button1(BUTTON1_PIN, true,true);    //pin,activeLow=true,internal pullup=true. Connect button to ground.
struct Config {
  char mode[5];
  char ssid[17];
  char password[17];
  char mdnsName[17];
  int port;
  char ipaddress[17];
  char outputFile[17];
};
Config config;                           //global configuration object

char ipaddress[17];
AsyncWebServer *server;
bool LED1status;

void display_freeram()
{
  #if defined(ARDUINO_ARCH_ESP32)
    Serial.print(F("Free Heap="));
    Serial.println(ESP.getFreeHeap());
  #elif defined(ARDUINO_ARCH_AVR)
    Serial.print(F("Free SRAM="));
    extern int __heap_start,*__brkval;
    int v,freeRam;
    freeRam = (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);  
    Serial.println(freeRam);
  #endif
}

String SendHTML(uint8_t led1stat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Wifi-enabled Mains Switch</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 100px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #FF0000;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #00FF00;}\n";
  ptr +=".button-off:active {background-color: #2980b9;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Wifi-enabled Mains Switch</h1>\n";
  //ptr +="<h3>Wireless Load Switch</h3>\n";
  
   if(led1stat)
  {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">SWITCH ON</a>\n";}
  else
  {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">SWITCH OFF</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

//handle pushbutton click to switch thyristor on/off.
void click1() {
  if (LED1status) {    //switch on?
    LED1status = LOW;
    digitalWrite(SSRPIN,LOW);    //turn thyristor off
  }
  else {     //swicth off
    LED1status = HIGH;
    digitalWrite(SSRPIN,HIGH);   //turn thyristor on
  }
  digitalWrite(LED1, LED1status);   //enable/disable LED
  Serial.printf("LED1 Status: %s\r\n",LED1status?"ON":"OFF");
}

void setup() {
  Serial.begin(115200);
  display_freeram();
  pinMode(SSRPIN,OUTPUT);
  digitalWrite(SSRPIN,LOW);
  pinMode(LED1,OUTPUT);
  LED1status = LOW;
  digitalWrite(LED1, LED1status);  //LED on
  button1.setDebounceTicks(50);
  button1.setClickTicks(150);
  button1.attachClick(click1);

  strlcpy(config.mode,"AP",sizeof(config.mode));        //or STA 
  strlcpy(config.ssid,"REM_MAINSW",sizeof(config.ssid));
  strlcpy(config.password,"rem_mainsw",sizeof(config.password));
  strlcpy(config.mdnsName,"rem_mainsw",sizeof(config.mdnsName));
  config.port = 80;
  strlcpy(config.ipaddress,"192.168.5.10",sizeof(config.ipaddress));    //fixed ip address for AP
  strlcpy(config.outputFile,"",sizeof(config.outputFile));

  if (strncmp(config.mode,"AP",2) == 0) {    //Access Point mode config
    Serial.printf("Setting up AP %s\r\n",config.ssid);
    WiFi.mode(WIFI_AP); 
    WiFi.softAP(config.ssid, config.password);
    delay(200);   //required!
    IPAddress local_ip;
    local_ip.fromString(config.ipaddress);
    IPAddress gateway=local_ip;
    IPAddress subnet(255,255,255,0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAPIP().toString().toCharArray(ipaddress,sizeof(ipaddress));
    Serial.printf("IP address: %s\r\n",ipaddress);
  }
  else if (strncmp(config.mode,"STA",3) == 0) {    //Station mode config
    Serial.printf("Connecting to %s\r\n",config.ssid);
    WiFi.mode(WIFI_STA);
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

  if (!MDNS.begin(config.mdnsName)) {         //NB: append .local when query
        Serial.printf("Error setting up MDNS responder %s\r\n",config.mdnsName);
  }
  else {
    Serial.printf("mDNS responder %s started\r\n",config.mdnsName);  //Windows: dns-sd -B or -Z
  }

  server = new AsyncWebServer(config.port);   //create web server

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){    //respond to home page request
    LED1status = digitalRead(LED1);
    Serial.printf("LED1 Status: %s\r\n",LED1status?"ON":"OFF");
    request->send(200, "text/html", SendHTML(LED1status));
  });

  server->on("/led1on", HTTP_GET, [](AsyncWebServerRequest *request){    //respond to switch on request
    LED1status = HIGH;
    Serial.println("LED1 Status: ON");
    digitalWrite(LED1, LED1status);  //LED on
    digitalWrite(SSRPIN,HIGH);
    request->send(200, "text/html", SendHTML(true)); 
  });

  server->on("/led1off", HTTP_GET, [](AsyncWebServerRequest *request){    //respond to switch off request
    LED1status = LOW;
    Serial.println("LED1 Status: OFF");
    digitalWrite(LED1, LED1status);  //LED off
    digitalWrite(SSRPIN,LOW);
    request->send(200, "text/html", SendHTML(false)); 
  });

  server->onNotFound([](AsyncWebServerRequest *request) {           //respond to invalid request
    request->send(404, "text/plain", "Not found");
  });

  server->begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", config.port);    //Add service to MDNS-SD
  Serial.printf("Switch status: %d\r\n",LED1status);
  Serial.printf("Browse to http://%s or http://%s.local for the EMA IPC home page.\r\n",ipaddress,config.mdnsName); 
}

void loop() {
  button1.tick();    //check pushbutton state
}
