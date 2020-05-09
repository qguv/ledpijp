#include <Arduino.h> /* DELAY */

#include "animation.h"
#include "apa102.h"
#include "config.h" /* NUM_LEDS */
#include "color.h"

/* rainbow state */
static double rainbow_hue;
static unsigned char cbuf[NUM_LEDS * 3];
static int cbuf_pos;

/* bounce state */
static int bounce_pos;

/* strobe state */
static unsigned long int last_flash;

/* solid color (off, night, white) state */
static unsigned long int anim_start;

/* shared globals */
const char * const animation_names[8] = { "off", "night", "rainbow", "bounce", "cycle", "strobe", "white", "undef" };
enum animation anim = ANIM_RAINBOW;
enum animation new_anim = ANIM_UNDEFINED;
double max_brightness = 1.0L;
double rainbow_density = 3.1L;
double cycle_speed = 0.5L;
double strobe_speed = 1.0L;

static void append_led(unsigned char r, unsigned char g, unsigned char b)
{
	int i = 3 * cbuf_pos;
	cbuf[i++] = r;
	cbuf[i++] = g;
	cbuf[i] = b;

	cbuf_pos++;
	cbuf_pos %= NUM_LEDS;
}

void wifi_ap(void)
{
	for (int i = 0; i < 10; i++) {
		blit_solid_leds(0, 0x10, 0);
		delay(100);
		blit_solid_leds(0, 0, 0x10);
		delay(100);
	}
	blit_solid_leds(0, 0, 0);
	delay(500);
}

void wifi_fail(void)
{
	for (int i = 0; i < 4; i++) {
		blit_solid_leds(0x10, 0, 0);
		delay(50);
		blit_solid_leds(0, 0, 0);
		delay(450);
	}
}

static void bouncer(int i, unsigned char *r, unsigned char *g, unsigned char *b)
{
	if (i == bounce_pos || 2*NUM_LEDS - i - 1 == bounce_pos) {
		*r = *g = *b = 0xff;
	} else {
		*r = *g = *b = 0;
	}
}

void begin_anim(void)
{
	switch (anim) {

	case ANIM_OFF:
		blit_solid_leds(0x00, 0x00, 0x00);
		break;

	case ANIM_BOUNCE:
		blit_solid_leds(0x00, 0x00, 0x00);
		bounce_pos = 0;
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
			append_led(gamma8[r], gamma8[g], gamma8[b]);
		}
		break;

	case ANIM_NIGHT:
		blit_solid_leds(0x04, 0x00, 0x00);
		break;

	case ANIM_WHITE:
		blit_solid_leds(gamma8[(int) (0xff * max_brightness)], gamma8[(int) (0xff * max_brightness)], gamma8[(int) (0xff * max_brightness)]);
		break;

	default:
		break;
	}

	anim_start = millis();
}

void cycle_anim(void)
{
	unsigned char r, g, b;
	unsigned long int t;
	unsigned char brt;

	switch (anim) {

	/* 'static' animations should be periodically restarted */
	case ANIM_OFF:
	case ANIM_NIGHT:
	case ANIM_WHITE:
		if (anim_start + 60000 >= millis())
			begin_anim();
		break;

	case ANIM_CYCLE:
		hsv2rgb(rainbow_hue, 1.0, max_brightness, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + cycle_speed, 360.0);
		blit_solid_leds(gamma8[r], gamma8[g], gamma8[b]);
		break;

	case ANIM_RAINBOW:
		hsv2rgb(rainbow_hue, 1.0, max_brightness, &r, &g, &b);
		rainbow_hue = fmod(rainbow_hue + rainbow_density, 360.0);
		append_led(gamma8[r], gamma8[g], gamma8[b]);
		blit_cbuf_leds(cbuf, cbuf_pos);
		break;

	case ANIM_BOUNCE:
		blit_leds_func(bouncer);
		bounce_pos++;
		bounce_pos %= NUM_LEDS * 2;
		break;

	case ANIM_STROBE:
		t = millis();
		brt = gamma8[(int) (0xff * max_brightness)];

		/* strobe speed is half the frequency in Hz, e.g. 10.0 is 20 Hz strobe */
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
}
