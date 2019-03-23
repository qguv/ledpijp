#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "animation.h"
#include "apa102.h"
#include "config.h"
#include "server.h"

#define HZ_TO_MS(HZ) (1000 / (HZ))

bool server_pending = true;
bool doomed = false;

unsigned long int next_anim_time;

WiFiServer server(80);

void setup(void)
{
	next_anim_time = 0;
	anim = new_anim = ANIM_RAINBOW;

	pinMode(CLK_PIN, OUTPUT);
	digitalWrite(CLK_PIN, 0);

	pinMode(DATA_PIN, OUTPUT);
	digitalWrite(DATA_PIN, 0);

	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PASS);
}

void loop(void)
{
	if (doomed) {
		blit_solid_leds(0, 0, 0);
		ESP.reset();
		for (;;);
	}

	if (server_pending) {
		if (WiFi.status() == WL_CONNECTED) {
			server_pending = false;
			server.begin();
		} else if (millis() > WIFI_GIVEUP * 1000) {
			server_pending = false;
			WiFi.mode(WIFI_AP);
			if (WiFi.softAP("ledpijp", PASS, 1, false, 8)) {
				server.begin();
				wifi_ap();
			} else {
				wifi_fail();
			}
		}
	}

	unsigned long int t = millis();
	if (t >= next_anim_time) {
		next_anim_time = t + HZ_TO_MS(ANIM_HZ);

		if (new_anim != ANIM_UNDEFINED) {
			anim = new_anim;
			new_anim = ANIM_UNDEFINED;
			begin_anim();
		} else {
			cycle_anim();
		}
	}

	WiFiClient client = server.available();
	if (client) {
		enum request_type request = serve(client);
		if (request == REQUEST_RESET)
			doomed = true;
	} else {
		yield();
	}

	// client is flushed and disconnected when it goes out of scope
}
