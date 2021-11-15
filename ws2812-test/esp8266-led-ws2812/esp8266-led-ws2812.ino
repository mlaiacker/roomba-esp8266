
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

#include <FS.h>
#include <Ticker.h>


#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>


Ticker ticker_state_charge;
Ticker ticker_state_dist;


const uint16_t PixelCount = 60; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

#define colorSaturation 128

// three element pixels, in different order and speeds
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

// Uart method is good for the Esp-01 or other pin restricted modules
// NOTE: These will ignore the PIN and use GPI02 pin
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

HslColor hslRed(red);
HslColor hslGreen(green);
HslColor hslBlue(blue);
HslColor hslWhite(white);
HslColor hslBlack(black);

String roombotVersion = "0.3.6";
String WMode = "1";

#define SERIAL_RX     D5  // pin for SoftwareSerial RX
#define SERIAL_TX     D6  // pin for SoftwareSerial TX
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
String ssid    = "";
String password = "";
String espName    = "Roombot";

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

// AP mode when WIFI not available
const char *APssid = "Roombot";
const char *APpassword = "thereisnospoon";



// Pimatic settings
String host   = "192.168.x.x";
const int httpPort    = 80;
String Username     = "admin";
String Password     = "password";
String chargevar = "chargestatus";
String distancevar = "distance";
char authVal[40];
char authValEncoded[40];
String ClientIP;

String roomba_state1 = "?";
String roomba_state_volt = "?";
String roomba_state_current = "?";
String roomba_state_temp = "?";
String roomba_state_dist = "?";
int32_t state_dist_total;
long state_rssi;
int state_adc;

int serial_state = 0;

uint8_t led_state_dim = 10;

#define BASE64_LEN 40
char unameenc[BASE64_LEN];


// HTML
//String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>Roombot control panel</a></div><div><ul class='nav navbar-nav'><li><a href='.'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li><li><a href='/filemanager.html'><span class='glyphicon glyphicon-file'></span> File manager</a></li><li><a href='/fupload'> File upload</a></li></ul></li><li><a href='https://github.com/incmve/roomba-eps8266/wiki' target='_blank'><span class='glyphicon glyphicon-question-sign'></span> Help</a></li></ul></div></div></nav>";
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
String roombacontrol     =  "<a href='roombastart'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> Start</button></a><a href='roombadock'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-home' aria-hidden='true'></span> Dock</button></a> <a href='roombastop'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-stop' aria-hidden='true'></span> Stop</button></a></div>";


// ROOT page
void handle_root()
{
  //delay(500);
   
  String title1     = panelHeaderName + String("Roombot Settings") + panelHeaderEnd;
  String IPAddClient    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + ClientIP + String(" ") + state_rssi + String("dBm") + panelBodyEnd;
//  String ClientName   = panelBodySymbol + String("tag") + panelBodyName + String("Client Name") + panelBodyValue + espName + panelBodyEnd;
//  String Version     = panelBodySymbol + String("info-sign") + panelBodyName + String("Roombot Version") + panelBodyValue + roombotVersion + panelBodyEnd;
  String Uptime     = panelBodySymbol + String("time") + panelBodyName + String("Uptime") + panelBodyValue + hour() + String(" h ") + minute() + String(" min ") + second() + String(" sec") + panelBodyEnd;

  String Status1     = panelBodySymbol + String("info-sign") + panelBodyName + String("Charge State") + panelBodyValue + roomba_state1 + panelBodyEnd;
  String Status2     = panelBodySymbol + String("info-sign") + panelBodyName + String("Volt") + panelBodyValue + roomba_state_volt + panelBodyEnd;
  String Status3     = panelBodySymbol + String("info-sign") + panelBodyName + String("Current") + panelBodyValue + roomba_state_current + panelBodyEnd;
  String Status4     = panelBodySymbol + String("info-sign") + panelBodyName + String("Dist") + panelBodyValue + roomba_state_dist + panelBodyEnd;
  String Status5     = panelBodySymbol + String("info-sign") + panelBodyName + String("Temp") + panelBodyValue + roomba_state_temp + panelBodyEnd;
  String Status6     = panelBodySymbol + String("info-sign") + panelBodyName + String("ADC") + panelBodyValue + state_adc + panelBodyEnd;


//  String title2     = panelHeaderName + String("Pimatic server") + panelHeaderEnd;
//  String IPAddServ    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + host + panelBodyEnd;
//  String User     = panelBodySymbol + String("user") + panelBodyName + String("Username") + panelBodyValue + Username + panelBodyEnd + panelEnd;


  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol + panelBodyName + panelcenter + roombacontrol + panelBodyEnd;

  server.send ( 200, "text/html", header + navbar + containerStart + title1 + IPAddClient + /*ClientName + Version +*/ Uptime + Status1 + Status2 + Status3 + Status4 + Status5 + Status6 + panelEnd + title3 + commands + containerEnd + siteEnd);
}


// Setup
void setup(void)
{
  Serial.begin(115200);
  mySerial.begin(115200);
  pinMode(SERIAL_RX, INPUT);
  pinMode(SERIAL_TX, OUTPUT);
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
  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 31)
  {
    delay(1000);
    Serial.print(".");
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    WiFi.disconnect();
    delay(1000);
    Serial.println("");
    Serial.println("Couldn't connect to network :( ");
    Serial.println("Setting up access point");
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
    Serial.println(myIP);

  }
  else
  {
    Serial.println("");
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

    strip.Begin();
    strip.Show();

  ticker_state_charge.attach(30, handle_esp_charging);
  ticker_state_dist.attach(0.1, handle_esp_distance);
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
        Serial.print("dist=");
        state_dist_total += (int16_t)(data|data_high<<8);
        Serial.println(state_dist_total);
        roomba_state_dist = state_dist_total+String("mm");
      }
      if(i==16)
      {
        Serial.print("charge state=");
        Serial.println((int)data);
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
        Serial.print("bat=");
        Serial.println((uint16_t)(data|data_high<<8));
        roomba_state_volt = (uint16_t)(data|data_high<<8) + String("mV");
      }
      if(i==20)
      {
        Serial.print("current=");
        Serial.println((int16_t)(data|data_high<<8));
        roomba_state_current = (int16_t)(data|data_high<<8) + String("mA");
      }
      if(i==21)
      {
        Serial.print("temp=");
        Serial.println((int)data);
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
void loop(void)
{
  handle_soft_serial();
  server.handleClient();
  led_loop();
}

uint8_t led_index=0, led_shift=0;
void led_loop()
{
    strip.SetPixelColor(0, red);
    strip.SetPixelColor(1, green);
    strip.SetPixelColor(2, blue);
    if(led_index&1) strip.SetPixelColor(3, RgbColor(led_state_dim));
    else strip.SetPixelColor(3, RgbColor(led_state_dim/2,0,0));
    // the following line demonstrates rgbw color support
    // if the NeoPixels are rgbw types the following line will compile
    // if the NeoPixels are anything else, the following line will give an error
    //strip.SetPixelColor(3, RgbwColor(colorSaturation));
    strip.Show();

}

// handles

void handle_api()
{
  // Get vars for all commands
  String action = server.arg("action");
  String value = server.arg("value");
  String api = server.arg("api");

  if (action == "clean" && value == "start")
  {
    handle_roomba_start();

  }

  if (action == "dock" && value == "home")
  {
    handle_roomba_dock();
  }
  if (action == "reset" && value == "true")
  {
    server.send ( 200, "text/html", "Reset ESP OK");
    delay(500);
    Serial.println("RESET");
    ESP.restart();
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
  Serial.println("handleFileDelete: " + path);
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

void handle_roomba_start()
{
  Serial.println("Starting");
  mySerial.write(128); // start
  delay(5);
  mySerial.write(131); // safe
  delay(5);
  mySerial.write(135); // clean
  Serial.println("I will clean master");
  server.send(200, "text/plain", "GOGO");
//  handle_root();

  led_state_dim = 100;
}

void handle_roomba_dock()
{
  mySerial.write(128);
  delay(5);
  mySerial.write(131);
  delay(5);
  mySerial.write(143);
  Serial.println("Thank you for letting me rest, going home master");
  server.send(200, "text/plain", "GOGO Home");
  led_state_dim = 50;
//  handle_root();
}

void handle_roomba_stop()
{
  mySerial.write(128);
  delay(5);
  mySerial.write(131);
  delay(5);
  mySerial.write(133); // power
  Serial.println("Thank you for letting me rest, sleep");
  server.send(200, "text/plain", "NOGO");
  led_state_dim = 0;
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

    //strip.SetPixelColor(led_index, RgbColor(0));
    led_index++;
    if(led_index>PixelCount){
      led_index=4;
      led_shift++;
    }
    strip.SetPixelColor(led_index, HslColor((float)((led_index + led_shift)%(PixelCount-4))/(PixelCount-4),1,(float)led_state_dim/255));

}
/*
void handle_esp_pimatic(String data, String variable) {
String yourdata;
  char uname[BASE64_LEN];
  String str = String(Username) + ":" + String(Password);
  str.toCharArray(uname, BASE64_LEN);
  memset(unameenc, 0, sizeof(unameenc));
  base64_encode(unameenc, uname, strlen(uname));


  yourdata = "{\"type\": \"value\", \"valueOrExpression\": \"" + data + "\"}";

  client.print("PATCH /api/variables/");
  client.print(variable);
  client.print(" HTTP/1.1\r\n");
  client.print("Authorization: Basic ");
  client.print(unameenc);
  client.print("\r\n");
  client.print("Host: " + host +"\r\n");
  client.print("Content-Type:application/json\r\n");
  client.print("Content-Length: ");
  client.print(yourdata.length());
  client.print("\r\n\r\n");
  client.print(yourdata);
 

  delay(500);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }
}
*/
