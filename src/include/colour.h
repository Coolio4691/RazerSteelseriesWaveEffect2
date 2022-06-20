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
    double r = 0, g = 0, b = 0;

	if (hsv.saturation == 0) {
		r = hsv.value;
		g = hsv.value;
		b = hsv.value;
	}
	else {
		int i;
		double f, p, q, t;

		if (hsv.hue == 360) {
			hsv.hue = 0;
        }
        else {
			hsv.hue = hsv.hue / 60;
        }

		i = (int)trunc(hsv.hue);
		f = hsv.hue - i;

		p = hsv.value * (1.0 - hsv.saturation);
		q = hsv.value * (1.0 - (hsv.saturation * f));
		t = hsv.value * (1.0 - (hsv.saturation * (1.0 - f)));

		switch (i) {
		case 0:
			r = hsv.value;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = hsv.value;
			b = p;
			break;

		case 2:
			r = p;
			g = hsv.value;
			b = t;
			break;

		case 3:
			r = p;
			g = q;
			b = hsv.value;
			break;

		case 4:
			r = t;
			g = p;
			b = hsv.value;
			break;

		default:
			r = hsv.value;
			g = p;
			b = q;
			break;
		}

	}

	struct rgb rgb;
	rgb.red = r * 255;
	rgb.green = g * 255;
	rgb.blue = b * 255;

	return rgb;
}

#endif