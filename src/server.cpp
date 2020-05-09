#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "server.h"
#include "animation.h"
#include "color.h"
#include "validate.h"

static void respond_reset(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nbye!"));
}

static void respond_new_anim(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(animation_names[new_anim]);
}

static void respond_anim_query(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(animation_names[anim]);
}

static void respond_double(WiFiClient client, double x)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(x);
}

static void respond_not_found(WiFiClient client)
{
	client.print(F("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nError 404: Not Found"));
}

static enum animation parse_animation_request(String request)
{
	if (request.startsWith("GET /a/next "))
		return (enum animation) (((int) anim + 1) % (int) ANIM_UNDEFINED);

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++)
		if (request.startsWith(String("GET /a/") + String(animation_names[i]) + String(" ")))
			return (enum animation) i;
	return ANIM_UNDEFINED;
}

/* returns new animation if necessary */
enum request_type serve(WiFiClient client)
{
	client.setTimeout(5000); // default is 1000

	new_anim = ANIM_UNDEFINED;
	enum request_type req_type = REQUEST_UNDEFINED;

	// Read the first line of the request
	String request = client.readStringUntil('\r');

	if (request.startsWith("GET / ")) {
		req_type = REQUEST_INDEX;

	} else if (request.startsWith("GET /r ")) {
		req_type = REQUEST_RESET;

	} else if (request.startsWith("GET /a ")) {
		req_type = REQUEST_ANIM_QUERY;

	} else if (request.startsWith("GET /b ")) {
		req_type = REQUEST_BRIGHTNESS_QUERY;

	} else if (request.startsWith("GET /s ")) {
		req_type = REQUEST_SPEED_QUERY;

	} else if (request.startsWith("GET /b/up ")) {
		req_type = REQUEST_BRIGHTNESS;
		max_brightness = zero_to_one(max_brightness + 0.1L);

	} else if (request.startsWith("GET /b/down ")) {
		req_type = REQUEST_BRIGHTNESS;
		max_brightness = zero_to_one(max_brightness - 0.1L);

	} else if (request.startsWith("GET /s/up ")) {
		if (anim == ANIM_RAINBOW) {
			req_type = REQUEST_SPEED;
			rainbow_density += 0.1L;
		} else if (anim == ANIM_CYCLE) {
			req_type = REQUEST_SPEED;
			cycle_speed += 0.1L;
		} else if (anim == ANIM_STROBE) {
			req_type = REQUEST_SPEED;
			strobe_speed += 0.1L;
		}

	} else if (request.startsWith("GET /s/down ")) {
		if (anim == ANIM_RAINBOW) {
			req_type = REQUEST_SPEED;
			rainbow_density = above_zero(rainbow_density - 0.1L);
		} else if (anim == ANIM_CYCLE) {
			req_type = REQUEST_SPEED;
			cycle_speed = above_zero(cycle_speed - 0.1L);
		} else if (anim == ANIM_STROBE) {
			req_type = REQUEST_SPEED;
			strobe_speed = above_zero(strobe_speed - 0.1L);
		}

	} else if (request.startsWith("GET /b/")) {
		int from = 7;
		int to = request.indexOf(" ", from);
		if (to != -1) {
			// yes, invalid numbers will set brightness to zero
			// you could do a cutsey thing and GET /b/dark but don't
			max_brightness = zero_to_one(request.substring(from, to).toFloat());
			req_type = REQUEST_BRIGHTNESS;
		}

	} else if (request.startsWith("GET /s/")) {
		if (anim == ANIM_RAINBOW || anim == ANIM_CYCLE || anim == ANIM_STROBE) {
			int from = 7;
			int to = request.indexOf(" ", from);
			if (to != -1) {
				// yes, invalid numbers will set speed to zero
				// you could do a cutsey thing and GET /s/pause but don't
				double newval = above_zero(request.substring(from, to).toFloat());
				if (anim == ANIM_RAINBOW) {
					rainbow_density = newval;
				} else if (anim == ANIM_CYCLE) {
					cycle_speed = newval;
				} else if (anim == ANIM_STROBE) {
					strobe_speed = newval;
				}

				req_type = REQUEST_SPEED;
			}
		}

	} else if (request.startsWith("GET /a/")) {
		enum animation anim_request = parse_animation_request(request);
		if (anim_request != ANIM_UNDEFINED) {
			new_anim = anim_request;
			req_type = REQUEST_ANIM;
		}
	}

	// throw away the rest of the request
	while (client.available())
		client.read();

	switch (req_type) {

	case REQUEST_INDEX:
		respond_index(client);
		break;

	case REQUEST_RESET:
		respond_reset(client);
		break;

	case REQUEST_ANIM_QUERY:
		respond_anim_query(client);
		break;

	case REQUEST_ANIM:
		respond_new_anim(client);
		break;

	case REQUEST_BRIGHTNESS:
	case REQUEST_BRIGHTNESS_QUERY:
		respond_double(
			client,
			anim == ANIM_OFF || anim == ANIM_NIGHT || anim == ANIM_MORNING || anim == ANIM_BOUNCE
				? -1.0L
				: max_brightness
		);
		break;

	case REQUEST_SPEED:
	case REQUEST_SPEED_QUERY:
		respond_double(client, anim == ANIM_RAINBOW ? rainbow_density : anim == ANIM_CYCLE ? cycle_speed : anim == ANIM_STROBE ? strobe_speed : -1.0L);
		break;

	case REQUEST_UNDEFINED:
		respond_not_found(client);
		break;

	default:
		respond_not_found(client);
		break;
	}

	return req_type;
}

/* respond to 'GET /' */
void respond_index(WiFiClient client)
{
	client.print(F(
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<!doctype html>"
		"\n<html>"
		"\n\t<head>"
		"\n\t\t<meta charset='utf-8' />"
		"\n\t\t<title>ledpijp</title>"
		"\n\t\t<script>"
		"\n\t\t\tfunction set_led(val) {"
		"\n\t\t\t\tfetch('/a/' + val).then(res => {"
		"\n\t\t\t\t\tif (res.ok) {"
		"\n\t\t\t\t\t\tres.text().then(body => document.querySelector('h1').innerHTML = body);"
		"\n\t\t\t\t\t\tfetch('/s').then(res => {"
		"\n\t\t\t\t\t\t\tif (res.ok) {"
		"\n\t\t\t\t\t\t\t\tres.text().then(body => {"
		"\n\t\t\t\t\t\t\t\t\tconst speed = document.querySelector('#speed');"
		"\n\t\t\t\t\t\t\t\t\tconst speed_container = document.querySelector('#speed-container');"
		"\n\t\t\t\t\t\t\t\t\tif (body === '-1.00') {"
		"\n\t\t\t\t\t\t\t\t\t\tspeed_container.hidden = true;"
		"\n\t\t\t\t\t\t\t\t\t} else {"
		"\n\t\t\t\t\t\t\t\t\t\tspeed.value = Math.log10(body);"
		"\n\t\t\t\t\t\t\t\t\t\tspeed_container.hidden = false;"
		"\n\t\t\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t\t\t});"
		"\n\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t});"
		"\n\t\t\t\t\t\tfetch('/b').then(res => {"
		"\n\t\t\t\t\t\t\tif (res.ok) {"
		"\n\t\t\t\t\t\t\t\tres.text().then(body => {"
		"\n\t\t\t\t\t\t\t\t\tconst brightness = document.querySelector('#brightness');"
		"\n\t\t\t\t\t\t\t\t\tconst brightness_container = document.querySelector('#brightness-container');"
		"\n\t\t\t\t\t\t\t\t\tif (body === '-1.00') {"
		"\n\t\t\t\t\t\t\t\t\t\tbrightness_container.hidden = true;"
		"\n\t\t\t\t\t\t\t\t\t} else {"
		"\n\t\t\t\t\t\t\t\t\t\tbrightness.value = Math.log10(body);"
		"\n\t\t\t\t\t\t\t\t\t\tbrightness_container.hidden = false;"
		"\n\t\t\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t\t\t});"
		"\n\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t});"
		"\n\t\t\t\t\t}"
		"\n\t\t\t\t});"
		"\n\t\t\t}"
		"\n\t\t</script>"
		"\n\t\t<style>"
		"\n\t\t\t* {"
		"\n\t\t\t\tbackground-color: black;"
		"\n\t\t\t\tcolor: white;"
		"\n\t\t\t}"
		"\n\t\t</style>"
		"\n\t</head>"
		"\n\t<body>"
		"\n\t\t<h1>"
	));
	client.print(animation_names[(int) anim]);
	client.print(F("</h1>"
		"\n\t\t<ul>"
	));

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++) {
		client.print(F("\n\t\t\t<li><a href='#' onclick='set_led(\""));
		client.print(animation_names[i]);
		client.print(F("\");'>"));
		client.print(animation_names[i]);
		client.print(F("</a></li>"));
	}
	client.print(F(
		"\n\t\t\t<li><a href='#' onclick='set_led(\"next\")'>next</a></li>"
		"\n\t\t</ul>"
		"\n\t\t<div id='brightness-container'>"
		"\n\t\t\t<label>Brightness</label>"
		"\n\t\t\t<input id='brightness' type='range' onchange='fetch(\"/b/\" + Math.pow(10, this.value))' min='-2' max='0' step='0.1' value='"
	));
	if (max_brightness > 0) {
		client.print(log10(max_brightness));
	} else {
		client.print("-2");
	}
	client.print(F("' />"
		"\n\t\t</div>"
		"\n\t\t<br />"
		"\n\t\t<div id='speed-container'>"
		"\n\t\t\t<label>Speed</label>"
		"\n\t\t\t<input id='speed' type='range' onchange='fetch(\"/s/\" + Math.pow(10, this.value))' min='-1' max='1' step='0.1' value='"
	));
	if (anim == ANIM_RAINBOW && rainbow_density > 0) {
		client.print(log10(rainbow_density));
	} else if (anim == ANIM_CYCLE && cycle_speed > 0) {
		client.print(log10(cycle_speed));
	} else if (anim == ANIM_STROBE && strobe_speed > 0) {
		client.print(log10(strobe_speed));
	} else {
		client.print("-1");
	}
	client.print(F("' />"
		"\n\t\t</div>"
		"\n\t\t<br />"
		"\n\t\t<button onclick='fetch(\"/r\")'>reset ledpijp</button>"
		"\n\t\t<br />"
		"\n\t\t<br />"
		"\n\t\t<a href='https://github.com/qguv/ledpijp' target='_blank'>source</a>"
		"\n\t</body>"
		"\n</html>"
	));
}
