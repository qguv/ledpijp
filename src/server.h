#pragma once

#include <ESP8266WiFi.h>

enum request_type {
	REQUEST_INDEX,
	REQUEST_ANIM,
	REQUEST_ANIM_QUERY,
	REQUEST_BRIGHTNESS,
	REQUEST_BRIGHTNESS_QUERY,
	REQUEST_SPEED,
	REQUEST_SPEED_QUERY,
	REQUEST_RESET,
	REQUEST_UNDEFINED, // must be last
};

enum request_type serve(WiFiClient client);
void respond_index(WiFiClient client);
