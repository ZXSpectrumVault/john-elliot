/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;             Create a .PRL file from two binary files                        ;
;                                                                             ;
;   Copyright (C) 1998-1999, John Elliott <jce@seasip.demon.co.uk>            ;
;                                                                             ;
;    This program is free software; you can redistribute it and/or modify     ;
;    it under the terms of the GNU General Public License as published by     ;
;    the Free Software Foundation; either version 2 of the License, or        ;
;    (at your option) any later version.                                      ;
;                                                                             ;
;    This program is distributed in the hope that it will be useful,          ;
;    but WITHOUT ANY WARRANTY; without even the implied warranty of           ;
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            ;
;    GNU General Public License for more details.                             ;
;                                                                             ;
;    You should have received a copy of the GNU General Public License        ;
;    along with this program; if not, write to the Free Software              ;
;    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                ;
;                                                                             ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char byte;

/* Up to 3 input files, and an output file */

static FILE *fpi1, *fpi2, *fpi3, *fpout;

/* makeprl: Build utility
 *
 * Combines two binary images into Digital Research .PRL format.
 *
 * Note: The second image will be used for the binary data. It should 
 * therefore be ORGed at 100h for PRL, RSX and FID files; and 000h for 
 * SPR files. The first image can be ORGed anywhere as long as it isn't the
 * same place as the second image.
 */

/*
 * Write the bit b to a stream of bits fp. Note: At the end of the file
 * creation process, there may be up to 7 bits of data left in this routine's
 * buffer. Write 7 zero bits to make sure all data have been written.
 *
 */

void addbit(FILE *fp, byte b)
{
	static byte m = 0x80;	/* Mask. MSB comes first */
	static byte c = 0;	/* Count of bits (write byte when we have 8) */
	static byte o = 0;	/* Output byte under construction */

	if (b) o |= m; else o &= (~m);	/* Write the bit */
	m = (m >> 1);			/* Mask to next position */
	c++;				/* Next count */
	if (c == 8)
	{
		fputc(o, fp);		/* Write the byte */
		m = 0x80;		/* Reset variables */
		c = 0;
		o = 0;
	}	
}


/* And the only other function in the program is main() */


int main(int argc, char **argv)
{
	byte p,q;
	int i, j;	/* General purpose */
	long count = 0;	/* Length of binary image */

	if (argc < 5)
	{
		fprintf(stderr,"Syntax: makeprl image.0 image.1 header image.prl\n");
		exit(1);
	}
	fpout = fopen(argv[4], "wb");	/* Open output file */
	if (!fpout) 
	{
		perror(argv[4]);
		exit(1);
	}

	fpi1 = fopen(argv[1], "rb");	/* Open input files */
	if (!fpi1)
	{
		fclose(fpout);
		perror(argv[1]);
		exit(1);	
	}

	fpi2 = fopen(argv[2], "rb");
	if (!fpi2)
	{
		fclose(fpout);
		fclose(fpi1);
		perror(argv[2]);
		exit(1);
	}

	/* Header image file - if there is none, then supply /dev/zero here */

	fpi3 = fopen(argv[3], "rb");
	if (!fpi3)
	{
		fclose(fpout);
		fclose(fpi1);
		fclose(fpi2);
		perror(argv[3]);
		exit(1);
	}

	/* Copy the PRL header, packing it out to 256 chars with zeroes */

	for (i = j = 0; i < 256; i++) 
	{
		if (j != EOF) j = fgetc(fpi3);
		if (j == EOF) fputc(0, fpout);	/* PRL header */
		else          fputc(j, fpout);
	}
	/* Copy the byte image from file 2 to PRL file */
	
	while (!feof(fpi2))
	{
		i = fgetc(fpi2);
		if (i == EOF) continue;
		fputc(i, fpout);
		++count;
	}
	fseek(fpi2, 0L, SEEK_SET);	/* Rewind file 2 */

	/* Compare byte-by-byte, and if they are different then put a 1 in 
         * the relocation map */

	while (!feof(fpi2))
	{
		int i = fgetc(fpi2);
		if (i == EOF) break;

		p = i;
		i = fgetc(fpi1);
		if (i == EOF) break;
		q = i;

		addbit(fpout, (p != q));
	}
	/* Flush the bitwise output buffer */

	for (p = 0; p < 7; p++) addbit(fpout, 0);
	fclose(fpi1);
	fclose(fpi2);
	fseek(fpout, 1L, SEEK_SET);	/* Write PRL file length */
	fputc((count & 0xFF), fpout);
	fputc((count >> 8), fpout);
	fclose(fpout);	

	return 0;
}

