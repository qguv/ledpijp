#include <math.h> /* fmod, fabs */

#include "color.h"

/* hue [0, 360), sat [0, 1], val [0, 1], rgb [0, 255] */
void hsv2rgb(double hue, double sat, double val, unsigned char *red, unsigned char *green, unsigned char *blue)
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
