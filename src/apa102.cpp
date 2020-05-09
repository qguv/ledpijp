#include <Arduino.h> /* digitalWrite */

#include "apa102.h"

static void header(void)
{
	for (int _ = 0; _ < 32; _++) {
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 0);
	}
}

static void footer(void)
{
	for (int _ = 0; _ < (NUM_LEDS / 2); _++) {
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, 0);
		digitalWrite(CLK_PIN, 0);
	}
}

static void sendbyte(unsigned char data)
{
	for (int b = 0; b < 8; b++) {
		int bit = (data >> (7 - b)) & 1;
		digitalWrite(DATA_PIN, bit);
		digitalWrite(CLK_PIN, 1);
		digitalWrite(DATA_PIN, bit);
		digitalWrite(CLK_PIN, 0);
	}
}

void blit_cbuf_leds(unsigned char cbuf[NUM_LEDS * 3], int cbuf_pos)
{
	header();
	for (int i = 0; i < NUM_LEDS; i++) {
		int red_i = ((cbuf_pos + i) % NUM_LEDS) * 3;
		sendbyte(0xff);
		sendbyte(cbuf[red_i + 2]);
		sendbyte(cbuf[red_i + 1]);
		sendbyte(cbuf[red_i]);
	}
	footer();
}

void blit_solid_leds(unsigned char r, unsigned char g, unsigned char b)
{
	header();
	for (int i = 0; i < NUM_LEDS; i++) {
		sendbyte(0xff);
		sendbyte(b);
		sendbyte(g);
		sendbyte(r);
	}
	footer();
}

void blit_leds_func(void (*fn)(int i, unsigned char *r, unsigned char *g, unsigned char *b))
{
	unsigned char r, g, b;
	header();
	for (int i = 0; i < NUM_LEDS; i++) {
		fn(i, &r, &g, &b);
		sendbyte(0xff);
		sendbyte(b);
		sendbyte(g);
		sendbyte(r);
	}
	footer();
}
