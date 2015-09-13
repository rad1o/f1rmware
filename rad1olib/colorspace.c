/*
 * color conversions
 * - hsl2rgb
 * - rgb2hsl
 * - later yuv?
 *
 * Created: 23.08.2015
 *  Author: MaZderMind (peter@mazdermind.de)
 */

#include "colorspace.h"

// https://github.com/lewisd32/avr-hsl2rgb/blob/master/hsl2rgb.cpp (_orig version)
void hsl2rgb(uint16_t hue, uint8_t sat, uint8_t lum, uint8_t rgb[3]) {
	uint16_t r_temp, g_temp, b_temp;
	uint8_t hue_mod;
	uint8_t inverse_sat = (sat ^ 255);

	hue = hue % 768;
	hue_mod = hue % 256;

	if (hue < 256)
	{
		r_temp = hue_mod ^ 255;
		g_temp = hue_mod;
		b_temp = 0;
	}

	else if (hue < 512)
	{
		r_temp = 0;
		g_temp = hue_mod ^ 255;
		b_temp = hue_mod;
	}

	else if ( hue < 768)
	{
		r_temp = hue_mod;
		g_temp = 0;
		b_temp = hue_mod ^ 255;
	}

	else
	{
		r_temp = 0;
		g_temp = 0;
		b_temp = 0;
	}

	r_temp = ((r_temp * sat) / 255) + inverse_sat;
	g_temp = ((g_temp * sat) / 255) + inverse_sat;
	b_temp = ((b_temp * sat) / 255) + inverse_sat;

	r_temp = (r_temp * lum) / 255;
	g_temp = (g_temp * lum) / 255;
	b_temp = (b_temp * lum) / 255;

	rgb[0] 	= (uint8_t)r_temp;
	rgb[1]	= (uint8_t)g_temp;
	rgb[2]	= (uint8_t)b_temp;
}
