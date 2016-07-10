/*
    PCW image handlers
    Copyright (C) 2000, 2006  John Elliott <jce@seasip.demon.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* Convert a pixel from RGB to B/W, using dithering */

/* 64-element ordered dither array matrix, holds values 63-126 */
static int dither[8][8] =
{
        {        63,  95,  71, 103,  65,  97,  73, 105 },
        {       111,  79, 119,  87, 113,  81, 121,  89 },
        {        75, 107,  67,  99,  77, 109,  69, 101 },
        {       123,  91, 115,  83, 125,  93, 117,  85 },
        {        66,  98,  74, 106,  64,  96,  72, 104 },
        {       114,  82, 122,  90, 112,  80, 120,  88 },
        {        78, 110,  70, 102,  76, 108,  68, 100 },
        {       126,  94, 118,  86, 124,  92, 116,  84 }
};

int mono(int x, int y, int r, int g, int b)
{
	/* RGB -> intensity: NTSC formula, from the comp.graphics FAQ 
         * Using long integers rather than doubles */
	long intensity = (((r * 299L) + (g * 587L) +  (b * 114L)) / 40L) + 6300L;  /* 6300 - 12675 */

	if ((100 * dither[x%8][y%8] + 50) < intensity) return 1;

	return 0;
}
