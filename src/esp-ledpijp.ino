#include "FastLED.h"

// FastLED "100-lines-of-code" demo reel, showing just a few
// of the kinds of animation patterns you can quickly and easily
// compose using FastLED.
//
// This example also shows one easy way to define multiple
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    13
#define CLK_PIN   14
#define LED_TYPE    APA102
#define COLOR_ORDER RGB
#define NUM_LEDS    2
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  144

void setup() {
	delay(3000); // 3 second delay for recovery

	// tell FastLED about the LED strip configuration
	FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

	// set master brightness control
	FastLED.setBrightness(BRIGHTNESS);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, bpm };

// Index number of which pattern is current
uint8_t gCurrentPatternNumber = 0;

// rotating "base color" used by many of the patterns
uint8_t gHue = 0;

void loop()
{
	// Call the current pattern function once, updating the 'leds' array
	gPatterns[gCurrentPatternNumber]();

	// send the 'leds' array out to the actual LED strip
	FastLED.show();
	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / FRAMES_PER_SECOND);

	// do some periodic updates
	EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
	EVERY_N_SECONDS(10) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// add one to the current pattern number, and wrap around at the end
void nextPattern()
{
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

// FastLED's built-in rainbow generator
void rainbow()
{
	fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

// built-in FastLED rainbow, plus some random sparkly glitter
void rainbowWithGlitter()
{
	rainbow();
	addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter) {
		leds[random16(NUM_LEDS)] += CRGB::White;
	}
}

// random colored speckles that blink in and fade smoothly
void confetti()
{
	fadeToBlackBy(leds, NUM_LEDS, 10);
	int pos = random16(NUM_LEDS);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void sinelon()
{
	fadeToBlackBy(leds, NUM_LEDS, 20);
	int pos = beatsin16(13, 0, NUM_LEDS);
	leds[pos] += CHSV(gHue, 255, 192);
}

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void bpm()
{
	uint8_t BeatsPerMinute = 132;
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
	for (int i = 0; i < NUM_LEDS; i++) { //9948
		leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
	}
}
