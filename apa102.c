#include "apa102.h"
#include "spi.h"

#include <math.h>	/* fmod, fabs */
#include <stdint.h>	/* uint8_t */
#include <stdio.h>	/* DEBUG printf putchar */
#include <string.h>	/* uint8_t */

int led_setup(void)
{
	return spi_open_port(SPI_DEVICE);
}

int led_teardown(void)
{
	return spi_close_port(SPI_DEVICE);
}

static void led_header(uint8_t *spi_data)
{
	memset(spi_data, 0, APA102_HEADER_BYTES);
}

static void led_footer(uint8_t *spi_data)
{
	memset(spi_data + APA102_HEADER_BYTES + APA102_LED_BYTES, 0, APA102_FOOTER_BYTES);
}

/* hue [0, 360), sat [0, 1], val [0, 1] */
static void hsv2rgb(double hue, double sat, double val, uint8_t *red, uint8_t *green, uint8_t *blue)
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

void led_solid(uint8_t red, uint8_t green, uint8_t blue, uint8_t *spi_data)
{
	uint8_t brightness = 0xe0 | BRIGHTEST;

	led_header(spi_data);

	/* write each data frame */
	for (int i = 4; i < APA102_HEADER_BYTES + APA102_LED_BYTES; i += 4) {
		spi_data[i] = brightness;
		spi_data[i+1] = blue;
		spi_data[i+2] = green;
		spi_data[i+3] = red;
	}

	led_footer(spi_data);
}

void led_rainbow(double start_hue, double end_hue, uint8_t *spi_data)
{
	uint8_t brightness = 0xe0 | BRIGHTEST;

	double hue_step = (end_hue - start_hue) / LEDS;

	led_header(spi_data);

	uint8_t *a = spi_data + APA102_HEADER_BYTES;
	for (int i = 0; i < LEDS; i++, a += 4) {
		a[0] = brightness;
		hsv2rgb(start_hue + i * hue_step, 1, 1, &a[3], &a[2], &a[1]);
	}

	led_footer(spi_data);
}

void led_debug(uint8_t *spi_data)
{
	for (int i = 0; i < SPI_DATA_BYTES; i++) {
		putchar(i % 20 ? ' ' : '\n');
		printf("%02x", spi_data[i]);
	}
	putchar('\n');
}

int led_blit(uint8_t *spi_data, size_t frames)
{
	return spi_write(SPI_DEVICE, spi_data, SPI_DATA_BYTES * frames);
}
