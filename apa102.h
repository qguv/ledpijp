#pragma once

#include "config.h"

#include <stdint.h>	/* uint8_t */
#include <stddef.h>	/* size_t */

#define APA102_HEADER_BYTES 4
#define APA102_LED_BYTES (4 * (LEDS))
#define APA102_FOOTER_BYTES (((LEDS) + 14) / 16)
#define SPI_DATA_BYTES ((APA102_HEADER_BYTES) + (APA102_LED_BYTES) + (APA102_FOOTER_BYTES))

#define BRIGHTEST 0x1f

int led_setup(void);
int led_teardown(void);
void led_solid(uint8_t red, uint8_t green, uint8_t blue, uint8_t *spi_data);
void led_rainbow(double start_hue, double end_hue, uint8_t *spi_data);
void led_debug(uint8_t *spi_data);
int led_blit(uint8_t *spi_data, size_t frames);
