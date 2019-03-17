#include <ESP8266WiFi.h>
#include <math.h>
#include <Arduino.h>

#define PORT 80
#define NUM_LEDS 71
#define DATA_PIN 13
#define CLK_PIN 14
#define WIFI_GIVEUP 10
#define MAX_BRIGHTNESS 1.0L
#define RAINBOW_DENSITY 3.1L
#define ANIM_HZ 60

#define HZ_TO_MS(HZ) (1000 / (HZ))

const char *ssid = "comet compat";
const char *password = "logicalis";

enum request_type {
	REQUEST_INDEX,
	REQUEST_NEW_ANIM,
	REQUEST_UNDEFINED, // must be last
};

enum animation {
	ANIM_OFF,
	ANIM_CYCLE,
	ANIM_RAINBOW,
	ANIM_NIGHT,
	ANIM_WHITE,
	ANIM_UNDEFINED, // must be last
};

const char * const animation_names[6] = { "off", "cycle", "rainbow", "night", "white", "undef" };

enum animation anim, new_anim;

int frame;

double rainbow_hue;

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
		"<!doctype html>\n"
		"<html>\n"
		"\t<head>\n"
		"\t\t<meta charset='utf-8' />\n"
		"\t\t<title>ledpijp</title>\n"
		"\t\t<script>\n"
		"\t\t\tfunction set_led(val) {\n"
		"\t\t\t\tfetch('/' + val).then(res => {\n"
		"\t\t\t\t\tif (res.ok)\n"
		"\t\t\t\t\t\tres.text().then(body => document.querySelector('h1').innerHTML = body);\n"
		"\t\t\t\t});\n"
		"\t\t\t}\n"
		"\t\t</script>\n"
		"\t</head>\n"
		"\t<body>\n"
		"\t\t<h1>\n"
	));
	client.print(animation_names[(int) anim]);
	client.print(F("</h1>\n\t\t<ul>"));

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++) {
		client.print(F("\n\t\t\t<li><a href='#' onclick='set_led(\""));
		client.print(animation_names[i]);
		client.print(F("\");'>"));
		client.print(animation_names[i]);
		client.print(F("</a></li>"));
	}
	client.print(F("\n\t\t\t<li><a href='#' onclick='set_led(\"next\")'>next</a></li>"));
	client.print(F("\n\t\t</ul>\n\t</body>\n</html>"));
}

void respond_new_anim(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
	client.print(animation_names[new_anim]);
}

void respond_not_found(WiFiClient client)
{
	client.print(F("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nError 404: Not Found"));
}

// get desired animation from request
enum animation get_animation(String request)
{
	if (request.startsWith("GET /next "))
		return (enum animation) (((int) anim + 1) % (int) ANIM_UNDEFINED);

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++)
		if (request.startsWith(String("GET /") + String(animation_names[i]) + String(" ")))
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

	case ANIM_CYCLE:
		break;

	case ANIM_RAINBOW:
		cbuf_pos = 0;
		unsigned char r, g, b;
		for (int i = 0; i < NUM_LEDS; i++) {
			hsv2rgb(rainbow_hue, 1.0, MAX_BRIGHTNESS, &r, &g, &b);
			rainbow_hue = fmod(rainbow_hue + RAINBOW_DENSITY, 360.0);
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

	switch (anim) {

	// 'static' animations should be periodically restarted
	case ANIM_OFF:
	case ANIM_NIGHT:
	case ANIM_WHITE:
		if (frame > 10000)
			begin_anim();
		break;

	case ANIM_CYCLE:
		hsv2rgb(rainbow_hue, 1.0, MAX_BRIGHTNESS, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + RAINBOW_DENSITY, 360.0);
		blit_solid_leds(r, g, b);
		break;

	case ANIM_RAINBOW:
		hsv2rgb(rainbow_hue, 1.0, MAX_BRIGHTNESS, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + RAINBOW_DENSITY, 360.0);
		append_led(r, g, b);
		blit_cbuf_leds();
		break;

	default:
		break;
	}

	frame++;
}

void serve(WiFiClient client)
{
	client.setTimeout(5000); // default is 1000

	enum request_type req_type = REQUEST_UNDEFINED;

	// Read the first line of the request
	String request = client.readStringUntil('\r');

	if (request.startsWith("GET / ")) {
		req_type = REQUEST_INDEX;
	} else {
		enum animation anim_request = get_animation(request);
		if (anim_request != ANIM_UNDEFINED) {
			new_anim = anim_request;
			req_type = REQUEST_NEW_ANIM;
		}
	}

	// throw away the rest of the request
	while (client.available())
		client.read();

	switch (req_type) {

	case REQUEST_INDEX:
		respond_index(client);
		break;

	case REQUEST_NEW_ANIM:
		respond_new_anim(client);
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
