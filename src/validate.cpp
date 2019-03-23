#include "validate.h"

double above_zero(double x)
{
	if (x < 0.0L)
		return 0.0L;
	else
		return x;
}

double zero_to_one(double x)
{
	if (x < 0.0L)
		return 0.0L;
	else if (x > 1.0L)
		return 1.0L;
	else
		return x;
}
