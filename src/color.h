#pragma once

extern const unsigned char gamma8[0x100];

/* hue [0, 360), sat [0, 1], val [0, 1], rgb [0, 255] */
void hsv2rgb(double hue, double sat, double val, unsigned char *red, unsigned char *green, unsigned char *blue);
