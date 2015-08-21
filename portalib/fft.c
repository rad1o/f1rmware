/*
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "portapack.h"

#include <stdint.h>
#include <math.h>

/* http://beige.ucs.indiana.edu/B673/node14.html */
/* http://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857?pgno=3 */

void fft_c_preswapped(float* data_preswapped, unsigned long nn)
{
	uint32_t n, mmax, m, j, istep, i;
	float wtemp, wr, wpr, wpi, wi, theta;
	float tempr, tempi;
 
	n = nn<<1;
	mmax=2;
	while( n > mmax ) {
		istep = 2 * mmax;
		theta = -(2.0f * M_PI / mmax);
		wtemp = sinf(0.5f * theta);
		wpr = -2.0f * wtemp * wtemp;
		wpi = sinf(theta);
		wr = 1.0f;
		wi = 0.0f;
		for (m=1; m < mmax; m += 2) {
			for (i=m; i <= n; i += istep) {
				j=i+mmax;

				tempr = wr * data_preswapped[j-1] - wi * data_preswapped[j];
				tempi = wr * data_preswapped[j]   + wi * data_preswapped[j-1];
 
				data_preswapped[j-1]  = data_preswapped[i-1] - tempr;
				data_preswapped[j]    = data_preswapped[i] - tempi;
				data_preswapped[i-1] += tempr;
				data_preswapped[i]   += tempi;
			}
			wtemp=wr;
			wr += wr*wpr - wi*wpi;
			wi += wi*wpr + wtemp*wpi;
		}
		mmax=istep;
	}
}