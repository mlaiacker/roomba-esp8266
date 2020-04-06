// Includes

#include <Time.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>

extern "C" {
  #include "user_interface.h"
}

String strLog = "";
String roombotVersion = "1.0.0";
String WMode = "1";


// Div
File UploadFile;
String fileName;
String  BSlocal = "0";


// WIFI
String ssid    = "mlaiacker";
String password = "83kdfiafo274DF";

String cmdPostURL = "http://192.168.1.200:8080/rest/items/Decke_Kueche";

String espName    = "esp-motion";

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

int state_led = 0;
int motion = 0;

StaticJsonDocument<500>  jsonBufferSettings;
StaticJsonDocument<500> jsonBufferData;

JsonObject& jsonSettings = jsonBufferSettings.to<JsonObject>();
JsonObject& jsonData = jsonBufferData.to<JsonObject>();


// HTML
String header       =  "<html lang='en'><head><title>" + espName + " control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'>"
"<div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>esp-humid control panel</a></div>"
"<div><ul class='nav navbar-nav'>"
"<li><a href='.'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li>"
"<li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a>"
"<ul class='dropdown-menu'>"
"<li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li>"
"<li><a href='/filemanager.html'><span class='glyphicon glyphicon-file'></span> File manager</a></li><li><a href='/fupload'> File upload</a></li>"
"<li><a href='/restart'> Restart</a></li>"
"</ul></li>"
"</ul></div></div></nav>";

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
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div></div></div>";

String roombacontrol    =  "<a href='roombastart'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> ON</button></a> <a href='roombastop'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-stop' aria-hidden='true'></span> OFF</button></a></div></div>";

// curl -X POST --header "Content-Type: text/plain" --header "Accept: application/json" -d "OFF" "http://192.168.1.200:8080/rest/items/Decke_Kinderzimmer"
// Setup
void setup(void)
{
  jsonSettings["adcThreshold"] = 0;
  jsonSettings["postURL"] = "http://192.168.1.200:8080/rest/items/Decke_Kueche";
  jsonData["Motion"] = 0.0;
  jsonData["ADC"] = 0.0;

  int FSTotal;
  int FSUsed;

  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, 0); 
  
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
    digitalWrite(2, 1); 
    delay(1000);
    strLog += ".";
    Serial.print(".");
    ++i;
    digitalWrite(2, 0); 
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    delay(1000);
    Serial.println("");
    Serial.println("Couldn't connect to network :( ");
  }
  else
  {
    strLog += "\n";
    strLog += "Connected to ";
    strLog += ssid + "\n";
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    strLog += " Hostname: ";
    strLog += espName + "\n";
    Serial.print(strLog);
  }
  if (!mdns.begin(espName.c_str())) {
    strLog+=("Error setting up MDNS responder!");
  }
  // Add service to MDNS-SD
  mdns.addService("http", "tcp", 80);

  server.on("/format", handleFormat );
  server.on("/", handle_root);
  server.on("/index.html", handle_root);
  server.on("/status", handle_root);
  server.on("/", handle_fupload_html);
  server.on("/updatefwm", handle_updatefwm_html);
  server.on("/fupload", handle_fupload_html);
  server.on("/filemanager_ajax", handle_filemanager_ajax);
  server.on("/delete", handleFileDelete);
  server.on("/restart", handle_esp_restart);
  server.on("/roombastart", handle_start);
  server.on("/roombastop", handle_stop);
  server.on("/api", handle_api);
  server.on("/api2", handle_api2);

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
      strLog += fileName + " size: " + upload.currentSize + "\n";
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      strLog += "Upload Size: ";
      strLog += upload.totalSize + "\n"; // need 2 commands to work!
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

  server.begin();
  strLog += "HTTP server started\n";
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  digitalWrite(2, 1); 
}

int cmdSend(bool state)
{
  if(jsonSettings["postURL"]!="")
  {
    HTTPClient http;
    http.begin(jsonSettings["postURL"]);
    http.addHeader("Content-Type", "text/plain");
    String cmd = "OFF";
    if(state) cmd = "ON";
    int httpCode = http.POST(cmd);
    if(httpCode!=200){
      strLog += String("result:")+httpCode+String("\n");
      // retry
      httpCode = http.POST(cmd);
    }
    String payload = http.getString();
    strLog +=  payload;
    return httpCode;
  } else {
    strLog +=  "no URL set";
    return 0;
  }
}

// ROOT page ##############
void handle_root()
{
  updateData();

  String title1     = panelHeaderName + espName + String(" Data") + panelHeaderEnd;
  String StatusHTML = "";

  //StatusHTML+=    panelBodySymbol + String("globe") + panelBodyName + String("RSSI") + panelBodyValue + jsonData["rssi"] + String("dBm") + panelBodyEnd;
  //StatusHTML +=   panelBodySymbol + String("time") + panelBodyName + String("Time UTC") + panelBodyValue + day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" ") + panelBodyEnd;

  //StatusHTML += panelBodySymbol + String("info-sign") + panelBodyName + String("Motion") + panelBodyValue + motion + panelBodyEnd;

  for (JsonObject::iterator it = jsonData.begin(); it != jsonData.end(); ++it)
  {
    if(it->value.as<String>().length()>0)
    {
      StatusHTML += panelBodySymbol + String("info-sign") + panelBodyName + it->key + panelBodyValue + it->value.as<String>() + panelBodyEnd;
    }
  }

  StatusHTML += panelBodySymbol + String("info-sign") + panelBodyName + String("Log") + "<pre>" +  strLog + "\n</pre>" + panelBodyEnd;

  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol +                 panelBodyName +                        panelcenter + roombacontrol +  panelBodyEnd;
  //commands += panelBodySymbol + String("time") + panelBodyName + String("Clean Time") + panelcenter + linksCleanHour + panelBodyEnd;
  //commands += panelBodySymbol + String("time") + panelBodyName + String("Clean Days") + panelcenter + linksCleanDays + panelBodyEnd;
  for (JsonObject::iterator it = jsonSettings.begin(); it != jsonSettings.end(); ++it)
  {
    commands += inputBodyStart + inputBodyName + " " + it->key + " " + inputBodyPOST + it->key + "' value='" + it->value.as<String>() + inputBodyClose + "</form>";
  }

  server.send ( 200, "text/html", header + navbar + containerStart + title1 + StatusHTML + panelEnd + title3 + commands + panelEnd + containerEnd + siteEnd);
}


void loop(void)
{
  server.handleClient();
  int pin = digitalRead(16);
  if(pin != motion)
  {    
    updateData();
    strLog += millis()/1000 + String(" Motion:")+ pin + String("\n");
    Serial.printf("%i Motion=%i\n",millis()/1000,pin);
    motion = pin;
    if((int)jsonData["ADC"] >= (int)jsonSettings["adcThreshold"])
    {
      pin =0; // dont turn light on
    }
    digitalWrite(2, !pin);
    cmdSend(pin);
  }
  if (strLog.length() > 350)
  {
    strLog.remove(0, 1);
  }
  delay(100);
}

void updateData()
{
  jsonData["Motion"] = motion;
  jsonData["ADC"] = (analogRead(0) + analogRead(0) +analogRead(0) +analogRead(0))/4;
}

void handle_start(){
  cmdSend(1);
  handle_root();
}

void handle_stop(){
  cmdSend(0);
  handle_root();
}

void handle_api()
{
  handle_settings();
  handle_root();
}

void handle_api2()
{
  handle_settings();
  updateData();
  String data;
  serializeJson(jsonBufferData, data);
  String settings;
  serializeJson(jsonBufferSettings, settings);
  server.send(200, "text/plain", "{\n\"Data\":" + data + ",\n\"Settings\":" + settings + "\n}");
}

void handle_settings()
{
  // Get vars for all commands
  String action = server.arg("action");
  String value = server.arg("value");
  String api = server.arg("api");

  if (action == "clean" && value == "stop")
  {
    cmdSend(0);
  } else if (action == "clean")
  {
    cmdSend(1);
  } else if (action == "reset" && value == "true")
  {
    server.send ( 200, "text/html", "Reset ESP OK");
    delay(500);
    Serial.println("RESET");
    ESP.restart();
  } else {
    bool changed = false;
    if (jsonSettings.containsKey(action))
    {
      jsonSettings[action] = value.toInt();
      changed = true;
    }
    for (JsonObject::iterator it = jsonSettings.begin(); it != jsonSettings.end(); ++it)
    {
      if (server.hasArg(it->key))
      {
        if(it->value.is<float>()){
          it->value = server.arg(it->key).toFloat();
        }else if(it->value.is<int>()){
          it->value = server.arg(it->key).toInt();        
        }else {
          it->value = server.arg(it->key).c_str();        
        }
        changed = true;
      }
    }
    if (changed)
    {
      yield();
      saveConfig();
    }
  }
}

bool saveConfig() {

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    strLog += ("Failed to open config file for writing\n");
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(jsonBufferSettings, configFile) == 0) {
    strLog +=("Failed to write to file");
  }
  configFile.close();
  strLog += ("saved Settings\n");
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    strLog += ("Failed to open config file\n");
    saveConfig();
    return false;
  }

  DeserializationError error = deserializeJson(jsonBufferSettings, configFile);
  if (error){
    strLog += F("Failed to read file, using default configuration");
    return false;
  }
  configFile.close();
  return true;
}


void handle_esp_restart() {
   ESP.restart();
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
  strLog += ("handleFileDelete: " + path);
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
  strLog += ("Format SPIFFS");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      strLog += ("Format SPIFFS failed\n");
    }
  }
  else
  {
    strLog += ("Format SPIFFS failed\n");
  }
  if (!SPIFFS.begin())
  {
    strLog += ("SPIFFS failed, needs formatting\n");
  }
  else
  {
    strLog += ("SPIFFS mounted\n");
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
  //strLog+=("handleFileRead: " + path+"\n");

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
