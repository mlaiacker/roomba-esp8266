/*
 * json_helper.cpp
 *
 *  Created on: 10.11.2018
 *      Author: max
 */

#ifndef JSON_HELPER_CPP_
#define JSON_HELPER_CPP_

#include <ArduinoJson.h>

bool saveConfig(String filename, JsonDocument &jsonConfig);
bool loadConfig(String filename,JsonDocument &jsonConfig);


#endif /* JSON_HELPER_CPP_ */
