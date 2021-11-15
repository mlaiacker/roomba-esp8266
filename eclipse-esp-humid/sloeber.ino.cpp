#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2021-01-05 18:48:06

#include "Arduino.h"
#include <Time.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Ticker.h>
#include <MQTT.h>
#include <SHT1X.h>
#include <OneWire.h>
#include <DallasTemperature.h>

void setup(void) ;
void oneFindDev() ;
String printAddress(DeviceAddress deviceAddress) ;
float getEvaporRate(float T) ;
void mqttConnect() ;
void mqttReceived(String &topic, String &payload) ;
void updateData() ;
void handle_root() ;
void loop(void) ;
void handle_roomba_start() ;
void handle_roomba_dock() ;
void handle_roomba_stop() ;
void start() ;
void stop() ;
void handle_esp_restart() ;
void handle_esp_charging() ;
void tick_read() ;
void handle_ntp() ;
void handle_api() ;
void handle_api2() ;
void handle_settings() ;
void handle_updatefwm_html() ;
void handle_fupload_html() ;
void handle_update_upload() ;
void handle_update_html2() ;
void handleFileDelete() ;
void handleFormat() ;
void handle_filemanager_ajax() ;
void tick_ntp() ;
String query_ntp() ;
void sendNTPpacket(IPAddress& address) ;
String formatBytes(size_t bytes) ;
void mqttJsonPub(JsonObject &json, String name) ;
bool saveConfig() ;
bool loadConfig() ;
String getContentType(String filename) ;
bool handleFileRead(String path) ;

#include "esp_humid.ino"


#endif
