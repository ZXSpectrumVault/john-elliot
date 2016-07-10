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


/* fh_flags   */
#define	DEFAULT 1	/* this is the default font (face and size) */
#define	HORZ_OFF  2	/* there are left and right offset tables */
#define	STDFORM  4	/* is the font in standard format */
#define MONOSPACE 8     /* mono-spaced font */

/* style bits */
#define	THICKEN	0x1
#define	LIGHT	0x2
#define	SKEW	0x4
#define	UNDER	0x8
#define	OUTLINE 0x10
#define	SHADOW	0x20
#define ROTODD  0x40
#define ROTHIGH 0x80
#define	ROTATE	(ROTODD | ROTHIGH)
#define	SCALE	0x100

typedef struct font_head {		/* Describes a font. Note that this */
    Uint32 font_id;			/* is the in-memory representation; */
    Sint32 point;			/* the GEM/16 font file is the same as */
    wchar_t name[32];			/* the GEM/16 in-memory font, but this */
    wchar_t first_ade;			/* is not the case here. Actually I */
    wchar_t last_ade;			/* hope this stuff stays here purely */
    Uint32 top;				/* as legacy support, and font output */
    Uint32 ascent;			/* proper is handled by a separate */
    Uint32 half;			/* scaler, as in GEM/5 */
    Uint32 descent;		
    Uint32 bottom;
    Uint32 maxCharWidth;
    Uint32 maxCellWidth;
    Uint32 leftOffset;		/* amount character slants left when skewed */
    Uint32 rightOffset;		/* amount character slants right */
    Uint32 thicken;		/* number of pixels to smear */
    Uint32 ul_size;		/* size of the underline */
    Uint32 lighten;		/* mask to and with to lighten  */
    Uint32 skew;		/* mask for skewing */
    Uint32 flags;		    

    Uint8 *hor_table;	  	/* horizontal offsets */
    Uint32 *off_table;		/* character offsets  */
    Uint8 *dat_table;		/* character definitions */
    Uint32 form_width;
    Uint32 form_height;

    struct font_head *next_font;/* pointer to next font */
    struct font_head *next_sect; /* pointer to next segment of font */	
    Uint32 lfu;			/* least frequently used counter for font */
    Uint32 *file_off;		/* gdos data for offset into font file */
    Uint32 data_siz;		/* gdos data for size of font data */
    Uint32 reserve[8];		/* reserved data area for future use */
} FontHead;
