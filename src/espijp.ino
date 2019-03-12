#include <Arduino.h>

#define APA102_DATA  13
#define APA102_CLOCK 14

#define NUM_LEDS 2
#define NUM_COLORS 9

// blue-green-red
unsigned int colors[NUM_COLORS] = { 0xff000010, 0xff000810, 0xff001008, 0xff001000, 0xff081000, 0xff100800, 0xff100000, 0xff100008, 0xff080010 };

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("Hello, world!");

	pinMode(APA102_CLOCK, OUTPUT);
	digitalWrite(APA102_CLOCK, 0);

	pinMode(APA102_DATA, OUTPUT);
	digitalWrite(APA102_DATA, 0);
}

void loop()
{
	static int frame = 0;
	unsigned int color = colors[frame];

	// prologue
	for (int i = 0; i < 32; i++) {
		digitalWrite(APA102_DATA, 0);
		digitalWrite(APA102_CLOCK, 1);
		digitalWrite(APA102_DATA, 0);
		digitalWrite(APA102_CLOCK, 0);
	}

	// color frames
	for (int l = 0; l < NUM_LEDS; l++) {
		for (int b = 0; b < 32; b++) {
			int bit = (color >> (31 - b)) & 1;
			digitalWrite(APA102_DATA, bit);
			digitalWrite(APA102_CLOCK, 1);
			digitalWrite(APA102_DATA, bit);
			digitalWrite(APA102_CLOCK, 0);
		}
	}

	// epilogue
	for (int i = 0; i < (NUM_LEDS / 2); i++) {
		digitalWrite(APA102_DATA, 0);
		digitalWrite(APA102_CLOCK, 1);
		digitalWrite(APA102_DATA, 0);
		digitalWrite(APA102_CLOCK, 0);
	}

	frame = (frame + 1) % NUM_COLORS;

	delay(500);
}
