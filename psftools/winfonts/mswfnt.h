/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000  John Elliott

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
*/
typedef unsigned char  msw_byte;    /* Sizes of these aren't critical */
typedef unsigned short msw_word;
typedef unsigned long  msw_dword;

/* A Windows font header (based on WINE's wingdi16.h) */
typedef struct 
    {
    msw_word  dfVersion;		/* 00 */
    msw_dword dfSize;			/* 02 */
    char      dfCopyright[60];		/* 06 */
    msw_word  dfType;			/* 42 */
    msw_word  dfPoints;			/* 44 */
    msw_word  dfVertRes;		/* 46 */
    msw_word  dfHorizRes;		/* 48 */
    msw_word  dfAscent;			/* 4A */
    msw_word  dfInternalLeading;	/* 4C */
    msw_word  dfExternalLeading;	/* 4E */
    msw_byte  dfItalic;			/* 50 */
    msw_byte  dfUnderline;		/* 51 */
    msw_byte  dfStrikeOut;		/* 52 */
    msw_word  dfWeight;			/* 53 */
    msw_byte  dfCharSet;		/* 55 */
    msw_word  dfPixWidth;		/* 56 */
    msw_word  dfPixHeight;		/* 58 */
    msw_byte  dfPitchAndFamily;		/* 5A */
    msw_word  dfAvgWidth;		/* 5B */
    msw_word  dfMaxWidth;		/* 5D */
    msw_byte  dfFirstChar;		/* 5F */
    msw_byte  dfLastChar;		/* 60 */
    msw_byte  dfDefaultChar;		/* 61 */
    msw_byte  dfBreakChar;		/* 62 */
    msw_word  dfWidthBytes;		/* 63 */
    msw_dword dfDevice;			/* 65 */
    msw_dword dfFace;			/* 69 */
    msw_dword dfBitsPointer;		/* 6D */
    msw_dword dfBitsOffset;		/* 71 */
    msw_byte  dfReserved;		/* 75 */

/* From here on, only present in version 3+ */
    msw_dword dfFlags;
    msw_word  dfAspace;
    msw_word  dfBspace;
    msw_word  dfCspace;
    msw_dword dfColorPointer;
    msw_dword dfReserved1[4];
    } MSW_FONTINFO;


/* Initialise a FONTINFO structure */
void msw_fontinfo_new(MSW_FONTINFO *f);

/* Read/write functions return:
 * -1: Read/write error
 * -2: Font is not in Windows format
 * -3: Out of memory 
 */
/* Write a FONTINFO to disc. */
int msw_fontinfo_write(MSW_FONTINFO *f, FILE *fp);
/* Load a FONTINFO from disc */
int msw_fontinfo_read (MSW_FONTINFO *f, FILE *fp);
/* Allocate space for the remainder of the font file, and load it.
 * If successful, (*addr) is the address of the font data, loaded
 * at an offset equal to the size of the header (so that offsets 
 * within the data are equal to offsets within the font file). 
 * The caller must call free() on (*addr) when it's finished.
 */
int msw_font_body_read(MSW_FONTINFO *f, FILE *fp, msw_byte **addr);
/* 
 * Write out the remainder of the font; it is assumed to be at an
 * offset from "addr", as above. 
 */
int msw_font_body_write(MSW_FONTINFO *f, FILE *fp, msw_byte *addr);

