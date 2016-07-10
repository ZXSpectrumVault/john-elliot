/************************************************************************

    ZXLILO v1.00 - Construct LILO menu screens with a Spectrum look

    Copyright (C) 2005  John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

byte bmp_header[] =
{
/* BITMAPFILEHEADER */
	'B' , 'M' , 		/* type */
	0x82, 0x58, 0x02, 0x00, /* size */
	0x00, 0x00, 0x00, 0x00, /* reserved */
	0xa6, 0x00, 0x00, 0x00,	/* offset to bits */
/* BITMAPINFOHEADER */
	0x28, 0x00, 0x00, 0x00,	/* size */
	0x80, 0x02, 0x00, 0x00, /* width */
	0xe0, 0x01, 0x00, 0x00, /* height */
	0x01, 0x00, 		/* planes */
	0x04, 0x00, 		/* depth */
	0x00, 0x00, 0x00, 0x00, /* No RLE */
	0xdc, 0x57, 0x02, 0x00, /* image size */
	0x12, 0x0b, 0x00, 0x00, /* X scale */
	0x12, 0x0b, 0x00, 0x00, /* Y scale */
	0x07, 0x00, 0x00, 0x00, /* Colours used */
	0x07, 0x00, 0x00, 0x00, /* Important colours */
/* Palette */
	0xc5, 0xc7, 0xc5, 0x00, /* 0: Grey */
	0x00, 0x00, 0x00, 0x00, /* 1: Black */
	0xff, 0xff, 0xff, 0x00, /* 2: White */
	0x00, 0x00, 0xff, 0x00, /* 3: Red */
	0x00, 0xff, 0xff, 0x00, /* 4: Yellow */
	0x00, 0xff, 0x00, 0x00, /* 5: Green */
	0xff, 0xff, 0x00, 0x00, /* 6: Cyan */
	0x00, 0x00, 0x00, 0x00, /* 7 */
	0x00, 0x00, 0x00, 0x00, /* 8 */
	0x00, 0x00, 0x00, 0x00, /* 9 */
	0x00, 0x00, 0x00, 0x00, /* A */
	0x00, 0x00, 0x00, 0x00, /* B */
	0x00, 0x00, 0x00, 0x00, /* C */
	0x00, 0x00, 0x00, 0x00, /* D */
	0x00, 0x00, 0x00, 0x00, /* E */
	0x00, 0x00, 0x00, 0x00, /* F */
};

byte lilo_header[] = 
{
	0x30, 0x00, 0x00, 0x00,	/* 00 Length */
	'L' , 'I' , 'L' , 'O' ,	/* 04 Magic */
	0xb0, 0x00, 		/* 08 Menu Y */
	0xb0, 0x00, 		/* 0A Menu X */
	0x01, 0x00, 		/* 0C Menu columns */
	0x05, 0x00,		/* 0E Menu rows */
	0x80, 0x00, 		/* 10 Column spacing */
	0x01, 0x00, 		/* 12 Foreground colour */
	0x02, 0x00, 		/* 14 Background colour */
	0x01, 0x00, 		/* 16 Shadow */
	0x01, 0x00, 		/* 18 Highlighted foreground */
	0x06, 0x00, 		/* 1A Highlighted background */
	0x01, 0x00, 		/* 1C Highlighted shadow */
	0x02, 0x00, 		/* 1E Timer foreground */
	0x01, 0x00, 		/* 20 Timer background */
	0x02, 0x00, 		/* 22 Timer shadow */
	0xa0, 0x00, 		/* 24 Timer Y */
	0x00, 0x01, 		/* 26 Timer X */
	0x04, 0x00, 		/* 28 Minimum per column */
	0x00, 0x00, 		/* 2A */
	0x00, 0x00, 		/* 2C */
	0x00, 0x00, 		/* 2E */
	
};

byte bitmap_data[320*480];

char *lilo[] = 
{
	"                                ",
	" L        IIIII  L        OOOO  ",
	" L          I    L       O    O ",
	" L          I    L       O    O ",
	" L          I    L       O    O ",
	" L          I    L       O    O ",
	" LLLLLLL  IIIII  LLLLLL   OOOO  ",
	"                                ",
};

#define GREY   0
#define BLACK  1
#define WHITE  2
#define RED    3
#define YELLOW 4
#define GREEN  5
#define CYAN   6

#define MENU_X 174
#define MENU_W 228
#define LINE_H 16
	
void set_header(int offset, int x)
{
	lilo_header[offset]   = x & 0xFF;
	lilo_header[offset+1] = (x >> 8) & 0xFF;
}

void plot(int x, int y, int ink)
{
	y = 479 - y;
	if (x & 1)
	{
			bitmap_data[y * 320 + (x >> 1)] &= 0xF0;
			bitmap_data[y * 320 + (x >> 1)] |= ink;
	}
	else
	{
			bitmap_data[y * 320 + (x >> 1)] &= 0x0F;
			bitmap_data[y * 320 + (x >> 1)] |= (ink << 4);
	}
}



void rect(int x, int y, int w, int h, int ink)
{
	int x1, y1;
	for (y1 = 0; y1 < h; y1++)
	{
		for (x1 = 0; x1 < w; x1++)
		{
			plot(x + x1, y + y1, ink);
		}
	}	
}


void caption(int x, int y, int ink)
{
	int x1, y1, w;
	for (y1 = 0; y1 < 8; y1++)
	{
		w = strlen(lilo[y1]);
		for (x1 = 0; x1 < w; x1++)
		{
			if (lilo[y1][x1] != ' ')
			{
				rect(x+2*x1, y+2*y1, 2, 2, ink);
			}
		}
	}	
}

void stripe(int x, int y, int ink)
{
	int y1;

	for (y1 = 0; y1 < 16; y1++)
	{
		rect(x, y + y1, 16, 1, ink);
		--x;
	}
}

void syntax(const char *arg)
{
	fprintf(stderr, "Syntax: %s { lines } { filename }\n", arg);
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
	int nopt = 5;
	int menu_y = 160;
	char *filename = "zxlilo.bmp";
	FILE *fp;

	if (argc > 1)
	{
		nopt = atoi(argv[1]);
		if (!nopt)
		{
				syntax(argv[0]);
		}
	}
	if (argc > 2)
	{
		filename = argv[2];
	}
	menu_y = 240 - (8 * nopt + 40);	/* Centre menu vertically */
	memset(bitmap_data, 0, sizeof(bitmap_data));
	rect(MENU_X, menu_y, MENU_W, LINE_H, BLACK);
	rect(MENU_X, menu_y + LINE_H, MENU_W, (nopt + 1) * LINE_H, WHITE);
	rect(MENU_X, menu_y, 2, (nopt + 2)*LINE_H, BLACK);
	rect(MENU_X + MENU_W - 2, menu_y, 2, (nopt + 2)*LINE_H, BLACK);
	rect(MENU_X, menu_y + (nopt + 2)*LINE_H - 2, MENU_W, 2, BLACK);

	caption(MENU_X, menu_y, WHITE);
	stripe(MENU_X + MENU_W - 34, menu_y, CYAN);
	stripe(MENU_X + MENU_W - 50, menu_y, GREEN);
	stripe(MENU_X + MENU_W - 66, menu_y, YELLOW);
	stripe(MENU_X + MENU_W - 82, menu_y, RED);

	set_header(8,    menu_y + LINE_H);
	set_header(0x0E, nopt);
	set_header(0x24, menu_y);
	
	if (!strcmp(filename, "-"))
	{
		fp = stdout;
	}
	else
	{
		fp = fopen(filename, "wb");
	}
	if (!fp)
	{
		perror(filename);
		return EXIT_FAILURE;
	}
	if (fwrite(bmp_header,  1,sizeof(bmp_header),  fp) < sizeof(bmp_header) 
	||  fwrite(lilo_header, 1,sizeof(lilo_header), fp) < sizeof(lilo_header)
	||  fwrite(bitmap_data, 1,sizeof(bitmap_data), fp) < sizeof(bitmap_data)
	||  fclose(fp))
	{
		perror(filename);
		return EXIT_FAILURE;		
	}

	return EXIT_SUCCESS;
}
