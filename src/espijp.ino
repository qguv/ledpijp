#include <ESP8266WiFi.h>
#include <Arduino.h>

#define PORT 80
#define NUM_LEDS 71

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
	ANIM_UNDEFINED, // must be last
};

const char * const animation_names[5] = { "off", "cycle", "rainbow", "night", "undef" };

enum animation anim, new_anim;

int frame;

// rainbow strip only: circular buffer to hold LEDs as they rainbow out
unsigned char cbuf[NUM_LEDS * 3];
int cbuf_pos;

WiFiServer server(PORT);

// respond to 'GET /'
void respond_index(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body><h1>"));
	client.print(animation_names[(int) anim]);
	client.print(F(" ("));
	client.print((int) anim);
	client.print(F(")"));
	client.print(F("</h1><ul>"));

	for (int i = 0; i < (int) ANIM_UNDEFINED; i++) {
		if (i == (int) anim) {
			client.print(F("<li style='font-style: italic;'>"));
			client.print(animation_names[i]);
			client.print(F("</li>"));
		} else {
			client.print(F("<li><a href='http://"));
			client.print(WiFi.localIP());
			client.print(F("/"));
			client.print(animation_names[i]);
			client.print(F("'>"));
			client.print(animation_names[i]);
			client.print(F("</a></li>"));
		}
	}
	client.print(F("</ul></body></html>"));
}

void respond_ok(WiFiClient client)
{
	client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK"));
}

void respond_not_found(WiFiClient client)
{
	client.print(F("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nError 404: Not Found"));
}

// get desired animation from request
enum animation get_animation(String request)
{
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

void blit_cbuf_leds()
{
	// TODO blit
}

void blit_solid_leds(unsigned char r, unsigned char g, unsigned char b)
{
	// TODO blit
}

/* hue [0, 360), sat [0, 1], val [0, 1], rgb [0, 255] */
static void hsv2rgb(double hue, double sat, double val, unsigned char *red, unsigned char *green, unsigned char *blue)
{
	hue = fmod(hue, 360);
	double chroma = val * sat;
	double x = chroma * (1.0 - fabs(fmod(hue / 60.0, 2) - 1.0));
	double m = val - chroma;

	if (hue < 60) {
		*red	= 0xff * (chroma + m);
		*green	= 0xff * (x + m);
		*blue	= 0xff * m;
	} else if (hue < 120) {
		*red	= 0xff * (x + m);
		*green	= 0xff * (chroma + m);
		*blue	= 0xff * m;
	} else if (hue < 180) {
		*red	= 0xff * m;
		*green	= 0xff * (chroma + m);
		*blue	= 0xff * (x + m);
	} else if (hue < 240) {
		*red	= 0xff * m;
		*green	= 0xff * (x + m);
		*blue	= 0xff * (chroma + m);
	} else if (hue < 300) {
		*red	= 0xff * (x + m);
		*green	= 0xff * m;
		*blue	= 0xff * (chroma + m);
	} else {
		*red	= 0xff * (chroma + m);
		*green	= 0xff * m;
		*blue	= 0xff * (x + m);
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
		for (; frame < NUM_LEDS; frame++) {
			hsv2rgb(frame, 1, 1, &r, &g, &b);
			append_led(r, g, b);
		}
		break;

	case ANIM_NIGHT:
		blit_solid_leds(0x04, 0x00, 0x00);
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
		if (frame > 10000)
			begin_anim();
		break;

	case ANIM_RAINBOW:
		hsv2rgb(frame, 1, 1, &r, &g, &b);
		append_led(r, g, b);
		break;

	case ANIM_CYCLE:
		hsv2rgb(frame, 1, 1, &r, &g, &b);
		blit_solid_leds(r, g, b);
		break;

	default:
		break;
	}

	frame++;
}

void setup()
{
	Serial.begin(115200);
	Serial.println();

	anim = new_anim = ANIM_RAINBOW;

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, 0);

	Serial.print(F("Connecting to "));
	Serial.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(F("."));
		digitalWrite(LED_BUILTIN, 1);
		delay(50);
		digitalWrite(LED_BUILTIN, 0);
		delay(125);
	}
	digitalWrite(LED_BUILTIN, 1);
	Serial.println(F(" OK"));

	server.begin();
	Serial.print(F("Listening at http://"));
	Serial.println(WiFi.localIP());
}

void loop()
{
	if (new_anim != ANIM_UNDEFINED) {
		anim = new_anim;
		begin_anim();
	}

	cycle_anim();

	WiFiClient client = server.available();
	if (!client)
		return;
	client.setTimeout(5000); // default is 1000

	enum request_type req_type = REQUEST_UNDEFINED;

	digitalWrite(LED_BUILTIN, 0);
	Serial.println(F("client connected"));

	// Read the first line of the request
	String request = client.readStringUntil('\r');
	Serial.println(F("request: "));
	Serial.println(request);

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
		Serial.print(F("client got index"));
		respond_index(client);
		break;

	case REQUEST_NEW_ANIM:
		Serial.print(F("client is switching animation from "));
		Serial.print(animation_names[(int) anim]);
		Serial.print(F(" to "));
		Serial.print(animation_names[(int) new_anim]);
		respond_ok(client);
		break;

	case REQUEST_UNDEFINED:
		Serial.println(F("bad request"));
		respond_not_found(client);
		break;

	default:
		Serial.print(F("unimplemented request of type "));
		Serial.println(req_type);
		respond_not_found(client);
		break;
	}

	// client is flushed and disconnected when it goes out of scope
	Serial.println(F("dropping connection"));
	digitalWrite(LED_BUILTIN, 1);
}
