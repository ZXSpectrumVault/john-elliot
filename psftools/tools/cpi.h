/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005  John Elliott

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

/* This is a mini-library for loading and saving CPI files, the better to 
 * manipulate them. */
typedef unsigned char cpi_byte;

/* This corresponds to a single font. For FONT and FONT.NT files, the font 
 * bitmap will be stored in this object. For DRFONT files, these just have 
 * the font size; the font bitmaps live in the CP_DRFONT structure below. */
typedef struct cp_font
{
	unsigned pr_type;	/* Printer CPI: Printer type */
	unsigned pr_seqsize;	/* Printer CPI: Escape sequence length */
	unsigned fontsize;	/* Size of mallocced data */
	unsigned height;	/* Character height */
	unsigned width;		/* Character width */
	unsigned nchars;	/* Count of characters */
	unsigned xaspect;	/* X aspect */
	unsigned yaspect;	/* Y aspect */
	cpi_byte *data;		/* Character bitmaps */
	long self_offset;	/* Offset of this structure in the file */
	struct cp_font *next;	/* Linked list of fonts in codepage */
} CP_FONT;

/* This is an entry for a codepage; it may have one or more fonts attached. */
typedef struct cp_head
{
	unsigned headsize;		/* Size (in file) of the header */
	unsigned dev_type;		/* Device type 1=Screen 2=Printer */
	char  dev_name[9];		/* Device name */
	unsigned short codepage;	/* Codepage ID */
	unsigned font_count;		/* Number of fonts */
	long  next_offset;		/* File offset of next header */
	long  cpih_offset;		/* File offset of CP_FONT */
	long  self_offset;		/* File offset of this */
	long  font_len;			/* Length of all font data */
	unsigned short *dr_lookup;	/* DRFONT lookup table */
	struct cp_font *fonts;		/* Child fonts */
	struct cp_head *next;		/* Linked list of these in CPI file */
} CP_HEAD;

/* This contains the font bitmaps used by DRFONT fonts. */
typedef struct cp_drfont
{
	unsigned char_len;
	long offset;
	unsigned bitmap_len;
	cpi_byte *bitmap;
} CP_DRFONT;

/* And this represents an entire file */
typedef struct cpi_file
{
	cpi_byte magic0;	/* 0x7F for DRFONT, 0xFF for FONT */
	char format[8];		/* DRFONT / FONT as string */
	CP_HEAD *firstpage;	/* Linked list of codepages */
	unsigned drcount;	/* Number of size records (DRFONT) */
	CP_DRFONT *drfonts;	/* The DRFONT-specific entries */	
	unsigned comment_len;	/* Size of comment (usually (c) message) */
	cpi_byte *comment;	/* Comment data */
} CPI_FILE;

/* The three CPI formats supported: */
typedef enum
{
	CPI_FONT, 	/* MS-DOS / PC-DOS screen or printer font */
	CPI_FONTNT, 	/* Windows NT font with relative addresses */
	CPI_DRFONT,	/* DR-DOS compressed screen font */
	CPI_NAKED	/* Linux .CP file */
}
cpi_format;

/* Initialise a CPI object */
int cpi_new   (CPI_FILE *f, cpi_format format);
/* Free a CPI object and all its memory */
int cpi_delete(CPI_FILE *f);
/* Load a CPI file */
int cpi_load  (CPI_FILE *f, const char *filename);
int cpi_loadfile(CPI_FILE *f, FILE *fp);
/* Save a CPI file. Interleaved is 0 to save the headers followed by the
 * data; 1 to save each header followed immediately by its associated data. */
int cpi_save  (CPI_FILE *f, const char *filename, int interleaved);
int cpi_savefile(CPI_FILE *f, FILE *fp, int interleaved);

/* Debugging: Dump a CPI structure as text */
void cpi_dump (CPI_FILE *f);

/* See if CPI file has the given codepage */
CP_HEAD *cpi_lookup(CPI_FILE *f, unsigned number);
/* Force CPI file to have the given codepage */
CP_HEAD *cpi_add_page(CPI_FILE *f, unsigned number);
/* Add a font to a codepage */
CP_FONT *cph_add_font(CP_HEAD *h);
/* Convert the internal format of a CPI file to DRFONT */
int cpi_crush(CPI_FILE *f);
/* Convert the internal format of a CPI file from DRFONT */
int cpi_bloat(CPI_FILE *f);
/* Get the number of characters in a codepage */
int cpi_count_chars(CP_HEAD *h);
/* Find the bitmap for a glyph */
cpi_byte *cpi_find_glyph(CPI_FILE *f, int codepage, int height, 
		int width, int glyph);

/* Error numbers have been chosen to match the ones in psflib.h */
#define CPI_ERR_OK     (0)	/* No error */
#define CPI_ERR_NOMEM  (-1)	/* malloc() failed */
#define CPI_ERR_BADFMT (-3)	/* CPI file not in correct format */
#define CPI_ERR_ERRNO  (-4)	/* Error in stdio function, use perror() */
#define CPI_ERR_EMPTY  (-5)	/* File is empty */
