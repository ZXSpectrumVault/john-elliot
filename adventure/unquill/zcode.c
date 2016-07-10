/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996-2000  John Elliott

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
#include "unquill.h"

#define LIBVER "7"	/* Required version number of Zcode module */

void zcode_binary(void)
{
	FILE *fp = fopen("iqrun.z5", "rb");
	char achad[6];
	ushrt n;
	int i;

#ifdef LIBDIR
	char fname[PATH_MAX];

	if (!fp)
	{
		strcpy(fname, LIBDIR);
		strcat(fname, "iqrun.z5");
		fp = fopen(fname, "rb");
	}
	if (!fp)
	{
		fprintf(stderr,"Cannot open iqrun.z5 or %siqrun.z5\n",
			LIBDIR);
		exit(1);

	}

#else
	if (!fp) 
	{
		fprintf(stderr,"Cannot open iqrun.z5\n");
		exit(1);	
	}
#endif
	/* Firstly: Copy bytes from Z-code file to outfile, until we see
         * the magic word ACHAD - the spell in Heavy On The Magick which
         * will revive a long-dead being. As Apex puts it so well:
         *
         * FOR AI IS DEAD 
         * TAKE ARM LEG HEAD
         * IN POT DISPLAY
         * AND ONE WORD SAY
         *
         */
	for (n = 0; n < 6; n++) achad[n] = 0;

	while (!feof(fp))
	{
		for (n = 0; n < 5; n++) achad[n] = achad[n+1];
		achad[5] = fgetc(fp);
		fputc(achad[5], outfile);
		if (memcmp(achad, "Achad" LIBVER, 6)) continue;

	/* We've got it. That's dealt with the head, so let's do the arm.
         * 
         */
		if (arch == ARCH_SPECTRUM)
		{
			/* Skip 256 bytes - Spectrum snapshots are smaller */
			for (n = 0; n < 256; n++) 
			{
				fputc(fgetc(fp), outfile);
			}
		}
		for (n = 0; n < mem_size; n++)
		{
			fgetc(fp); /* Skip the corresponding z-code byte */
			fputc(zmem(n + mem_base), outfile);
		}
	/* Now the options */
		fgetc(fp); fputc(1,     outfile);	/* Valid game */
		fgetc(fp); fputc(dbver, outfile); /* Database version? */
		fgetc(fp); fputc(mopt,  outfile); /* Mono? */
		fgetc(fp); fputc(!gopt, outfile); /* Graphics */
		fgetc(fp); fputc(nobeep,outfile); /* Quiet */
		fgetc(fp); fputc(arch,  outfile); /* Architecture */
	/* Now the legs */
		while (	(i = fgetc(fp)) != EOF)
		{
			fputc(i, outfile);	
		}
	}	
	fclose(fp);	/* It's in the pot and ready! */
/*
 * nb: Because this is a one-pass transformation, the Z-code checksum is
 *    not set on the output file. If this causes trouble, a standalone
 *    checksum utility could be built.
 *
 */	

}
