/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005-6  John Elliott

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
#include "cnvshell.h"
#include "psflib.h"
#include <limits.h>

#ifndef PATH_MAX
# ifdef CPM
#  define PATH_MAX 20
# else
#  define PATH_MAX 256
# endif
#endif

typedef struct range
{
	int first;
	int last;
	struct range *next;
} RANGE;

typedef struct subset
{
	int applies;
	RANGE *range;
} SUBSET;

/* Originally extracted a subset of a PSF font, for utilities which don't get 
 * on well with Unicode. Now does rather more */

static char helpbuf[2048];
static PSF_MAPPING *codepage = NULL;
static PSF_MAPPING *setcodepage = NULL;
static int first  = -1;
static int last   = -1;
static int height = -1;
static int width  = -1;
static int strip  = 0;
static SUBSET bold;
static SUBSET centre;
static SUBSET dblheight;
static SUBSET flipx;
static SUBSET inverse;
static SUBSET repeat;
static SUBSET scale;
static SUBSET thin;
static int psf1  = 0, psf2 = 0;
static int ucs = 0;
static char permfile[PATH_MAX];
static char linebuf[20];
static psf_dword *permutation;

static PSF_FILE psfi, psfp, psfo;

/* Program name */
char *cnv_progname = "PSFXFORM";

int within_subset(int idx, SUBSET *s)
{
	RANGE *r;

	if (!s->applies) return 0;
	if (!s->range)   return 1;

	for (r = s->range; r != NULL; r = r->next)
	{
		if (idx >= r->first && idx <= r->last) return 1;
	}
	return 0;
}

char *parse_subset(char *value, SUBSET *s)
{
	char *p, *q;
	RANGE *r;

	s->applies = 1;
	if (!value[0])	/* No range applies */
	{
		s->range = NULL;
		return NULL;
	}
/* Parse a string formed [nnn|nnn-nnn]{,[nnn|nnn-nnn]}* */
	do
	{
		if (!isdigit(value[0]))
		{
			return "A character range must be formed nnn "
				"or nnn-nnn or nnn,nnn-nnn...";
		}
		r = malloc(sizeof(RANGE));
		if (!r) return "Out of memory";
		p = strchr(value, ',');	/* p->the part after the next comma */
		if (p) 
		{
			*p = 0;
			p++;
		}
		/* Now see if we have nnn or nnn-nnn */
		q = strchr(value, '-');
		if (q)
		{
			r->first = atoi(value);
			r->last  = atoi(q+1);
		}
		else
		{
			r->first = r->last = atoi(value); 
		}
		/* Add the new range to the subset */
		r->next = s->range;
		s->range = r;
		/* And go to the bit after the comma */
		value = p;
	}
	while (p);
	return NULL;
}

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "first"))  { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))   { last = atoi(value); return NULL; }
	if (!stricmp(variable, "height")) { height = atoi(value); return NULL; }
	if (!stricmp(variable, "width"))  { width = atoi(value); return NULL; }
	if (!stricmp(variable, "flip"))   { return parse_subset(value, &flipx); }
	if (!stricmp(variable, "bold"))   { return parse_subset(value, &bold); }
	if (!stricmp(variable, "thin"))   { return parse_subset(value, &thin); }
	if (!stricmp(variable, "double")) { return parse_subset(value, &dblheight); }
	if (!stricmp(variable, "inverse")) { return parse_subset(value, &inverse);}
	if (!stricmp(variable, "repeat")) { return parse_subset(value, &repeat);}
	if (!stricmp(variable, "scale")) { return parse_subset(value, &scale);}
	if (!stricmp(variable, "centre")) { return parse_subset(value, &centre);}
	if (!stricmp(variable, "center")) { return parse_subset(value, &centre);}
	if (!stricmp(variable, "strip"))  { strip = 1; return NULL; }
	if (!stricmp(variable, "256"))    { first = 0; last = 255; return NULL; }
	if (!stricmp(variable, "psf1"))   { psf1 = 1; return NULL; }
	if (!stricmp(variable, "psf2"))   { psf2 = 1; return NULL; }
	if (!stricmp(variable, "permute")) 
	{ 
		strncpy(permfile, value, PATH_MAX - 1); 
		permfile[PATH_MAX - 1] = 0; 
		return NULL; 
	}
	if (!stricmp(variable, "codepage"))  
	{
		codepage = psf_find_mapping(value);
		if (codepage == NULL) return "Code page name not recognised.";
		return NULL;
	}
	if (!stricmp(variable, "setcodepage"))  
	{
		setcodepage = psf_find_mapping(value);
		if (setcodepage == NULL) 
			return "Code page name not recognised.";
		return NULL;
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
} 


/* Return help string */
char *cnv_help(void)
{
    sprintf(helpbuf, "Syntax: %s psffile psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n "
		     "    --first=n         Start with character n\n"
                     "    --last=n          Finish with character n\n"
                     "    --256             Equivalent to --first=0 --last=255\n"
		     "    --psf1            Force output to PSF1 format\n"
		     "    --psf2            Force output to PSF2 format\n"
		     "    --bold{=range}    Make the characters bold\n"
		     "    --centre{=range}  Centre character in cell when changing size\n"
		     "    --double{=range}  Double character height\n"
		     "    --flip{=range}    Mirror characters horizontally\n"
		     "    --inverse{=range} Swap ink/paper cells\n"
		     "    --repeat{=range}  Repeat edge rows/columns when increasing character size\n"
		     "    --scale{=range}   Scale characters when changing size\n"
		     "    --thin{=range}    Make the characters thin\n"
		     "    --strip           Remove any Unicode directory\n"
		     "    --permute=f       Shuffle characters as described in file f\n"
		     "    --codepage=x      Extract the specified codepage\n"
		     "    --setcodepage=x   Replace the Unicode directory with the specified codepage\n"
		     "\n"
		     "Ranges, if present, are formed r,r,r... where each r is either nnn or nnn-nnn\n");
            
    return helpbuf;
}

/* Load a file describing permutations */
int load_transfile(char *transfile)
{
	int from, to;
	FILE *fp;
	char *s, *result;
	int c;

	if (!strcmp(transfile, "-")) fp = stdin;
	else fp = fopen(transfile, "r");

	if (!fp) 
	{
		perror(transfile);
		return -1;
	}

	do
	{
		result = fgets(linebuf, 20, fp);	
		if (feof(fp) || result == NULL) break;

		/* If line was longer than 20 chars, swallow the rest */
		s = strchr(linebuf, '\n'); 
		if (!s)
		{
			do
			{
				c = fgetc(fp);
			} while (c != EOF && c != '\n');
			if (c == EOF) break;
		}
		s = strchr(linebuf, ';'); if (s) *s = 0;
		s = strchr(linebuf, '#'); if (s) *s = 0;
		if (sscanf(linebuf, "%d,%d", &from, &to) == 2 
		&& to < psfi.psf_length && to >= 0)
		{
			permutation[to] = from;
		}	
	} while (!feof(fp));
	if (!strcmp(transfile, "-")) fclose(fp);
	return 0;
}


#define MIN(a,b) (a < b) ? a : b

void copychar(int ndest, int nsrc, psf_byte *dest, psf_byte *src)
{
/* Rewritten to use psf_get_pixel, psf_set_pixel rather than fumbling around
 * with the bitmaps. */
	int xsrc, ysrc, xdest, ydest;
	int dx, dy;
	psf_byte pix, pixl, pixr;
	int srcheight;
	double xscale, yscale;
	psf_unicode_dirent *ude;

/* 1. Copy the source bitmap to the destination bitmap. 
 *    dblheight makes the source pretend to be twice as high as it is */
	xscale = 1.0;
	yscale = 1.0;
	dx = 0;
	dy = 0;
	srcheight = psfi.psf_height;
	if (within_subset(ndest, &dblheight)) srcheight *= 2;
	if (within_subset(ndest, &scale))
	{
		xscale = ((double)psfi.psf_width) / ((double)psfo.psf_width);	
		yscale = ((double)srcheight)      / ((double)psfo.psf_height);
	}
	if (within_subset(ndest, &centre))
	{
		dx = ((int)psfi.psf_width - (int)psfo.psf_width) / 2;
		dy = ((int)srcheight      - (int)psfo.psf_height) / 2;
	}
	for (ydest = 0; ydest < psfo.psf_height; ydest++)
	{
		for (xdest = 0; xdest < psfo.psf_width; xdest++)
		{
			if (within_subset(ndest, &scale))
			{
				xsrc = (int)((xscale * xdest));
				ysrc = (int)((yscale * ydest));
			}
			else
			{
				xsrc = xdest + dx;
				ysrc = ydest + dy;
			}
			if (within_subset(ndest, &dblheight)) ysrc /= 2;
			if (within_subset(ndest, &flipx)) 
				xsrc = (psfi.psf_width - 1) - xsrc;
/* If we are repeating the last row and column out to the edge, then translate
 * out-of-range coords to the edge. */
			if (within_subset(ndest, &repeat))
			{
				if (xsrc < 0) xsrc = 0;
				if (xsrc >= psfi.psf_width) 
					xsrc = psfi.psf_width - 1;
				if (ysrc < 0) ysrc = 0;
				if (ysrc >= psfi.psf_height) 
					ysrc = psfi.psf_height - 1;
			}
/* If still out of range, return blank pixel */
			if (ysrc < 0 || ysrc >= psfi.psf_height || 
			    xsrc < 0 || xsrc >= psfi.psf_width)
			{
				pix = 0;
			}
			else psf_get_pixel(&psfi, nsrc, xsrc, ysrc, &pix);
			psf_set_pixel(&psfo, ndest, xdest, ydest, pix);
			/* Now apply special effects: Bold. Bold is done by 
			 * setting this pixel to black if the source pixel one
			 * to the left is black. */
			if (within_subset(ndest, &bold))
			{
				int bxsrc = xsrc - 1;
				if (bxsrc < 0 && within_subset(ndest, &repeat)) 
					bxsrc = 0;
				if (bxsrc >= 0)
				{
					psf_get_pixel(&psfi, nsrc, bxsrc, ysrc, &pixl);
					if (pixl) psf_set_pixel(&psfo, ndest, xdest, ydest, pixl);
				}
			}
			/* Thin. Thin checks three pixels: If this pixel is 
			 * black, the one to the left is white, and the one 
			 * to the right is black, then make this pixel white */
			if (within_subset(ndest, &thin))
			{
				int tlsrc = xsrc - 1;
				int trsrc = xsrc + 1;
				if (within_subset(ndest, &repeat))
				{
					if (tlsrc < 0) tlsrc = 0;
					if (trsrc >= psfi.psf_width) 
						trsrc = psfi.psf_width - 1;
				}
				if (tlsrc >= 0)
					psf_get_pixel(&psfi, nsrc, tlsrc, ysrc, &pixl);
				else pixl = 0;
				if (trsrc < psfi.psf_width)
					psf_get_pixel(&psfi, nsrc, trsrc, ysrc, &pixr);
				else pixr = 0;
				if (pixl == 0 && pix != 0 && pixr != 0)
				{
					psf_set_pixel(&psfo, ndest, xdest, ydest, 0);
				}
			}
			if (within_subset(ndest, &inverse))
			{
				psf_get_pixel(&psfo, ndest, xdest, ydest, &pix);
				psf_set_pixel(&psfo, ndest, xdest, ydest, !pix);
			}
		}
	}
/* Old psftools-1.0.2 code 

	int x, y, dy, altx, lastp, lastq, thisp;
	int w, sw, dw;
	int lastrow;
	psf_byte maskr, maskw;
	psf_byte *srcp, *destp;

// Possibly I should use psf_getpixel & psf_setpixel here? 
	sw = (psfi.psf_width + 7) / 8;
	dw = (psfo.psf_width + 7) / 8;
	w = MIN(psfo.psf_width,  psfi.psf_width);

	memset(dest, 0, psfo.psf_charlen);
	dy = 0;
	lastrow = 0;
	if (centre)
	{
		if (dblheight) dy = (psfo.psf_height - 2 * psfi.psf_height);
		else	       dy =  psfo.psf_height - psfi.psf_height;
		dy /= 2;
	}
	for (y = 0; y < psfi.psf_height; y++)
	{
		if (dblheight)	srcp = src + (y/2) * sw;
		else		srcp  = src + y * sw;
		if (y + dy < 0 || (y + dy > psfo.psf_height)) continue;
		lastrow = y + dy;
		destp = dest + (y+dy) * dw;
		maskr = 0x80;
		lastp = lastq = 0;
		for (x = 0; x < w; x++)
		{
// If bolding, then do it by duplicating the last source pixel 
			if ((srcp[x/8] & maskr) || (lastp && bold))
			{
				if (flipf)
				{
					altx = w - x - 1;
					maskw = 0x80 >> (altx & 7);
					destp[altx/8] |= maskw;
				}
				else
				{
					destp[x/8] |= maskr;
				}
			}
			if (srcp[x/8] & maskr) lastp = 1; else lastp = 0;
			maskr = maskr >> 1;
			if (maskr == 0) maskr = 0x80;		
		}	
// Thinning effect 
		if (thin)
		{
			maskr = 0x80;
			maskw = 1;
			lastp = lastq = 0;
			for (x = 0; x < w; x++)
			{
				thisp = (destp[x/8] & maskr) ? 1 : 0;
// Erode left-hand edge 
				if (lastq == 0 && lastp == 1 && thisp == 1)
				{
		// If so, erode previous pixel 
					destp[(x-1)/8] &= ~maskw;
				}
				lastq = lastp;
				lastp = thisp;
				maskr = maskr >> 1; if (maskr==0) maskr = 0x80;	
				maskw = maskw >> 1; if (maskw==0) maskw = 0x80;	
			}
		}
		if (repeat)
		{
			// Stretch last pixel column 
			maskr = 0x80 >> ((w - 1) & 7);
			maskw = 0x80 >> (w & 7);
			for (x = w; x < psfo.psf_width; x++)
			{
				if (destp[(w-1)/8] & maskr) destp[x/8]|=  maskw;
				else			    destp[x/8]&= ~maskw;
				maskw = maskw >> 1;
				if (maskw == 0) maskw = 0x80;
			}
		}
	}
	if (repeat)
	{
		srcp  = dest + lastrow * dw;
		for (y = lastrow + 1; y < psfo.psf_height; y++)
		{
			destp = dest + y * dw;
			memcpy(destp, srcp, dw);
		}
	}
*/
	if (setcodepage)
	{
		if (ndest < setcodepage->psfm_count)
			psf_unicode_addmap(&psfo, ndest, setcodepage, ndest);
	}
	else if (ucs) for (ude = psfi.psf_dirents_used[nsrc]; ude != NULL;
		     ude = ude->psfu_next)
	{
		psf_unicode_add(&psfo, ndest, ude->psfu_token);
	}
}

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	psf_dword glyph, sch, dch, f, l;
	psf_byte *src, *dest;

	if (strip && setcodepage)
	{
		return "Cannot have both --strip and --setcodepage options";
	}

	psf_file_new(&psfi);
	psf_file_new(&psfo);
	psf_file_new(&psfp);
        rv = psf_file_read(&psfi, infile);

	if (rv != PSF_E_OK) return psf_error_string(rv);
	if (permfile[0])
	{
		permutation = calloc(psfi.psf_length, sizeof(psf_dword));
		if (permutation == NULL)
		{
			return "No memory for buffer required by --permute option";
		}
		for (sch = 0; sch < psfi.psf_length; sch++) 
			permutation[sch] = sch;
		if (load_transfile(permfile)) 
			return "Could not load permutation table file.";
		rv = psf_file_create(&psfp, psfi.psf_width, psfi.psf_height,
				psfi.psf_length, 0);
		if (rv) return psf_error_string(rv);
	}
	if (codepage && !psf_is_unicode(&psfi))
	{
		return "Cannot extract by codepage; source file has no Unicode table";
	}

	f = (first >= 0) ? first : 0;
	l = (last  >= 0) ? last  : (psfi.psf_length - 1);
	if (codepage && l >= 256) l = 255;
	if (l >= psfi.psf_length) l = psfi.psf_length;

	ucs = 0;
	if (codepage || setcodepage || psf_is_unicode(&psfi)) ucs = 1;
	if (strip) ucs = 0;
	if (height == -1) height = psfi.psf_height;
	if (width == -1) 
	{
		width = psfi.psf_width;
		if (bold.applies) width++;
	}
	rv = psf_file_create(&psfo, width, height, (l - f + 1), ucs);
	if (rv) return psf_error_string(rv);

/* Do permutation as a separate pass */
	if (permutation) 
	{
		for (dch = 0; dch < psfi.psf_length; dch++)
		{	
			sch  = permutation[dch];
			if (sch > psfi.psf_length) continue;
			src  = psfi.psf_data + sch * psfi.psf_charlen; 
			dest = psfp.psf_data + dch * psfp.psf_charlen;
			memcpy(dest, src, psfp.psf_charlen);
		}
		memcpy(psfi.psf_data, psfp.psf_data, 
				psfp.psf_charlen * psfp.psf_length);
		psf_file_delete(&psfp);
	}

	for (sch = f; sch <= l; sch++)
	{
		dest = psfo.psf_data + (sch - f) * psfo.psf_charlen;
		if (codepage)
		{
			if (sch < 256 && !psf_unicode_lookupmap(&psfi, 
					codepage, sch, &glyph, NULL))
			{
				src = psfi.psf_data + glyph * psfi.psf_charlen;
				copychar(sch - f, glyph, dest, src);
			}
			else
			{
				if (sch < 256 && !psf_unicode_banned(codepage->psfm_tokens[sch][0])) fprintf(stderr, "Warning: U+%04lx not found in font\n", codepage->psfm_tokens[sch][0]);

				memset(dest, 0, psfo.psf_charlen);	
			}
		}
		else	
		{
			src = psfi.psf_data + sch * psfi.psf_charlen;
			copychar(sch - f, sch, dest, src);
		}
	}
	if (psf1) psf_force_v1(&psfo);
	if (psf2) psf_force_v2(&psfo);
	rv = psf_file_write(&psfo, outfile);
	psf_file_delete(&psfi);	
	psf_file_delete(&psfo);	
	if (permutation) free(permutation);
	if (rv) return psf_error_string(rv);
	return NULL;
}

