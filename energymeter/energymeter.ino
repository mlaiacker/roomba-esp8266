// Includes
#include <Time.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <EEPROM.h>
//#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#include <FS.h>
#include <Ticker.h>
#include <WiFiUdp.h>

unsigned int localPort = 2382;      // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
Ticker ticker_ntp;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

String roombotVersion = "0.4.8";
String WMode = "1";

//#define SERIAL_RX     D5  // pin for SoftwareSerial RX
//#define SERIAL_TX     D6  // pin for SoftwareSerial TX
//#define  GPIO_LED     16

#define GPIO_SCK    14
#define GPIO_DATA   12

#define  GPIO_BUTTON     13

#define LED_CLK_ON (digitalWrite(GPIO_SCK, 1))
#define LED_CLK_OFF (digitalWrite(GPIO_SCK, 0))
#define LED_DATA_ON (digitalWrite(GPIO_DATA, 1))
#define LED_DATA_OFF (digitalWrite(GPIO_DATA, 0))

// Div
File UploadFile;
String fileName;
String  BSlocal = "0";
String strLog="";
int FSTotal;
int FSUsed;


// WIFI
String ssid    = "";
String password = "";
String espName    = "esp-meter";

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

// Pimatic settings
String ClientIP;

long state_rssi;
int state_adc;
Ticker ticker_adc;

uint8_t ledDigit[8];
uint8_t ledPoint[8];
int serial_state = 0;

float button_dtime = 0;
int button_time=0;
int button_time_ms=0;
int button_dtime_ms;

int power = 0;
float powerMean = 0;
int powerSum = 0;
int energy = 0; 
int button_count = 0;
String stateDisplay = "";

#define BASE64_LEN 40
char unameenc[BASE64_LEN];

// HTML
//String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String header       =  "<html lang='en'><head><title>"+espName+" control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>"+espName+" control panel</a></div><div><ul class='nav navbar-nav'><li><a href='.'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li><li><a href='/filemanager.html'><span class='glyphicon glyphicon-file'></span> File manager</a></li><li><a href='/fupload'> File upload</a></li><li><a href='/ntp'> NTP</a></li></ul></li><li><a href='https://github.com/incmve/roomba-eps8266/wiki' target='_blank'><span class='glyphicon glyphicon-question-sign'></span> Help</a></li></ul></div></div></nav>";
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

String inputBodyStart   =  "<form action='/api' method='POST'><div class='panel panel-default'><div class='panel-body'>";
String inputBodyName    =  "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>";
String inputBodyPOST    =  "</span><input type='text' name='";
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div>";
String roombacontrol    =  "<a href='roombastart'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> Start</button></a><a href='roombadock'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-home' aria-hidden='true'></span> Dock</button></a> <a href='roombastop'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-stop' aria-hidden='true'></span> Stop</button></a></div></div>";


// ROOT page
void handle_root()
{
   
  String title1     = panelHeaderName + String("esp-meter Settings") + panelHeaderEnd;
  String IPAddClient    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + ClientIP + String(" ") + state_rssi + String("dBm") + panelBodyEnd;

  String Uptime     = panelBodySymbol + String("time") + panelBodyName + String("Time UTC") + panelBodyValue + day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" ") + panelBodyEnd;

  String Status6     = panelBodySymbol + String("info-sign") + panelBodyName + String("ADC") + panelBodyValue + state_adc + panelBodyEnd;
  Status6 += panelBodySymbol + String("info-sign") + panelBodyName + String("Counts") + panelBodyValue + button_count + panelBodyEnd;
  Status6 += panelBodySymbol + String("info-sign") + panelBodyName + String("dt") + panelBodyValue + button_dtime + String("s ")+ panelBodyEnd;
  Status6 += panelBodySymbol + String("info-sign") + panelBodyName + String("Event") + panelBodyValue + button_time + String("s") + panelBodyEnd;
  Status6 += panelBodySymbol + String("info-sign") + panelBodyName + String("Leistung") + panelBodyValue + powerMean + String("W ")+ panelBodyEnd;

  Status6 += panelBodySymbol + String("info-sign") + panelBodyName + String("Log") + "<pre>" +  strLog + "\n</pre>"+ panelBodyEnd;

  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol +                 panelBodyName +                        panelcenter + roombacontrol +  panelBodyEnd;
         commands+= inputBodyStart + inputBodyName + " Display " + inputBodyPOST + "display' value='" + stateDisplay + inputBodyClose + "</form>";

  server.send ( 200, "text/html", header + navbar + containerStart + title1 + IPAddClient + Uptime + Status6 + panelEnd + title3 + commands + panelEnd + containerEnd + siteEnd);
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["counts"] = button_count;

  strLog+= "t:"+String(now())+"json save count"+button_count+"\n";
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    strLog+=("Failed to open config file for writing\n");
    return false;
  }

  json.printTo(configFile);
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    strLog+=("Failed to open config file for reading\n");
    saveConfig();
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    strLog+=("Config file size is too large\n");
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
    strLog+=("Failed to parse config file\n");
    return false;
  }

  if(json.containsKey("counts"))
  {
    button_count = json["counts"];
  }
  return true;
}

// Setup
void setup(void)
{
  Serial.begin(115200);
  pinMode(GPIO_SCK, OUTPUT);
  pinMode(GPIO_DATA, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
//  digitalWrite(GPIO_LED, 0); // on
  
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
      Serial.println("fs_info failed");
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
    strLog+=(".");
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    strLog+=("\n");
    strLog+=("Couldn't connect to network :( \n");
  }
  else
  {
    strLog+=("\n");
    strLog+=("Connected to ");
    strLog+=(ssid);
    strLog+=("\nIP address: ");
    strLog+=(WiFi.localIP());
    strLog+=("\nHostname: ");
    strLog+=(espName+"\n");

  }

  server.on("/format", handleFormat );
  server.on("/", handle_root);
  server.on("/index.html", handle_root);
  server.on("/status", handle_root);
  server.on("/api", handle_api);
  server.on("/", handle_fupload_html);
  server.on("/updatefwm", handle_updatefwm_html);
  server.on("/fupload", handle_fupload_html);
  server.on("/filemanager_ajax", handle_filemanager_ajax);
  server.on("/delete", handleFileDelete);
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
      strLog+=("Upload Name: " + fileName+"\n");
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
      strLog+=(fileName + " size: " + upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      strLog+=("Upload Size: ");
      strLog+=(upload.totalSize+"\n");  // need 2 commands to work!
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

  if (!mdns.begin(espName.c_str(), WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  server.begin();
    // get IP
  IPAddress ip = WiFi.localIP();
  ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  Serial.println("HTTP server started");
  ticker_ntp.attach(36000, tick_ntp);
  Serial.println("Starting UDP");
  udp.begin(localPort);

  ticker_adc.attach(10, periodic_adc);

  tick_ntp();
  button_time = now();
  button_time_ms = millis();
  ledDigit[4]=255;
  ledDigit[5]=255;
  ledDigit[6]=255;
  ledDigit[7]=255;
  Serial.println(strLog);
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
  //Serial.println("handleFileRead: " + path);

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

// LOOP
int button_counter;
int button_filter;
int button_old, button_val;
int digit = 0;
int digitTime = 0; 
void loop(void)
{
  server.handleClient();

  button_val = digitalRead(GPIO_BUTTON);
  if(!button_val && button_old==1)
  {
    if(button_filter>100)
    {
        button_dtime_ms = (millis() - button_time_ms);
        button_time_ms = millis();
        button_dtime = (float)(now() -button_time)*0 + (float)button_dtime_ms/1000.0;
        power = 3600.0/button_dtime;
        button_time = now();
        button_count++;
        powerSum += power;
        if(button_count%10==0)
        {
          powerMean = powerSum/10.0;
          powerSum = 0;
        }
    }
    button_filter = 0;
  }
  
  if(button_val>0){
    if(button_filter<100000)
    {
      button_filter++;
    }
  } else button_filter = 0;
  button_old = button_val;

  if(millis()>=digitTime)
  {
    digitTime = millis() + 1;
    if(digit>=8){
      digit=0;
      ledPoint[0] = 0;
      ledPoint[1] = 0;
      ledPoint[2] = 0;
      ledVal(0,4,power) ;
//      ledVal(4,4,state_adc) ;
    }
    uint8_t point = 0;

    ledPoint[3]=button_val;
    
    ledSetDigit(digit, ledDigit[digit], ledPoint[digit]);
    digit++;
  }
  
  if(strLog.length()>300)
  {
    strLog.remove(0,1);
  }

}

void periodic_adc() {
  state_adc  = analogRead(A0);
  state_rssi = WiFi.RSSI();
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
  handle_display();
}
void handle_display()
{
    if(server.hasArg("display"))
    {
      stateDisplay = server.arg("display");
      if(stateDisplay.length()>0)
        ledDigit[4] = stateDisplay.charAt(0);
      if(stateDisplay.length()>1)
        ledDigit[5] = stateDisplay.charAt(1);
      if(stateDisplay.length()>2)
        ledDigit[6] = stateDisplay.charAt(2);
      if(stateDisplay.length()>3)
        ledDigit[7] = stateDisplay.charAt(3);        
      //ledVal(4,4,stateDisplay);
    }
    if(server.hasArg("point"))
    {
      String param = server.arg("point");
      if(param.length()>0)
        ledPoint[4] = param.charAt(0)=='1';
      if(param.length()>1)
        ledPoint[5] = param.charAt(1)=='1';
      if(param.length()>2)
        ledPoint[6] = param.charAt(2)=='1';
      if(param.length()>3)
        ledPoint[7] = param.charAt(3)=='1';        
      //ledVal(4,4,stateDisplay);
    }

    server.send ( 200, "text/plain", "{Leistung:"+String(powerMean)+"W, Counts:"+ button_count+", ADC:" + state_adc + ", Display:"+ stateDisplay+ "}\n");
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
  strLog+= ("handleFileDelete: " + path +"\n");
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
  Serial.println("Format SPIFFS");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      Serial.println("Format SPIFFS failed");
    }
  }
  else
  {
    Serial.println("Format SPIFFS failed");
  }
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS failed, needs formatting");
  }
  else
  {
    Serial.println("SPIFFS mounted");
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


void handle_esp_restart() {
//  saveConfig();
  ESP.restart();
}

void tick_ntp()
{
  strLog+=query_ntp();
  if(button_count!=0)
  {
    saveConfig();
  }
  stateDisplay = "";
  ledDigit[4]=255;
  ledDigit[5]=255;
  ledDigit[6]=255;
  ledDigit[7]=255;
  ledPoint[4] = 0;
  ledPoint[5] = 0;
  ledPoint[6] = 0;
  ledPoint[7] = 0;
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
    result += ("no packet yet");
  }
  else {
    result += ("packet received, length=");
    result += String(cb);
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
    result += String(epoch);
    setTime(epoch);
  }
  // wait ten seconds before asking for the time again
  return result;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
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

void ledDelay(void)
{
}

void ledVal(int start, int len, int val)
{
    int divider =1;    
    int first_none_zero_found = 0;
    for(int i=1;i<len; i++) divider *=10;

    for(int i=0;i<len; i++)
    {
      ledDigit[start+i] = (val/divider)%10;
      if(ledDigit[start+i]!=0) first_none_zero_found =1;
      if(first_none_zero_found==0) ledDigit[start+i] = 255;
      divider /=10;
    }
}


unsigned char ledDecode(  unsigned char zahl)
{
  switch(zahl)
  {
    case '0':
    case  0 : return 255 - 0x3F;
    case '1':
    case  1 : return 255 - 0x06;
    case '2':
    case  2 : return 255 - 0x5b;
    case '3':
    case  3 : return 255 - 0x4f;
    case '4':
    case  4 : return 255 - 0x66;
    case '5':
    case  5 : return 255 - 0x6d;
    case '6':
    case  6 : return 255 - 0x7d;
    case '7':
    case  7 : return 255 - 0x07;
    case '8':
    case  8 : return 255 - 0x7f;
    case '9':
    case  9 : return 255 - 0x6f;
    case 'a':
    case 'A': return 255 - 0x77;
    case 'B':
    case 'b': return 255 - 0x7c;
    case 'c': return 255 - 0x58;
    case 'C': return 255 - 0x39;
    case 'D':
    case 'd': return 255 - 0x5e;
    case 'E':
    case 'e': return 255 - 0x79;
    case 'F':
    case 'f': return 255 - 0x71;

    case 'o': return 255 - 0x5c;
    case 'H': return 255 - 0x46;
    case 'J': return 255 - 0x0E;
    case 'L': return 255 - 0x38;

    case '-': return 255 - 0x40;
    case '_': return 255 - 0x08;
    case '[': return 255 - 0x39;
    case ']': return 255 - 0x0f;
    case '|': return 255 - 0x30;
    case '°': return 255 - 0x63;

    default : return 255;
  }
}

void ledSetDigit(unsigned char welches, unsigned char was, unsigned char punkt)
{
  unsigned char i,data;
  data = ledDecode(was);
  if(punkt) data &= 0x7f;
  for(i=0;i<=7;i++)
  {
    LED_CLK_OFF;
    if(data & (1<<i)) LED_DATA_ON; else LED_DATA_OFF;
    LED_CLK_ON;
    LED_CLK_ON;
    LED_DATA_OFF;
    LED_CLK_OFF;
  }
  for(i=0;i<=7;i++)
  {
    LED_CLK_OFF;
    if(welches != i) LED_DATA_ON; else LED_DATA_OFF;
    LED_CLK_ON;
    LED_CLK_ON;
    LED_DATA_OFF;
    LED_CLK_OFF;
  } 
}

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
