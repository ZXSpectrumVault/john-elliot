/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;             Convert binary file to Z80 assembly                             ;
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

/* Converts a binary file to ZSM source suitable for inclusion. Since 
 * z80asm doesn't have a .phase directive, we support code relocation by: 
 *
 * 1. Create the module - either .PRL, or ORGed at a particular address.
 * 2. Assemble it to a standalone binary.
 * 3. Convert it to ZSM source (as a sequence of bytes)
 * 4. Include this source into the program which should contain the 
 *   relocated code.
 */ 

static FILE *fpin, *fpout;

int main(int argc, char **argv)
{
	int c, l;

	c = l = 0;
	if (argc < 2) fpin  = stdin;  else fpin  = fopen(argv[1], "rb");
	if (argc < 3) fpout = stdout; else fpout = fopen(argv[2], "w");

	if (!fpin)
	{	
		perror(argv[1]); exit(1);
	}
	if (!fpout)
	{
		perror(argv[2]); exit(1);
	}

	while (c != EOF)
	{
		c = fgetc(fpin); 
		if (c == EOF) break;

		if (!l) fprintf(fpout,"\n\tDEFB\t");
		else	fprintf(fpout,", ");
		fprintf(fpout, "0%02xh", c);
		++l; if (l == 10) l = 0;
	}	
	fprintf(fpout, "\n");
	if (fpin  != stdin)  fclose(fpin);
	if (fpout != stdout) fclose(fpout);
	return 0;
}

