#include <ESP8266WiFi.h>
#include <math.h>
#include <Arduino.h>

#define PORT 80
#define NUM_LEDS 71
#define DATA_PIN 13
#define CLK_PIN 14
#define WIFI_GIVEUP 10
#define ANIM_HZ 60

#define HZ_TO_MS(HZ) (1000 / (HZ))

const char *ssid = "comet compat";
const char *password = "logicalis";

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

enum animation {
	ANIM_OFF,
	ANIM_NIGHT,
	ANIM_RAINBOW,
	ANIM_CYCLE,
	ANIM_STROBE,
	ANIM_WHITE,
	ANIM_UNDEFINED, // must be last
};

const char * const animation_names[7] = { "off", "night", "rainbow", "cycle", "strobe", "white", "undef" };

enum animation anim, new_anim;

int frame;
double rainbow_hue;
unsigned long int last_flash;
bool doomed = false;

double max_brightness = 1.0L;
double rainbow_density = 3.1L;
double cycle_speed = 0.5L;
double strobe_speed = 1.0L;

unsigned long int next_anim_time;

// rainbow strip only: circular buffer to hold LEDs as they rainbow out
unsigned char cbuf[NUM_LEDS * 3];
int cbuf_pos;

WiFiServer server(PORT);

// respond to 'GET /'
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
		"\n\t\t\t\t\t\t\t\t\t\tspeed.value = +body;"
		"\n\t\t\t\t\t\t\t\t\t\tspeed_container.hidden = false;"
		"\n\t\t\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t\t\t});"
		"\n\t\t\t\t\t\t\t}"
		"\n\t\t\t\t\t\t});"
		"\n\t\t\t\t\t}"
		"\n\t\t\t\t});"
		"\n\t\t\t}"
		"\n\t\t</script>"
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
		"\n\t\t<label>Brightness</label>"
		"\n\t\t<input id='brightness' type='range' onchange='fetch(\"/b/\" + this.value)' min='0' max='1' step='0.01' value='"
	));
	client.print(max_brightness);
	client.print(F("' />"
		"\n\t\t<br />"
		"\n\t\t<div id='speed-container'>"
		"\n\t\t\t<label>Speed</label>"
		"\n\t\t\t<input id='speed' type='range' onchange='fetch(\"/s/\" + this.value)' min='0' max='10' step='0.1' value='"
	));
	client.print(rainbow_density);
	client.print(F("' />"
		"\n\t\t</div>"
		"\n\t\t<br />"
		"\n\t\t<button onclick='fetch(\"/r\")'>reset ledpijp</button>"
		"\n\t</body>"
		"\n</html>"
	));
}

void respond_reset(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nbye!"));
}

void respond_new_anim(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(animation_names[new_anim]);
}

void respond_anim_query(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(animation_names[anim]);
}

void respond_double(WiFiClient client, double x)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(x);
}

void respond_not_found(WiFiClient client)
{
	client.print(F("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nError 404: Not Found"));
}

// get desired animation from request
enum animation get_animation(String request)
{
	if (request.startsWith("GET /a/next "))
		return (enum animation) (((int) anim + 1) % (int) ANIM_UNDEFINED);

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++)
		if (request.startsWith(String("GET /a/") + String(animation_names[i]) + String(" ")))
			return (enum animation) i;
	return ANIM_UNDEFINED;
}

void append_led(unsigned char r, unsigned char g, unsigned char b)
{
	int i = 3 * cbuf_pos;
	cbuf[i++] = r;
	cbuf[i++] = g;
	cbuf[i] = b;

	cbuf_pos++;
	cbuf_pos %= NUM_LEDS;
}

void apa102_header()
{
	for (int _ = 0; _ < 32; _++) {
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 0);
	}
}

void apa102_footer()
{
	for (int _ = 0; _ < (NUM_LEDS / 2); _++) {
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 0);
	}
}

void apa102_sendbyte(unsigned char data)
{
	for (int b = 0; b < 8; b++) {
		int bit = (data >> (7 - b)) & 1;
		digitalWrite(DATA_PIN, bit);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, bit);
		digitalWrite(CLK_PIN, 0);
	}
}

void blit_cbuf_leds()
{
	apa102_header();
	for (int i = 0; i < NUM_LEDS; i++) {
		int red_i = ((cbuf_pos + i) % NUM_LEDS) * 3;
		apa102_sendbyte(0xff);
		apa102_sendbyte(cbuf[red_i + 2]);
		apa102_sendbyte(cbuf[red_i + 1]);
		apa102_sendbyte(cbuf[red_i]);
	}
	apa102_footer();
}

void blit_solid_leds(unsigned char r, unsigned char g, unsigned char b)
{
	apa102_header();
	for (int i = 0; i < NUM_LEDS; i++) {
		apa102_sendbyte(0xff);
		apa102_sendbyte(b);
		apa102_sendbyte(g);
		apa102_sendbyte(r);
	}
	apa102_footer();
}

/* hue [0, 360), sat [0, 1], val [0, 1], rgb [0, 255] */
static void hsv2rgb(double hue, double sat, double val, unsigned char *red, unsigned char *green, unsigned char *blue)
{
	double chroma = val * sat;
	double x = chroma * (1.0 - fabs(fmod(hue / 60.0, 2.0) - 1.0));
	double m = val - chroma;

	if (hue < 60.0) {
		*red	= 255.0 * (chroma + m);
		*green	= 255.0 * (x + m);
		*blue	= 255.0 * m;
	} else if (hue < 120.0) {
		*red	= 255.0 * (x + m);
		*green	= 255.0 * (chroma + m);
		*blue	= 255.0 * m;
	} else if (hue < 180.0) {
		*red	= 255.0 * m;
		*green	= 255.0 * (chroma + m);
		*blue	= 255.0 * (x + m);
	} else if (hue < 240.0) {
		*red	= 255.0 * m;
		*green	= 255.0 * (x + m);
		*blue	= 255.0 * (chroma + m);
	} else if (hue < 300.0) {
		*red	= 255.0 * (x + m);
		*green	= 255.0 * m;
		*blue	= 255.0 * (chroma + m);
	} else {
		*red	= 255.0 * (chroma + m);
		*green	= 255.0 * m;
		*blue	= 255.0 * (x + m);
	}
}

void begin_anim()
{
	frame = 0;

	switch (anim) {

	case ANIM_OFF:
		blit_solid_leds(0x00, 0x00, 0x00);
		break;

	case ANIM_STROBE:
	case ANIM_CYCLE:
		break;

	case ANIM_RAINBOW:
		cbuf_pos = 0;
		unsigned char r, g, b;
		for (int i = 0; i < NUM_LEDS; i++) {
			hsv2rgb(rainbow_hue, 1.0, max_brightness, &r, &g, &b);
			rainbow_hue = fmod(rainbow_hue + rainbow_density, 360.0);
			append_led(r, g, b);
		}
		break;

	case ANIM_NIGHT:
		blit_solid_leds(0x04, 0x00, 0x00);
		break;

	case ANIM_WHITE:
		blit_solid_leds(0xff, 0xff, 0xff);
		break;

	default:
		break;
	}
}

void cycle_anim()
{
	unsigned char r, g, b;
	unsigned long int t;
	unsigned char brt;

	switch (anim) {

	// 'static' animations should be periodically restarted
	case ANIM_OFF:
	case ANIM_NIGHT:
	case ANIM_WHITE:
		if (frame > 10000)
			begin_anim();
		break;

	case ANIM_CYCLE:
		hsv2rgb(rainbow_hue, 1.0, max_brightness, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + cycle_speed, 360.0);
		blit_solid_leds(r, g, b);
		break;

	case ANIM_RAINBOW:
		hsv2rgb(rainbow_hue, 1.0, max_brightness, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + rainbow_density, 360.0);
		append_led(r, g, b);
		blit_cbuf_leds();
		break;

	case ANIM_STROBE:
		t = millis();
		brt = 0xff * max_brightness;
		// strobe speed is half the frequency in Hz, e.g. 10.0 is 20 Hz strobe
		if (t - last_flash > 500 / strobe_speed) {
			last_flash = t;
			blit_solid_leds(brt, brt, brt);
		} else if (t - last_flash > 10) {
			blit_solid_leds(0, 0, 0);
		} else {
			blit_solid_leds(brt, brt, brt);
		}
		break;

	default:
		break;
	}

	frame++;
}

double above_zero(double x)
{
	if (x < 0.0L)
		return 0.0L;
	else
		return x;
}

double zero_to_one(double x)
{
	if (x < 0.0L)
		return 0.0L;
	else if (x > 1.0L)
		return 1.0L;
	else
		return x;
}

void serve(WiFiClient client)
{
	client.setTimeout(5000); // default is 1000

	enum request_type req_type = REQUEST_UNDEFINED;

	// Read the first line of the request
	String request = client.readStringUntil('\r');

	if (request.startsWith("GET / ")) {
		req_type = REQUEST_INDEX;

	} else if (request.startsWith("GET /r ")) {
		req_type = REQUEST_RESET;
		doomed = true;

	} else if (request.startsWith("GET /a ")) {
		req_type = REQUEST_ANIM_QUERY;

	} else if (request.startsWith("GET /b ")) {
		req_type = REQUEST_BRIGHTNESS_QUERY;

	} else if (request.startsWith("GET /s ")) {
		req_type = REQUEST_SPEED_QUERY;

	} else if (request.startsWith("GET /b/up ")) {
		req_type = REQUEST_BRIGHTNESS;
		max_brightness = zero_to_one(max_brightness + 0.1L);

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
		enum animation anim_request = get_animation(request);
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
		respond_double(client, max_brightness);
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
}

void wifi_connecting() {
	blit_solid_leds(0x10, 0x10, 0x10);
	delay(50);
	blit_solid_leds(0, 0, 0);
	delay(125);
}

void wifi_ap() {
	for (int i = 0; i < 10; i++) {
		blit_solid_leds(0, 0x10, 0);
		delay(100);
		blit_solid_leds(0, 0, 0x10);
		delay(100);
	}
	blit_solid_leds(0, 0, 0);
	delay(500);
}

void wifi_fail() {
	blit_solid_leds(0x10, 0, 0);
	delay(50);
	blit_solid_leds(0, 0, 0);
	delay(450);
}

void setup()
{
	next_anim_time = 0;
	anim = new_anim = ANIM_RAINBOW;

	pinMode(CLK_PIN, OUTPUT);
	digitalWrite(CLK_PIN, 0);

	pinMode(DATA_PIN, OUTPUT);
	digitalWrite(DATA_PIN, 0);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	for (int i = 0; (WiFi.status() != WL_CONNECTED) && i < (WIFI_GIVEUP * 1000 / (170)); i++)
		wifi_connecting();

	// couldn't connect to network; create one instead
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.mode(WIFI_AP);
		boolean ok = WiFi.softAP("ledpijp", "logicalis", 1, false, 8);
		if (ok) {
			wifi_ap();
		} else {
			for (;;) {
				wifi_fail();
			}
		}
	}

	server.begin();
}

void loop()
{
	if (doomed)
		ESP.reset();

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
		serve(client);
	} else {
		yield();
	}

	// client is flushed and disconnected when it goes out of scope
}
