
// Includes

#include <Time.h>
#include <TimeLib.h>
#include <Base64_lib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <EEPROM.h>
#include <SoftwareSerial.h>

#include <ArduinoJson.h>

#include <FS.h>
#include <Ticker.h>

Ticker ticker_state_charge;
Ticker ticker_state_dist;

Ticker ticker_ntp;
#include <WiFiUdp.h>

unsigned int localPort = 2391;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

String strLog = "";

String roombotVersion = "0.4.6";
String WMode = "1";

#define SERIAL_RX     D5  // pin for SoftwareSerial RX
#define SERIAL_TX     D6  // pin for SoftwareSerial TX
#define  GPIO_LED     16
#define  GPIO_BUTTON     13

SoftwareSerial mySerial(SERIAL_RX, SERIAL_TX); // (RX, TX. inverted, buffer)


// Div
File UploadFile;
String fileName;
String  BSlocal = "0";
int FSTotal;
int FSUsed;

//-------------- FSBrowser application -----------
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

// WIFI
String ssid    = "mlaiacker";
String password = "83kdfiafo274DF";
String espName    = "Roombot";

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

// AP mode when WIFI not available
//const char *APssid = "Roombot";
//const char *APpassword = "thereisnospoon";

String roomba_state1 = "?";
String roomba_state_volt = "?";
String roomba_state_current = "?";
String roomba_state_temp = "?";
String roomba_state_dist = "?";
int32_t state_dist_total=0;
long state_rssi;
int state_adc;

int serial_state = 0;

int state_led = 0;

int clean_auto_on = 1;
int32_t clean_dist = 0;
int clean_daily_mask = 0x1f; 
int clean_hour = 12;

// HTML
//String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>Roombot control panel</a></div><div><ul class='nav navbar-nav'><li><a href='.'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li><li><a href='/filemanager.html'><span class='glyphicon glyphicon-file'></span> File manager</a></li><li><a href='/fupload'> File upload</a></li><li><a href='/ntp'> NTP</a></li></ul></li><li><a href='https://github.com/incmve/roomba-eps8266/wiki' target='_blank'><span class='glyphicon glyphicon-question-sign'></span> Help</a></li></ul></div></div></nav>";
String containerStart   =  "<div class='container'><div class='row'>";
String containerEnd     =  "<div class='clearfix visible-lg'></div></div></div>";
String siteEnd        =  "</body></html>";

String panelHeaderName    =  "<div class='col-md-4'><div class='page-header'><h1>";
String panelHeaderEnd   =  "</h1></div>";
String panelEnd       =  "</div>";

String panelBodySymbol    =  "<div class='panel panel-default'><div class='panel-body'><span class='glyphicon glyphicon-";
String panelBodyName    =  "'></span> ";
String panelBodyValue   =  "<span class='pull-right'>";
String panelcenter   =  "<div class='row'><div class='span6' style='text-align:center'>";
String panelBodyEnd     =  "</span></div></div>";

String inputBodyStart   =  "<form action='' method='POST'><div class='panel panel-default'><div class='panel-body'>";
String inputBodyName    =  "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>";
String inputBodyPOST    =  "</span><input type='text' name='";
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div>";
String roombacontrol    =  "<a href='roombastart'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> Start</button></a><a href='roombadock'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-home' aria-hidden='true'></span> Dock</button></a> <a href='roombastop'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-stop' aria-hidden='true'></span> Stop</button></a></div></div>";
String linksCleanHour;//   =  "<a href='api?action=cleanHour&value=0'><button type='button' class='btn btn-default'>0</button></a> <a href='api?action=cleanHour&value=1'><button type='button' class='btn btn-default'>1</button></a> <a href='api?action=cleanHour&value=2'> <button type='button' class='btn btn-default'>2</button></a> <a href='api?action=cleanHour&value=3'><button type='button' class='btn btn-default'>3</button></a> </div></div>";
String linksCleanDays   =  "<a href='api?action=cleanDays&value=1'><button type='button' class='btn btn-default'>M</button></a> <a href='api?action=cleanDays&value=2'><button type='button' class='btn btn-default'>T</button></a> <a href='api?action=cleanDays&value=4'><button type='button' class='btn btn-default'>W</button></a> <a href='api?action=cleanDays&value=8'><button type='button' class='btn btn-default'>T</button></a> <a href='api?action=cleanDays&value=16'><button type='button' class='btn btn-default'>F</button></a> <a href='api?action=cleanDays&value=32'><button type='button' class='btn btn-default'>S</button></a> <a href='api?action=cleanDays&value=64'><button type='button' class='btn btn-default'>S</button></a> </div></div>";


// ROOT page
void handle_root()
{
  String clean_days="";
  if(clean_daily_mask & 1)
  {
      clean_days += String("M");
  } else clean_days += String("_");
  if(clean_daily_mask & 2)
  {
      clean_days += String("T");
  } else clean_days += String("_");
  if(clean_daily_mask & 4)
  {
      clean_days += String("W");
  } else clean_days += String("_");
  if(clean_daily_mask & 8)
  {
      clean_days += String("T");
  } else clean_days += String("_");
  if(clean_daily_mask & 16)
  {
      clean_days += String("F");
  } else clean_days += String("_");
  if(clean_daily_mask & 32)
  {
      clean_days += String("S");
  } else clean_days += String("_");
  if(clean_daily_mask & 64)
  {
      clean_days += String("S");
  } else clean_days += String("_");
   
  String title1     = panelHeaderName + String("Roombot Settings") + panelHeaderEnd;
  String StatusHTML = "";

  StatusHTML+= panelBodySymbol + String("globe") + panelBodyName + String("Signal") + panelBodyValue + state_rssi + String("dBm") + panelBodyEnd;

  StatusHTML+= panelBodySymbol + String("time") + panelBodyName + String("Time UTC") + panelBodyValue + day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" ") + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("time") + panelBodyName + String("Clean Time") + panelBodyValue + clean_hour + String(" h") + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("time") + panelBodyName + String("Clean Days") + panelBodyValue + clean_days + String(" ") + panelBodyEnd;

  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Charge State") + panelBodyValue + roomba_state1 + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Volt") + panelBodyValue + roomba_state_volt + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Current") + panelBodyValue + roomba_state_current + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Dist") + panelBodyValue + roomba_state_dist + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Temp") + panelBodyValue + roomba_state_temp + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("ADC") + panelBodyValue + state_adc + String(" AutoClean:")+ clean_auto_on + String(" LED:")+ state_led + panelBodyEnd;
  StatusHTML+= panelBodySymbol + String("info-sign") + panelBodyName + String("Log") + "<pre>" +  strLog + "\n</pre>" + panelBodyEnd;


//  String title2     = panelHeaderName + String("Pimatic server") + panelHeaderEnd;
//  String IPAddServ    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + host + panelBodyEnd;
//  String User     = panelBodySymbol + String("user") + panelBodyName + String("Username") + panelBodyValue + Username + panelBodyEnd + panelEnd;


  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol +                 panelBodyName +                        panelcenter + roombacontrol +  panelBodyEnd;
         commands+= panelBodySymbol + String("time")+ panelBodyName + String("Clean Time") + panelcenter + linksCleanHour + panelBodyEnd;
         commands+= panelBodySymbol + String("time")+ panelBodyName + String("Clean Days") + panelcenter + linksCleanDays + panelBodyEnd;

  server.send ( 200, "text/html", header + navbar + containerStart + title1 + StatusHTML + panelEnd + title3 + commands + panelEnd + containerEnd + siteEnd);
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["cleanTime"] = clean_hour;
  json["cleanDays"] = clean_daily_mask;
  json["cleanAuto"] = clean_auto_on;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    strLog+="Failed to open config file for writing\n";
    return false;
  }

  json.printTo(configFile);
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    strLog+="Failed to open config file\n";
    saveConfig();
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    strLog+="Config file size is too large\n";
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    strLog+="Failed to parse config file\n";
    return false;
  }

  if(json.containsKey("cleanTime"))
  {
    clean_hour = json["cleanTime"];
  }
  if(json.containsKey("cleanDays"))
  {
    clean_daily_mask = json["cleanDays"];
  }
  if(json.containsKey("cleanAuto"))
  {
    clean_auto_on = json["cleanAuto"];
  }

  return true;
}

// Setup
void setup(void)
{
  Serial.begin(115200);
  mySerial.begin(115200);
  pinMode(SERIAL_RX, INPUT);
  pinMode(SERIAL_TX, OUTPUT);
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  digitalWrite(GPIO_LED, 0); // on
  
  // Check if SPIFFS is OK
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS failed, needs formatting");
    handleFormat();
    delay(500);
    ESP.restart();
  }
  else
  {
    FSInfo fs_info;
    if (!SPIFFS.info(fs_info))
    {
      strLog+="fs_info failed\n";
    }
    else
    {
      FSTotal = fs_info.totalBytes;
      FSUsed = fs_info.usedBytes;
    }
  }

  loadConfig();
  
  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 31)
  {
    delay(1000);
    strLog+=".";
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    WiFi.disconnect();
    delay(1000);
    strLog+="Couldn't connect to network :( \n";
  /*  Serial.println("Setting up access point");
    Serial.println("SSID: ");
    Serial.println(APssid);
    Serial.println("password: ");
    Serial.println(APpassword);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APssid, APpassword);
    WMode = "AP";
    Serial.print("Connected to ");
    Serial.println(APssid);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("IP address: ");
    Serial.println(myIP);*/
  }
  else
  {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(espName);

  }

  server.on ( "/format", handleFormat );
  server.on("/", handle_root);
  server.on("/index.html", handle_root);
  server.on("/status", handle_root);
  server.on("/", handle_fupload_html);
  server.on("/api", handle_api);
  server.on("/updatefwm", handle_updatefwm_html);
  server.on("/fupload", handle_fupload_html);
  server.on("/filemanager_ajax", handle_filemanager_ajax);
  server.on("/delete", handleFileDelete);
  server.on("/roombastart", handle_roomba_start);
  server.on("/roombadock", handle_roomba_dock);
  server.on("/roombastop", handle_roomba_stop);
  server.on("/restart", handle_esp_restart);
  server.on("/ntp", handle_ntp);


  // Upload firmware:
  server.on("/updatefw2", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []()
  {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      fileName = upload.filename;
      Serial.setDebugOutput(true);
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        Update.printError(Serial);
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      if (Update.end(true)) //true to set the size to the current progress
      {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      }
      else
      {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);

    }
    yield();
  });
  // upload file to SPIFFS
  server.on("/fupload2", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      fileName = upload.filename;
      Serial.setDebugOutput(true);
      //fileName = upload.filename;
      Serial.println("Upload Name: " + fileName);
      String path;
      if (fileName.indexOf(".css") >= 0)
      {
        path = "/css/" + fileName;
      }
      else if (fileName.indexOf(".js") >= 0)
      {
        path = "/js/" + fileName;
      }
      else if (fileName.indexOf(".otf") >= 0 || fileName.indexOf(".eot") >= 0 || fileName.indexOf(".svg") >= 0 || fileName.indexOf(".ttf") >= 0 || fileName.indexOf(".woff") >= 0 || fileName.indexOf(".woff2") >= 0)
      {
        path = "/fonts/" + fileName;
      }
      else
      {
        path = "/" + fileName;
      }
      UploadFile = SPIFFS.open(path, "w");
      // already existing file will be overwritten!
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (UploadFile)
        UploadFile.write(upload.buf, upload.currentSize);
      Serial.println(fileName + " size: " + upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Serial.print("Upload Size: ");
      Serial.println(upload.totalSize);  // need 2 commands to work!
      if (UploadFile)
        UploadFile.close();
    }
    yield();
  });

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  if (!mdns.begin(espName.c_str(), WiFi.localIP())) 
  {
    strLog+="Error setting up MDNS responder!\n";
  }
  server.begin();
    // get IP
//  IPAddress ip = WiFi.localIP();
//  ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  ticker_state_charge.attach(30, handle_esp_charging);
  ticker_state_dist.attach(10, handle_esp_distance);

  ticker_ntp.attach(36000, tick_ntp);
  
  udp.begin(localPort);

  tick_ntp();
  digitalWrite(GPIO_LED,1); // off

  for(int i=0;i<24;i++)
  {
    linksCleanHour   +=  "<a href='api?action=cleanHour&value="+String(i)+"'><button type='button' class='btn btn-default'>"+String(i)+"</button></a>\n";
  }
  linksCleanHour+="</div></div>";

}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path)
{
  Serial.println("handleFileRead: " + path);

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    if ( (path.startsWith("/css/") || path.startsWith("/js/") || path.startsWith("/fonts/")) &&  !path.startsWith("/js/insert"))
    {
      server.sendHeader("Cache-Control", " max-age=31104000");
    }
    else
    {
      server.sendHeader("Connection", "close");
    }
    size_t sent = server.streamFile(file, contentType);
    size_t contentLength = file.size();
    file.close();
    return true;
  }
  else
  {
    //Serial.println(path);
  }
  return false;
}

bool handle_roomba_state()
{
  int data,data_high;
  if (mySerial.available()==26) {
    for(int i=0; i<26;i++)
    {
      data = mySerial.read();
      if(i==13)
      {
//        Serial.print("dist=");
        state_dist_total -= (int16_t)(data|data_high<<8);
        if((state_led==0) && (state_dist_total>(clean_dist+10000)))
        {
          state_led = 1;
          clean_dist = state_dist_total;
        }
//        Serial.println(state_dist_total);
        roomba_state_dist = (state_dist_total/1000)+String("m ") + (state_dist_total-clean_dist)/1000 +String("m") ;
      }
      if(i==16)
      {
 //       Serial.print("charge state=");
 //       Serial.println((int)data);
        switch(data)
        {
          case 0: roomba_state1 = String("0 Not Charging"); break;
          case 1: roomba_state1 = String("1 Charging Recovery"); break;
          case 2: roomba_state1 = String("2 Charging"); break;
          case 3: roomba_state1 = String("3 Trickle Charging"); break;
          case 4: roomba_state1 = String("4 Waiting"); break;
          default:
          case 5: roomba_state1 = String("5 Charging Error"); break;
        }
      }
      if(i==18)
      {
//        Serial.print("bat=");
//        Serial.println((uint16_t)(data|data_high<<8));
        roomba_state_volt = (uint16_t)(data|data_high<<8) + String("mV");
      }
      if(i==20)
      {
//        Serial.print("current=");
//        Serial.println((int16_t)(data|data_high<<8));
        roomba_state_current = (int16_t)(data|data_high<<8) + String("mA");
      }
      if(i==21)
      {
//        Serial.print("temp=");
//        Serial.println((int)data);
        roomba_state_temp = data + String("&degC");
      }
      if(i==23)
      {
        Serial.print("charge=");
        Serial.println((uint16_t)(data|data_high<<8));
      }
      if(i==25)
      {
        Serial.print("cap=");
        Serial.println((uint16_t)(data|data_high<<8));
      }
      data_high = data;
    }
    return true;
  }
  return false;
}

void handle_soft_serial()
{
  switch(serial_state)
  {
    case 1: if(handle_roomba_state()) serial_state = 0;
    break;
    default:
    if (mySerial.available()) {    
      Serial.println("got data:");
      while(mySerial.available())
      {
        Serial.print(mySerial.read());
        Serial.print(" ");
      }
      Serial.println(";");
    }    
  }
}

// LOOP
int button_counter;
void loop(void)
{
  if (strLog.length() > 300)
  {
    strLog.remove(0, 1);
  }

  handle_soft_serial();
  server.handleClient();

  if(!digitalRead(GPIO_BUTTON))
  {
    state_led=0;
    button_counter++;
    clean_dist = state_dist_total;
    if(button_counter>100000)
    {
      button_counter = 0;      
      clean_auto_on = !clean_auto_on;
    }
  } else {
    button_counter = 0;
  }
  
  if(clean_auto_on)
  {
    pinMode(GPIO_LED,OUTPUT);
    if(state_led)
    {
        digitalWrite(GPIO_LED,0); // on
    } else 
    {
        digitalWrite(GPIO_LED,1); // off    
    }
  } else
  {
    pinMode(GPIO_LED, INPUT);    
  }
}

// handles
void handle_ntp()
{
  String ntp_res;
  ntp_res = query_ntp();
  server.send(200, "text/plain", "  Time:"+ day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" \n")+ ntp_res);
}
void handle_api()
{
  // Get vars for all commands
  String action = server.arg("action");
  String value = server.arg("value");
  String api = server.arg("api");

  if (action == "clean" && value == "start")
  {
    handle_roomba_start();

  } else if (action == "dock" && value == "home")
  {
    handle_roomba_dock();
  } else if (action == "reset" && value == "true")
  {
    server.send ( 200, "text/html", "Reset ESP OK");
    delay(500);
    Serial.println("RESET");
    ESP.restart();
  } else {
    bool changed = false;
    if(action == "cleanHour")
    {
      clean_hour = value.toInt();
      changed = true;
    }
    if(action == "cleanDays")
    {
      int bits = value.toInt()&0x7f;
      if(clean_daily_mask & bits) clean_daily_mask &= ~bits;
      else clean_daily_mask |= bits;
      changed = true;
    }
    handle_root();
    if(changed)
    {
      saveConfig();
    }
  }
}

void handle_updatefwm_html()
{
  server.send ( 200, "text/html", "<form method='POST' action='/updatefw2' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form><br<b>For firmware only!!</b>");
}

void handle_fupload_html()
{
  String HTML = "<br>Files on flash:<br>";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    HTML += fileName.c_str();
    HTML += " ";
    HTML += formatBytes(fileSize).c_str();
    HTML += " , ";
    HTML += fileSize;
    HTML += "<br>";
    //Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }

  server.send ( 200, "text/html", "<form method='POST' action='/fupload2' enctype='multipart/form-data'><input type='file' name='update' multiple><input type='submit' value='Update'></form><br<b>For webfiles only!!</b>Multiple files possible<br>" + HTML);
}

void handle_update_upload()
{
  if (server.uri() != "/update2") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
    Serial.printf("Update: %s\n", upload.filename.c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  }
  yield();
}

void handle_update_html2()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void handleFileDelete()
{
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  if (!path.startsWith("/")) path = "/" + path;
  strLog+="handleFileDelete: " + path+"\n";
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFormat()
{
  server.send ( 200, "text/html", "OK");
  strLog+="Format SPIFFS\n";
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      strLog+="Format SPIFFS failed\n";
    }
  }
  else
  {
    strLog+="Format SPIFFS failed\n";
  }
  if (!SPIFFS.begin())
  {
    strLog+="SPIFFS failed, needs formatting\n";
  }
  else
  {
    strLog+="SPIFFS mounted\n";
  }
}
void handle_filemanager_ajax()
{
  String form = server.arg("form");
  if (form != "filemanager")
  {
    String HTML;
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
      fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      HTML += String("<option>") + fileName + String("</option>");
    }

    // Glue everything together and send to client
    server.send(200, "text/html", HTML);
  }
}

void roomba_start()
{
  strLog+=String(hour()) + ":" + String(minute()) +" Starting";
  mySerial.write(128); // start
  delay(5);
  mySerial.write(131); // safe
  delay(5);
  mySerial.write(135); // clean  
}

void handle_roomba_start()
{
  roomba_start();
  server.send(200, "text/plain", "GOGO");
}

void handle_roomba_dock()
{
  mySerial.write(128);
  delay(5);
  mySerial.write(131);
  delay(5);
  mySerial.write(143);
  strLog+=String(hour()) + ":" + String(minute()) +" going home\n";
  server.send(200, "text/plain", "GOGO Home");
}

void handle_roomba_stop()
{
  mySerial.write(128);
  delay(5);
  mySerial.write(131);
  delay(5);
  mySerial.write(133); // power
  strLog+=String(hour()) + ":" + String(minute()) + " stop\n";
  server.send(200, "text/plain", "NOGO");
//  handle_root();
}


void handle_esp_restart() {
  ESP.restart();
}

void handle_esp_charging() {
  state_rssi = WiFi.RSSI();

  mySerial.write(128); // start
  delay(5);
  mySerial.write(142); // sensor
  delay(5);
  mySerial.write((byte)0); // all
  delay(5);
  serial_state=1;
}

void handle_esp_distance() {
  
  state_adc  = analogRead(A0);
  // print out the value you read:
  //Serial.println(state_adc);
  if(state_led==0)
  {
    if((1<<((weekday()-2)%7)) & clean_daily_mask)
    {
      if(hour()==clean_hour)
      {
        if(clean_auto_on)
        {
          roomba_start();
          state_led = 1;
        }      
      }
    }
  }
}


void tick_ntp()
{
  strLog+=query_ntp();
}
String query_ntp()
{
  String result="";
    //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    result += ("no packet yet\n");
  }
  else {
    result += ("packet received, length=");
    result += String(cb)+"\n";
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    result += ("Seconds since Jan 1 1900 = " );
    result += String(secsSince1900);
    if(secsSince1900==0) return result;
    // now convert NTP time into everyday time:
    result += ("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    result += String(epoch)+"\n";
    setTime(epoch);
  
  }
  // wait ten seconds before asking for the time again
  return result;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  strLog+="sending NTP packet...";
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

