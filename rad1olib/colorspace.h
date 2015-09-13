/*
 * color conversions
 * - hsl2rgb
 * - rgb2hsl
 * - later yuv?
 *
 * Created: 23.08.2015
 *  Author: MaZderMind (peter@mazdermind.de)
 */

#include "setup.h"

// https://github.com/lewisd32/avr-hsl2rgb/blob/master/hsl2rgb.cpp
void hsl2rgb(uint16_t hue, uint8_t sat, uint8_t lum, uint8_t rgb[3]);
