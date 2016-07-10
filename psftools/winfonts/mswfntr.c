
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mswfnt.h"
#include "psflib.h"

int msw_fontinfo_read(MSW_FONTINFO *f, FILE *fp)
    {
    int n;

    if (read_word (fp, &f->dfVersion))         return -1;

    if (f->dfVersion != 0x100 && 
        f->dfVersion != 0x200 &&
	f->dfVersion != 0x300) 		       return -2;

    if (read_dword(fp, &f->dfSize))            return -1;
    if (fread          (f->dfCopyright, 1, 60, fp) < 60) return -1;
    if (read_word (fp, &f->dfType))            return -1;
    if (read_word (fp, &f->dfPoints))          return -1;
    if (read_word (fp, &f->dfVertRes))         return -1;
    if (read_word (fp, &f->dfHorizRes))        return -1;
    if (read_word (fp, &f->dfAscent))          return -1;
    if (read_word (fp, &f->dfInternalLeading)) return -1;
    if (read_word (fp, &f->dfExternalLeading)) return -1;
    if (read_byte (fp, &f->dfItalic))          return -1;
    if (read_byte (fp, &f->dfUnderline))       return -1;
    if (read_byte (fp, &f->dfStrikeOut))       return -1;
    if (read_word (fp, &f->dfWeight))          return -1;
    if (read_byte (fp, &f->dfCharSet))         return -1;
    if (read_word (fp, &f->dfPixWidth))        return -1;
    if (read_word (fp, &f->dfPixHeight))       return -1;
    if (read_byte (fp, &f->dfPitchAndFamily))  return -1;
    if (read_word (fp, &f->dfAvgWidth))        return -1;
    if (read_word (fp, &f->dfMaxWidth))        return -1;
    if (read_byte (fp, &f->dfFirstChar))       return -1;
    if (read_byte (fp, &f->dfLastChar))        return -1;
    if (read_byte (fp, &f->dfDefaultChar))     return -1;
    if (read_byte (fp, &f->dfBreakChar))       return -1;
    if (read_word (fp, &f->dfWidthBytes))      return -1;
    if (read_dword(fp, &f->dfDevice))          return -1;
    if (read_dword(fp, &f->dfFace))            return -1;
    if (read_dword(fp, &f->dfBitsPointer))     return -1;
    if (read_dword(fp, &f->dfBitsOffset))      return -1;
    if (f->dfVersion >= 0x200)
        {
	if (read_byte (fp, &f->dfReserved))        return -1;
        }
    if (f->dfVersion >= 0x0300)
        {
        if (read_dword(fp, &f->dfFlags))       return -1;
        if (read_word (fp, &f->dfAspace))      return -1;
        if (read_word (fp, &f->dfBspace))      return -1;
        if (read_word (fp, &f->dfCspace))      return -1;
	if (read_dword(fp, &f->dfColorPointer))return -1;
	for (n = 0; n < 4; n++) 
		if (read_dword(fp, &f->dfReserved1[n])) return -1;
        }
    return 0;
    }


int msw_font_body_read(MSW_FONTINFO *f, FILE *fp, msw_byte **addr)
{
	long size, base;

	if (!addr) return -3;
	/* Work out font size. This is f->dfSize minus header size */

	if (f->dfVersion >= 0x0300) base = 0x9A;
	else			    base = 0x76;

	size = f->dfSize - base;

	*addr = malloc(f->dfSize);
	if (!*addr) return -3;

	if (fread( (*addr)+base, 1, size, fp) < size) 
	{
		free(*addr);
		*addr = NULL;
		if (ferror(fp)) return -1;
		return -2;
	}
	return 0;	
}

