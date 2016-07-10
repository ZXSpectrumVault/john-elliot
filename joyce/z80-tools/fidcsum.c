/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;             Checksum a CP/M driver (*.FID)                                  ;
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

byte *data;

int main(int argc, char **argv)
{
	FILE *fpi = fopen(argv[1], "rb");
	FILE *fpo;
	byte hdr[256];
	unsigned int nbytes, n;
	unsigned int cksum, osum;

	if (!fpi) { perror(argv[1]); exit(1); }

	fpo = fopen(argv[2], "wb");
	if (!fpo) { perror(argv[2]); fclose(fpi); exit(1); }

	if (fread(hdr, 1, 256, fpi) < 256) 
	{
		fprintf(stderr,"%s is not a FID file\n", argv[1]);
		fclose(fpi); fclose(fpo);
		exit(1);
	}

	nbytes = (hdr[1] + 256*hdr[2]);		/* Size of file */
	nbytes += (nbytes + 7) / 8;		/* Size of relmap */
	nbytes += 256;				/* Size of header */

	data = malloc(nbytes + 128); 
	memset(data, 0, nbytes + 128);
	if (!data)	
	{
		fclose(fpi); fclose(fpo);
		fprintf(stderr,"Out of memory error - cannot malloc %d bytes\n",
				nbytes);
		exit(2);
	}
	memcpy(data,hdr,256);
	if (fread(data + 256, 1, nbytes - 256, fpi) < (nbytes - 256))
	{
		fprintf(stderr,"Unexpected end-of-file on %s\n", argv[1]);
		fclose(fpi); fclose(fpo);
		exit(1);
	}
	osum = data[0x110] + 256 *data[0x111];
	cksum = 0;
	data[0x110] = data[0x111] = 0;

	for (n = 0; n < nbytes; n++)
	{
		cksum += (unsigned int)(data[n]);	
	}
	data[0x110] = (cksum & 0xFF);
	data[0x111] = (cksum >> 8);

	if (fwrite(data, 1, nbytes, fpo) < nbytes)
	{
		fprintf(stderr,"Write error on file %s\n", argv[2]);
		fclose(fpi); fclose(fpo);
		exit(1);
	}
	fclose(fpi); fclose(fpo);
	if (cksum == osum)
		printf("Checksum was correct\n");
	else	printf("Checksum was wrong\n");
	return 0;
}
