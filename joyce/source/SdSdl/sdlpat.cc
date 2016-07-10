/*************************************************************************
**	 Copyright 2005, John Elliott                                   **
**       Copyright 1999, Caldera Thin Clients, Inc.                     ** 
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                ** 
**                                                                      ** 
**                  Historical Copyright                                ** 
**                                                                      **
**                                                                      **
**                                                                      **
**  Copyright (c) 1987, Digital Research, Inc. All Rights Reserved.     **
**  The Software Code contained in this listing is proprietary to       **
**  Digital Research, Inc., Monterey, California and is covered by U.S. **
**  and other copyright protection.  Unauthorized copying, adaptation,  **
**  distribution, use or display is prohibited and may be subject to    **
**  civil and criminal penalties.  Disclosure to others is prohibited.  **
**  For the terms and conditions of software code use refer to the      **
**  appropriate Digital Research License Agreement.                     **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"

/* The GEM patterns from HIRESPAT, widened to 32 bits.
 * Some are writeable; in which case, these are just the defaults. */

static Uint32 line_styles[] = 
{
	0xFFFFFFFFL,	/* Copying the 16-bit ones */
	0xFFF0FFF0L,
	0xC0C0C0C0L,
	0xFF18FF18L,
	0xFF00FF00L,
	0xF191F191L,
	0
};

// Fill patterns. These are not writeable.

Uint32 OEMMSKPAT = 7;
Uint32 OEMPAT[] =
{
	0xFFFFFFFF, 0x80808080, 0x80808080, 0x80808080, 
	0xFFFFFFFF, 0x80808080, 0x80808080, 0x80808080,
 // diagonal bricks
	0x20202020, 0x40404040, 0x80808080, 0x41414141,
	0x22222222, 0x14141414, 0x08080808, 0x10101010,
// grass
	0x00000000, 0x00000000, 0x10101010, 0x28282828,
	0x00000000, 0x00000000, 0x01010101, 0x82828282,
// trees
	0x02020202, 0x02020202, 0xAAAAAAAA, 0x50505050,
	0x20202020, 0x20202020, 0xAAAAAAAA, 0x05050505,
// dashed x's
	0x40404040, 0x80808080, 0x00000000, 0x08080808,
	0x04040404, 0x02020202, 0x00000000, 0x20202020,
// cobble stone
	0x66066606, 0xc6c6c6c6, 0xd8d8d8d8, 0x18181818,
	0x81818181, 0x8db18db1, 0x0c330c33, 0x60006000,
// sand
	0x00000000, 0x00000000, 0x04000400, 0x00000000,
	0x00100010, 0x00000000, 0x80008000, 0x00000000,
// rough weave
	0xf8f8f8f8, 0x6c6c6c6c, 0xc6c6c6c6, 0x8f8f8f8f,
	0x1f1f1f1f, 0x36363636, 0x63636363, 0xf1f1f1f1,
// quilt	
	0xAAAAAAAA, 0x00000000, 0x88888888, 0x14141414,
	0x22222222, 0x41414141, 0x88888888, 0x00000000,
// patterned cross
	0x08080808, 0x00000000, 0xAAAAAAAA, 0x00000000,
	0x08080808, 0x00000000, 0x88888888, 0x00000000,
// balls
	0x77777777, 0x98989898, 0xf8f8f8f8, 0xf8f8f8f8,
	0x77777777, 0x89898989, 0x8f8f8f8f, 0x8f8f8f8f,
// vertical scales
	0x80808080, 0x41414141, 0x3e3e3e3e, 0x08080808,
	0x08080808, 0x08080808, 0x14141414, 0xe3e3e3e3,
// diagonal scales
	0x81818181, 0x42424242, 0x24242424, 0x18181818, 
	0x06060606, 0x01010101, 0x80808080, 0x80808080,
// checker board
	0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0,
	0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f,
// filled diamond
	0x08080808, 0x1c1c1c1c, 0x3e3e3e3e, 0x7f7f7f7f,
	0xffffffff, 0x7f7f7f7f, 0x3e3e3e3e, 0x1c1c1c1c,
// herringbone
	0x11111111, 0x22222222, 0x44444444, 0xffffffff,
	0x88888888, 0x44444444, 0x22222222, 0xffffffff,
};

Uint32 DITHRMSK = 3;
Uint32 DITHER[] =
{
	0x00000000, 0x44444444, 0x00000000, 0x11111111,	// Level 2
	0x00000000, 0x55555555, 0x00000000, 0x55555555, // Level 4
	0x88888888, 0x55555555, 0x22222222, 0x55555555, // Level 6
	0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555, // Level 8
	0xAAAAAAAA, 0xDDDDDDDD, 0xAAAAAAAA, 0x77777777, // Level 10
	0xAAAAAAAA, 0xFFFFFFFF, 0xAAAAAAAA, 0xFFFFFFFF, // Level 12
	0xEEEEEEEE, 0xFFFFFFFF, 0xEEEEEEEE, 0xFFFFFFFF, // Level 14
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF	// Level 16
};

Uint32 HAT_0_MSK = 7;
Uint32 HATCH0[] = 
{
	0x01010101, 0x02020202, 0x04040404, 0x08080808, 
	0x10101010, 0x20202020, 0x40404040, 0x80808080,
//medium spaced thick 45 deg
	0x60606060, 0xc0c0c0c0, 0x81818181, 0x03030303,
	0x06060606, 0x0c0c0c0c, 0x18181818, 0x30303030,
// ;medium +-45 deg
	0x42424242, 0x81818181, 0x81818181, 0x42424242,
	0x24242424, 0x18181818, 0x18181818, 0x24242424,
// medium spaced vertical
	0x80808080, 0x80808080, 0x80808080, 0x80808080,
	0x80808080, 0x80808080, 0x80808080, 0x80808080, 
// medium spaced horizontal
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
// medium spaced cross
	0xffffffff, 0x80808080, 0x80808080, 0x80808080,
	0x80808080, 0x80808080, 0x80808080, 0x80808080,
};


Uint32 HAT_1_MSK = 15;
Uint32 HATCH1[] = 
{
// wide +45 deg
	0x00010001, 0x00020002, 0x00040004, 0x00080008,
	0x00100010, 0x00200020, 0x00400040, 0x00800080,
	0x01000100, 0x02000200, 0x04000400, 0x08000800,
	0x10001000, 0x20002000, 0x40004000, 0x80008000,
// widely spaced thick 45 deg
	0x80038003, 0x00070007, 0x000e000e, 0x001c001c,
	0x00380038, 0x00700070, 0x00e000e0, 0x01c001c0,
	0x03800380, 0x07000700, 0x0e000e00, 0x1c001c00,
	0x38003800, 0x70007000, 0xe000e000, 0xc001c001,
// widely +- 45 deg
	0x80018001, 0x40024002, 0x20042004, 0x10081008,
	0x08100810, 0x04200420, 0x02400240, 0x01800180,
	0x01800180, 0x02400240, 0x04200420, 0x08100810,
	0x10081008, 0x20042004, 0x40024002, 0x80018001,
// widely spaced vertical
	0x80008000, 0x80008000, 0x80008000, 0x80008000,
	0x80008000, 0x80008000, 0x80008000, 0x80008000,
	0x80008000, 0x80008000, 0x80008000, 0x80008000,
	0x80008000, 0x80008000, 0x80008000, 0x80008000,
// widely spaced horizontal
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
// ;widely spaced horizontal/vert cross
        0xFFFFFFFF, 0x80008000, 0x80008000, 0x80008000,
        0x80008000, 0x80008000, 0x80008000, 0x80008000,
        0x80008000, 0x80008000, 0x80008000, 0x80008000,
        0x80008000, 0x80008000, 0x80008000, 0x80008000,
};
Uint32 HOLLOW[] = { 0x00000000 };
Uint32 SOLID [] = { 0xFFFFFFFF };

Uint32 udpat[32] = 
{
	0x00000000, 0x00000000, 0x3ffc3ffc, 0x20042004,
	0x20042004, 0x2e742e74, 0x2e542e54, 0x2e542e54, 
	0x2e542e54, 0x2e542e54, 0x2e742e74, 0x20042004,
	0x20042004, 0x3ffc3ffc, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x3ffc3ffc, 0x20042004,
        0x20042004, 0x2e742e74, 0x2e542e54, 0x2e542e54,
        0x2e542e54, 0x2e542e54, 0x2e742e74, 0x20042004,
        0x20042004, 0x3ffc3ffc, 0x00000000, 0x00000000,
};

// Marker shapes
// Format of these is : <no. of polylines>
//			[ <no. of coordinate pairs> [ <coordinate pairs> ]] 

Sint32 m_dot[]   = { 1, 2,   0, 0,   0, 0 };
Sint32 m_plus[]  = { 2, 2,   0,-4,   0, 4,  2,  -5, 0,   5, 0 };
Sint32 m_star[]  = { 3, 2,   0,-4,   0, 4,  2,   4, 2,  -4,-2, 
                                                2,   4,-2,  -4, 2 };
Sint32 m_square[]= { 1, 5,  -5,-4,   5,-4,       5, 4,  -5, 4,  -5,-4 };
Sint32 m_cross[] = { 2, 2,  -5,-4,   5, 4,  2,  -5, 4,   5,-4 };
Sint32 m_dmnd[]  = { 1, 5,  -5, 0,   0,-4,       5, 0,   0, 4,  -5, 0 };


void SdSdl::init_patterns()
{
	int n;

	for (n = 0; n < MAX_LINE_STYLE; n++) m_line_sty[n] = line_styles[n];
	for (n = 0; n < 16 /*XXX 32*/ ; n++) m_UD_PATRN[n] = udpat[n];
}



Uint32 DrvGlobals::MAP_COL(Uint32 colour)
{
	return SDL_MapRGB(m_surface->format, 
			m_clr_red[colour],	
			m_clr_green[colour],
			m_clr_blue[colour]);
}

// Palette commands

inline int min(int a, int b) { return (a < b) ? a : b; }
inline int max(int a, int b) { return (a > b) ? a : b; }

void DrvGlobals::setColor(long index, short r, short g, short b)
{
	SDL_Color clr;

	if (index >= MAX_COLOR || index < 0) return;
	m_req_red[index]   = max(0, min(r, 1000));
	m_req_green[index] = max(0, min(g, 1000));
	m_req_blue[index]  = max(0, min(b, 1000));

	clr.r = m_clr_red  [index] = (m_req_red  [index] * 255) / 1000;
        clr.g = m_clr_green[index] = (m_req_green[index] * 255) / 1000;
        clr.b = m_clr_blue [index] = (m_req_blue [index] * 255) / 1000;

	SDL_SetColors(m_surface, &clr, index, 1);
}


long DrvGlobals::qColor(long index, short mode, short *r, short *g, short *b)
{
	if (index < 0 || index >= MAX_COLOR) return -1;

	if (!mode)
	{
		*r = m_req_red  [index];
		*g = m_req_green[index];
		*b = m_req_blue [index];
	}
	else
	{
		*r = (m_clr_red  [index] * 1000) / 255;
                *g = (m_clr_green[index] * 1000) / 255;
                *b = (m_clr_blue [index] * 1000) / 255;
	}
	return index;
}

