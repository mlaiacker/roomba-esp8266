

// Includes

#include <Time.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Ticker.h>
#include <WiFiUdp.h>

#include <SHT1X.h>

#include <OneWire.h>
#include <DallasTemperature.h>

DeviceAddress oneAddr[8];

Ticker ticker_state_charge;
Ticker ticker_state_dist;
Ticker ticker_ntp;
Ticker ticker_off;

unsigned int localPort = 5390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

String strLog = "";
String roombotVersion = "0.3.6";
String WMode = "1";

OneWire oneWire(D1);
#define GPIO_RELAY2   D2
SHT1x sht15(D5, D6);//Data, SCK
#define GPIO_RELAY    D7
#define GPIO_RELAY3   D8

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Div
File UploadFile;
String fileName;
String  BSlocal = "0";


// WIFI
String ssid    = "mlaiacker";
String password = "83kdfiafo274DF";
//String ssid    = "FRITZ!Box 7430 FJ";
//String password = "50782790526795774722";

String espName    = "esp-humid";

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

int state_led = 0;
int started_by_timer = 0;

StaticJsonDocument<500>  jsonBufferSettings;
StaticJsonDocument<500> jsonBufferData;

JsonObject& jsonSettings = jsonBufferSettings.to<JsonObject>();
JsonObject& jsonData = jsonBufferData.to<JsonObject>();

// HTML
String header       =  "<html lang='en'><head><title>" + espName + " control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='//maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>esp-humid control panel</a></div><div><ul class='nav navbar-nav'><li><a href='.'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li><li><a href='/filemanager.html'><span class='glyphicon glyphicon-file'></span> File manager</a></li><li><a href='/fupload'> File upload</a></li><li><a href='/ntp'> NTP</a></li></ul></li><li><a href='https://github.com/incmve/roomba-eps8266/wiki' target='_blank'><span class='glyphicon glyphicon-question-sign'></span> Help</a></li></ul></div></div></nav>";
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

String roombacontrol    =  "<a href='roombastart'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> Start</button></a><a href='roombadock'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-home' aria-hidden='true'></span> Dock</button></a> <a href='roombastop'><button type='button' class='btn btn-default'><span class='glyphicon glyphicon-stop' aria-hidden='true'></span> Stop</button></a></div></div>";
String linksCleanHour;//   =  "<a href='api?action=cleanHour&value=0'><button type='button' class='btn btn-default'>0</button></a> <a href='api?action=cleanHour&value=1'><button type='button' class='btn btn-default'>1</button></a> <a href='api?action=cleanHour&value=2'> <button type='button' class='btn btn-default'>2</button></a> <a href='api?action=cleanHour&value=3'><button type='button' class='btn btn-default'>3</button></a> </div></div>";
String linksCleanDays   =  "<a href='api?action=cleanDays&value=1'><button type='button' class='btn btn-default'>M</button></a> <a href='api?action=cleanDays&value=2'><button type='button' class='btn btn-default'>T</button></a> <a href='api?action=cleanDays&value=4'><button type='button' class='btn btn-default'>W</button></a> <a href='api?action=cleanDays&value=8'><button type='button' class='btn btn-default'>T</button></a> <a href='api?action=cleanDays&value=16'><button type='button' class='btn btn-default'>F</button></a> <a href='api?action=cleanDays&value=32'><button type='button' class='btn btn-default'>S</button></a> <a href='api?action=cleanDays&value=64'><button type='button' class='btn btn-default'>S</button></a> </div></div>";

// Setup
void setup(void)
{
  jsonSettings["Duration"] = 0;
  jsonSettings["DurationA"] = 0;
  jsonSettings["DurationB"] = 0;
  jsonSettings["DurationC"] = 0;
  jsonSettings["Power"] = 10;
  jsonSettings["Threshold"] = 0.0f;
  jsonSettings["cleanTime"] = 0;
  jsonSettings["cleanDays"] = 0;

  int FSTotal;
  int FSUsed;

  Serial.begin(115200);
  pinMode(GPIO_RELAY, OUTPUT);
  pinMode(GPIO_RELAY2, OUTPUT);
  pinMode(GPIO_RELAY3, OUTPUT);
  //  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  digitalWrite(GPIO_RELAY, 0); // off
  digitalWrite(GPIO_RELAY2, 0); // off
  digitalWrite(GPIO_RELAY3, 0); // off
  analogWriteFreq(2500);
  
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
    strLog += ".";
    Serial.print(".");
    ++i;
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
  server.on("/roombastart", handle_roomba_start);
  server.on("/roombadock", handle_roomba_dock);
  server.on("/roombastop", handle_roomba_stop);
  server.on("/restart", handle_esp_restart);
  server.on("/ntp", handle_ntp);
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
  // Start up the library ds1820
  sensors.begin();
  // get IP
  //IPAddress ip = WiFi.localIP();
  //ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

  strLog += "HTTP server started\n";

  ticker_state_charge.attach(30, handle_esp_charging);
  ticker_state_dist.attach(60, tick_read);
  ticker_ntp.attach(36000, tick_ntp);

  udp.begin(localPort);

  tick_ntp();
  tick_read();
  //  digitalWrite(GPIO_LED,1); // off

  for (int i = 0; i < 24; i++)
  {
    linksCleanHour   +=  "<a href='api?action=cleanTime&value=" + String(i) + "'><button type='button' class='btn btn-default'>" + String(i) + "</button></a>\n";
  }
  linksCleanHour += "</div></div>";
}

void oneFindDev()
{
  strLog += "Found ";
  strLog += String(sensors.getDeviceCount(), DEC);
  strLog += (" devices.\n");
  oneWire.reset_search();
  // assigns the first address found to insideThermometer
  int i = 0;
  while (oneWire.search(oneAddr[i]) && i < 8) {
    strLog += "Found: " + printAddress(oneAddr[i]) + "\n";
    sensors.requestTemperatures();
    strLog += "temp=" + String(sensors.getTempC(oneAddr[i])) + "\n";
    i++;
  }

}
String printAddress(DeviceAddress deviceAddress)
{
  String  result = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) result += ("0");
    result += String(deviceAddress[i], HEX);
  }
  return result;
}

void updateData()
{
  sensors.requestTemperatures();
  for (int i = 0; i < 8; i++)
  {
    if (oneAddr[i][0] == 0x28)
    {
      jsonData[printAddress(oneAddr[i])] = sensors.getTempC(oneAddr[i]);
    }
  }
//  jsonData["PumpA"] = getState();
//  jsonData["PumpB"] = digitalRead(GPIO_RELAY2);
//  jsonData["PumpC"] = digitalRead(GPIO_RELAY3);
  int clean_daily_mask = jsonSettings["cleanDays"];
  String clean_days = "";
  if (clean_daily_mask & 1)
  {
    clean_days += String("M");
  } else clean_days += String("_");
  if (clean_daily_mask & 2)
  {
    clean_days += String("T");
  } else clean_days += String("_");
  if (clean_daily_mask & 4)
  {
    clean_days += String("W");
  } else clean_days += String("_");
  if (clean_daily_mask & 8)
  {
    clean_days += String("T");
  } else clean_days += String("_");
  if (clean_daily_mask & 16)
  {
    clean_days += String("F");
  } else clean_days += String("_");
  if (clean_daily_mask & 32)
  {
    clean_days += String("S");
  } else clean_days += String("_");
  if (clean_daily_mask & 64)
  {
    clean_days += String("S");
  } else clean_days += String("_");
  jsonData["Clean Days"] = clean_days;
  jsonData["Clean Time"] = jsonSettings["cleanTime"].as<int>();
}

// ROOT page ##############
void handle_root()
{
  updateData();

  String title1     = panelHeaderName + espName + String(" Data") + panelHeaderEnd;
  String StatusHTML = "";

  //StatusHTML+=    panelBodySymbol + String("globe") + panelBodyName + String("RSSI") + panelBodyValue + jsonData["rssi"] + String("dBm") + panelBodyEnd;
  StatusHTML +=   panelBodySymbol + String("time") + panelBodyName + String("Time UTC") + panelBodyValue + day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" ") + panelBodyEnd;

  for (JsonObject::iterator it = jsonData.begin(); it != jsonData.end(); ++it)
  {
    // *it contains the key/value pair
    const char* key = it->key;

    // it->value contains the JsonVariant which can be casted as usual
    const char* value = it->value;

    // this also works
    StatusHTML += panelBodySymbol + String("info-sign") + panelBodyName + it->key + panelBodyValue + it->value.as<String>() + panelBodyEnd;
  }

  StatusHTML += panelBodySymbol + String("info-sign") + panelBodyName + String("Log") + "<pre>" +  strLog + "\n</pre>" + panelBodyEnd;

  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol +                 panelBodyName +                        panelcenter + roombacontrol +  panelBodyEnd;
  //commands += panelBodySymbol + String("time") + panelBodyName + String("Clean Time") + panelcenter + linksCleanHour + panelBodyEnd;
  commands += panelBodySymbol + String("time") + panelBodyName + String("Clean Days") + panelcenter + linksCleanDays + panelBodyEnd;
  
  for (JsonObject::iterator it = jsonSettings.begin(); it != jsonSettings.end(); ++it)
  {
    // *it contains the key/value pair
    const char* key = it->key;
    // it->value contains the JsonVariant which can be casted as usual
    const char* value = it->value;
    commands += inputBodyStart + inputBodyName + " " + it->key + " " + inputBodyPOST + it->key + "' value='" + it->value.as<String>() + inputBodyClose + "</form>";
  }
  server.send ( 200, "text/html", header + navbar + containerStart + title1 + StatusHTML + panelEnd + title3 + commands + panelEnd + containerEnd + siteEnd);
}


bool handle_roomba_state()
{
}

void loop(void)
{
  server.handleClient();
  if (strLog.length() > 300)
  {
    strLog.remove(0, 1);
  }

}


void handle_roomba_start()
{
  state_led=7;
  start();
  server.send(200, "text/plain", "GOGO");
}

void handle_roomba_dock()
{
  server.send(200, "text/plain", "GOGO Home");
  //  handle_root();
}

void handle_roomba_stop()
{
  stop();
  server.send(200, "text/plain", "NOGO");
}

void start()
{
  int power = jsonSettings["Power"].as<int>();
  if(state_led&1)
  {
    state_led &= ~(1);
    if(jsonSettings["DurationA"].as<int>()>0)
    {
      analogWrite(GPIO_RELAY, power); // on
      ticker_off.once(jsonSettings["DurationA"].as<int>(), stop);
    }
  } else if(state_led&2)
  {
    state_led &= ~(2);
    if(jsonSettings["DurationB"].as<int>()>0)
    {
      analogWrite(GPIO_RELAY2, power); // on
      ticker_off.once(jsonSettings["DurationB"].as<int>(), stop);
    }
  } else if(state_led&4)
  {
    state_led &= ~(4);
    if(jsonSettings["DurationC"].as<int>()>0)
    {
      analogWrite(GPIO_RELAY3, power); // on
      ticker_off.once(jsonSettings["DurationC"].as<int>(), stop);
    }
  } else {
      state_led =0;
      ticker_off.once(jsonSettings["Duration"].as<int>(), stop);      
  }
  strLog += String(hour()) + ":" + String(minute()) + " Starting " + state_led + "\n";
}

void stop()
{
  strLog += String(hour()) + ":" + String(minute()) + " Stop " + state_led +"\n";
  analogWrite(GPIO_RELAY, 0); // off
  analogWrite(GPIO_RELAY2, 0); // off
  analogWrite(GPIO_RELAY3, 0); // off
  if(state_led!=0)
  {
    start();
  }
}

int getState()
{
  return digitalRead(GPIO_RELAY);
}



void handle_esp_restart() {
  stop();
  ESP.restart();
}

void handle_esp_charging() {
  jsonData["RSSI"] = WiFi.RSSI();
}

void tick_read() {
  jsonData["ADC"]  = analogRead(A0);
  
  float tempC= sht15.readTemperatureC();
  float humidity = sht15.readHumidity();


  Serial.println(millis());
  Serial.print(" Temp = ");
  Serial.print(tempC);
  Serial.println("C");
  Serial.print(" Humidity = ");
  Serial.print(humidity);
  Serial.println("%");

  if(humidity>100 || humidity<=0 || tempC<-100 || tempC>200)
  {
    jsonData["Temperature"]  = "";
    jsonData["Humidity"] = "";
  } else{
    jsonData["Temperature"]  = tempC;
    jsonData["Humidity"] = humidity;    
  }
    int clean_hour = jsonSettings["cleanTime"];
    float settings_thresh = jsonSettings["Threshold"];
    int clean_daily_mask = jsonSettings["cleanDays"];
    if ((1 << ((weekday() - 2) % 7)) & clean_daily_mask)
    {
      if (hour() == clean_hour)
      {
        if ((humidity > settings_thresh || settings_thresh == -1) &&  started_by_timer == 0)
        {
          strLog += "Humidity= " + String(humidity) + "\n";
          started_by_timer = 1;
          state_led |= 7;
          start();
        }
      }
    }

    if (hour() != clean_hour)
      started_by_timer = 0;
}


// handles
void handle_ntp()
{
  String ntp_res;
  ntp_res = query_ntp();
  server.send(200, "text/plain", "  Time:" + day() + String(".") + month() + String(".") + year() + String("   ") + hour() + String(":") + minute() + String(":") + second() + String(" \n") + ntp_res);
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
    state_led = 0;
    stop();
  } else if (action == "clean")
  {
    state_led |=value.toInt(); 
    start();
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
    if (action == "cleanDays")
    {
      int bits = value.toInt() & 0x7f;
      int clean_daily_mask = jsonSettings["cleanDays"];
      if (clean_daily_mask & bits) clean_daily_mask &= ~bits;
      else clean_daily_mask |= bits;
      jsonSettings["cleanDays"]  = clean_daily_mask;
      changed = true;
    } else {
      if (jsonSettings.containsKey(action))
      {
        jsonSettings[action] = value.toInt();
        changed = true;
      }
    }
    for (JsonObject::iterator it = jsonSettings.begin(); it != jsonSettings.end(); ++it)
    {
      if (server.hasArg(it->key))
      {
        if(it->value.is<float>())
          it->value = server.arg(it->key).toFloat();
        else if(it->value.is<int>())
          it->value = server.arg(it->key).toInt();        
        changed = true;
      }
    }
    if (changed)
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

void tick_ntp()
{
  strLog += query_ntp();
  oneFindDev();
}
String query_ntp()
{
  String result = "";
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  for(int i=0;i++;i<5)
  {
    delay(200);
    int cb = udp.parsePacket();
    if (!cb) {
      result += ("no packet yet\n");
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
      result += ("\nSeconds since Jan 1 1900 = " );
      result += String(secsSince1900);
      if (secsSince1900 == 0) return result;
      // now convert NTP time into everyday time:
      result += ("\nUnix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      result += String(epoch);
      setTime(epoch);
      return result;
    }
  }
  // wait ten seconds before asking for the time again
  return result;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  strLog += ("sending NTP packet...\n");
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
  String tmp;
  serializeJson(jsonBufferSettings, tmp);
  strLog +=  tmp;
  return true;
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

