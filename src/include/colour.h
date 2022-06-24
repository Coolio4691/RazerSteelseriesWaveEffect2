#ifndef __COLOUR_H__
#define __COLOUR_H__

#include <stdint.h>
#include <math.h>

struct hsv {
    double hue;
    double saturation;
    double value;
};

struct rgb hsv_to_rgb(struct hsv hsv) {
	// create rgb values variable
    double r = 0, g = 0, b = 0;

	// if saturation is 0 set rgb to value
	if (hsv.saturation == 0) {
		r = hsv.value;
		g = hsv.value;
		b = hsv.value;
	} // else if saturation > 0
	else {
		int i;
		double f, p, q, t;

		if (hsv.hue == 360) hsv.hue = 0; // if hsv hue == 360 set hsv hue to 0
        else hsv.hue = hsv.hue / 60; // set hsv hue to hsv hue / 60

		i = (int)trunc(hsv.hue);
		f = hsv.hue - i;

		p = hsv.value * (1.0 - hsv.saturation);
		q = hsv.value * (1.0 - (hsv.saturation * f));
		t = hsv.value * (1.0 - (hsv.saturation * (1.0 - f)));

		// check if i is 0, 1, 2, 3, 4 or default to
		switch (i) {
		case 0:
			// set r to hsv value
			r = hsv.value;
			// set g to t
			g = t;
			// set b to p
			b = p;
			break;
		case 1:
			// set r to q
			r = q;
			// set g to hsv value
			g = hsv.value;
			// set b to p
			b = p;
			break;
		case 2:
			// set r to p
			r = p;
			// set g to hsv value
			g = hsv.value;
			// set b to t
			b = t;
			break;
		case 3:
			// set r to p
			r = p;
			// set g to q
			g = q;
			// set b to hsv value
			b = hsv.value;
			break;
		case 4:
			// set r to t
			r = t;
			// set g to p
			g = p;
			// set b to hsv value
			b = hsv.value;
			break;
		default:
			// set r to hsv value
			r = hsv.value;
			// set g to p
			g = p;
			// set b to q
			b = q;
			break;
		}

	}

	// create rgb struct
	struct rgb rgb;
	// set red to r * 255
	rgb.red = r * 255;
	// set green to g * 255
	rgb.green = g * 255;
	// set blue to b * 255
	rgb.blue = b * 255;

	return rgb;
}

#endif