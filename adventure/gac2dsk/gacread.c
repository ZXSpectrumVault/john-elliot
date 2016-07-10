/************************************************************************

    GAC2DSK 1.0.0 - Convert GAC tape files to +3DOS

    Copyright (C) 2007, 2009  John Elliott <jce@seasip.demon.co.uk>

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

#define INSTANTIATE
#include "gacread.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif


void parsecode(int n)
{
	gac_sp = datablock[n+ 3] + 256 * datablock[n+ 4];
	gac_pc = datablock[n+14] + 256 * datablock[n+15];
	gac_de = datablock[n+18] + 256 * datablock[n+19];
	gac_ix = datablock[n+22] + 256 * datablock[n+23];
}

int loadblock(FILE *fp)
{
	byte header[2];
	byte *addr;

	if (fread(header, 1, 2, fp) < 2)
	{
		return -2;
	}
	datalen = header[1] * 256 + header[0];
	addr = datablock;
	while (datalen > 16384)
	{
		if (fread(addr, 1, 16384, fp) < 16384) return -1;

		addr += 16384;
		datalen -= 16384;
	}
	if (fread(addr, 1, datalen, fp) != datalen)
	{
		return -1;
	}	
	return 0;
}


int load_gac(const char *filename)
{
	FILE *fp;
	int err, p3dos;
	unsigned char header[128];

	fp = fopen(filename, "rb");
	if (!fp)
	{
		perror(filename);
		return -1;
	}
	p3dos = 0;
	/* Check for a +3DOS header */
	if (fread(header, 1, sizeof(header), fp) == sizeof(header))
	{
		if (!memcmp(header, "PLUS3DOS\032", 9))
		{
			byte sum = 0;
			int n;

			for (n = 0; n < 127; n++) sum += header[n];
			if (header[n] == sum)
			{
				fprintf(stderr, "+3DOS header on input file\n");
				p3dos = 128;
			}
		}
	}

	fseek(fp, p3dos, SEEK_SET);
	/* First block should be the header for a BASIC loader */
	do
	{
		int n;

		err = loadblock(fp);
		if (err == -2)
		{
			fprintf(stderr, "End of file %s. No GAC loader "
					"was found.\n", filename);
			fclose(fp);
			return -2;
		}
		if (err == -1)
		{
			fprintf(stderr, "%s: Unexpected EOF.\n", filename);
			fclose(fp);
			return -1;
		}
		/* Is this a header? */
		if (datablock[0] == 0) switch(datablock[1])
		{
			case 0: fprintf(stderr, "Program: %-10.10s\n", 
						datablock+2); break;
			case 1: fprintf(stderr, "Number array: %-10.10s\n", 
						datablock+2); break;
			case 2: fprintf(stderr, "Character array: %-10.10s\n", 
						datablock+2); break;
			case 3: fprintf(stderr, "Bytes: %-10.10s\n", 
						datablock+2); break;
		}
		/* Is this the BASIC loader? */
		if (datablock[0] != 0 || datablock[1] != 0) continue;
		memcpy(progname, datablock + 2, 10);
		progname[10] = 0;
		err = loadblock(fp);
		if (err)
		{
			fprintf(stderr, "%s: Unexpected EOF.\n", filename);
			fclose(fp);
			return -1;
		}
		/* Seek out the loader */
		for (n = 0; n < datalen - 2; n++)
		{
			if (datablock[n]   == 0xEA &&
			    datablock[n+1] == 0xF3 &&
			    datablock[n+2] == 0x31)
			{
				fprintf(stderr, "Found GAC loader at offset %d\n", n);
				parsecode(n);
/* Load the main block of data */
				err = loadblock(fp);
				if (err)
				{
					fprintf(stderr, "%s: Unexpected EOF.\n", filename);
					fclose(fp);
					return -1;
				}
				fclose(fp);
				return 0;
			}
		}	
	} while (1);
	return -1;	/* Should never get here */
}
