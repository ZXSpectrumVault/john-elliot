/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

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

#define EXT ".zxb"	/* Default file extension for +3DOS file */

#ifdef __PACIFIC__
#define AV0 "SPECFORM"
#else
#define AV0 argv[0]
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

FILE *fileSrc, *fileDest;
char *nameSrc, *nameDest;
int fixing;

unsigned char header[128] = 
	{ 'P', 'L', 'U', 'S', '3', 'D', 'O', 'S', 0x1A, 1, 0,
	 0, 0, 0, 0,	/* Length */
 	 3		/* CODE */
	 };

void help(char *name)
{
	fprintf(stderr,"Syntax: %s filename    - add +3DOS header\n", name);
	fprintf(stderr,"        %s -a addr filename - add +3DOS header with specified start address\n", name);
	fprintf(stderr,"        %s -F filename - fix existing +3DOS header\n", name);
	fprintf(stderr,"        %s -F filename - fix existing +3DOS header\n", name);
	exit(1);
}

void die(void)
{
	if (fileSrc)  fclose(fileSrc);
	if (fileDest) 
	{
		fclose(fileDest);
		remove(nameDest);
	}
	if (nameDest) free(nameDest);	
	exit(2);
}

void fail(char *s)
{
	perror(s);
	die();
}


void diewith(char *me, char *s)
{
	fprintf(stderr,"%s:%s\n",me, s);
	die();
}



int main(int argc, char **argv)
{
	long nLength;
	int n, argno;
	unsigned char sum;
	unsigned int nStartAddr = 0;

	argno = 1;

	while ((argno < argc) && (argv[argno][0] == '-'))
	{
		if (argv[argno][1] == 'F')
		{
			fixing = 1;
			++argno;
		}
		else if (argv[argno][1] == 'a')
		{
			++argno;
			if (argno < argc)
			{
				nStartAddr = atoi(argv[argno]);
				++argno;
			}
		}
		else
		{
			help(argv[0]);
		}
	}

	while (argno < argc)
	{
		nameSrc = argv[argno];
		fileSrc = fopen(argv[argno],fixing ? "r+b" : "rb");
		if (!fileSrc) fail(argv[argno]);

		nameDest = malloc(5 + strlen(argv[argno]));

		if (!nameDest) diewith(AV0, "Out of memory");

		strcpy(nameDest, argv[argno]);

#ifdef SHORTNAMES	/* Slice off the existing extension */
		{
			char *p = strrchr(nameDest,'.');
			if (p) *p = 0;
		}
#endif
		strcat(nameDest,EXT);

		if (!fixing)
		{
			fileDest = fopen(nameDest, "wb");
			if (!fileDest) fail(nameDest);
		}
		fseek(fileSrc, 0, SEEK_END);	/* Get length of fileSrc */
		nLength = ftell(fileSrc);
		fseek(fileSrc, 0, SEEK_SET);

		if (fixing)
		{
			if (fread(header, 1, 128, fileSrc) < 128)
			{
				fail(nameSrc);
			}
	                fseek(fileSrc, 0, SEEK_SET);
		}

		if (!fixing)
		{
			header[0x0b] =  (nLength + 0x80)        & 0xFF;
			header[0x0c] = ((nLength + 0x80) >> 8)  & 0xFF;
			header[0x0d] = ((nLength + 0x80) >> 16) & 0xFF;
			header[0x0e] = ((nLength + 0x80) >> 24) & 0xFF;

			header[0x10] =  nLength       & 0xFF;
			header[0x11] = (nLength >> 8) & 0xFF;

			header[0x12] = nStartAddr        & 0xFF;
			header[0x13] = (nStartAddr >> 8) & 0xFF;
		}

		if (!fixing || !memcmp(header, "PLUS3DOS\032\001", 11))
		{
			for (sum = 0, n = 0; n < 0x7f; n++)
			{
				sum += header[n];
			}	
			header[0x7f] = sum;
		}
		else if (fixing) fprintf(stderr, "%s: No +3DOS signature\n", 
                                         nameSrc);
		if (fixing)
		{
			fwrite(header, 1, 0x80, fileSrc);
			if (ferror(fileSrc)) fail(nameSrc);
		}
		else
		{
			fwrite(header, 1, 0x80, fileDest);
			if (ferror(fileDest)) fail(nameDest);
			while (nLength > 0)
			{
				int i = fgetc(fileSrc);

				if (i == EOF) diewith (AV0, "Premature end-of-file");
				fputc(i, fileDest);
				--nLength;
			}
			fclose(fileDest);
		}
		fclose(fileSrc);
		free(nameDest);	
		++argno;
	}
	return 0;
}
