/*
 * json_helper.cpp
 *
 *  Created on: 10.11.2018
 *      Author: max
 */
#include "json_helper.h"
#include <FS.h>
#include <string>

extern String strLog;


bool saveConfig(String filename, JsonDocument &jsonConfig) {
	SPIFFS.remove(filename);
	File configFile = SPIFFS.open(filename, "w");
	if (!configFile) {
		strLog += ("Failed to open "+filename+" for writing\n");
		return false;
	}
	serializeJson(jsonConfig, configFile);
	configFile.close();
	//strLog += ("saved Settings\n");
	return true;
}

bool loadConfig(String filename,JsonDocument &jsonConfig) {
  File configFile = SPIFFS.open(filename, "r");
  if (!configFile) {
    strLog += ("Failed to open:"+filename+"\n");
    saveConfig(filename, jsonConfig);
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    strLog += ("Config file size is too large\n");
    return false;
  }


  DynamicJsonDocument jsonBuffer(1024);

  DeserializationError error = deserializeJson(jsonBuffer, configFile);
  if (error) {
    strLog += ("Failed to parse config file "+ String(error.c_str()) +"\n");
    configFile.close();
    return false;
  }
  configFile.close();

  JsonObject  json= jsonBuffer.as<JsonObject>();
  for (JsonObject::iterator it = json.begin(); it != json.end(); ++it)
  {
    if (strlen(it->key().c_str()) > 1)
    {
      if(it->value().is<float>())
      {
    	  jsonConfig[it->key()] = it->value().as<float>();
      } else if(it->value().is<int>())
      {
    	  jsonConfig[it->key()] = it->value().as<int>();
      }else
      {
    	  jsonConfig[it->key()] = it->value();
      }
    }
  }

/*  String tmp;
  serializeJson(jsonConfig, tmp);
  strLog +=  tmp;*/
  return true;
}
