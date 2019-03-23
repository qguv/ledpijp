#pragma once

#include "config.h"

void blit_cbuf_leds(unsigned char cbuf[NUM_LEDS * 3], int cbuf_pos);
void blit_solid_leds(unsigned char r, unsigned char g, unsigned char b);
