/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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

#include "Pcw.hxx"

#ifdef HAVE_WINDOWS_H

#include "PcwBdos.hxx"
#include <direct.h>
#include <io.h>


void alert(const string s)
{
	::MessageBox(NULL, s.c_str(), "JOYCE fatal error", MB_OK | MB_ICONSTOP);
}

void txt2fcb(Z80 *R, word addr, char *s);	/* Parse ASCII filename to FCB */

#define FIND_ATTR _A_RDONLY | _A_HIDDEN | _A_ARCH

static char pathname[162];	/* DOS pathname passed by Z80 program */


word jdos_wildex(Z80 *R, JDOSMode mode)	/* Expand DOS wildcard */
{
	struct _finddata_t fblk;
	int m = 0;
	char c;
	long hfind;

	if (mode == JM_NEVER) return 0;

	do {
		c = R->fetchB((word)(R->getDE() + m));	/* Extract pathname */
		pathname[m] = c;
		++m;
	} while (c);

	fblk.attrib = FIND_ATTR;
	hfind = _findfirst(pathname, &fblk);
	
	if (!hfind) return 0;

	m = R->b;
	while (m > 0)
	{
		if (_findnext(hfind, &fblk)) return 0;
		--m;
	}
	
	/* Substitute filename into pathname */

	m = strlen(pathname) - 1;
	if (m < 0) return 0;

	while (pathname[m] != '\\' && 
	       pathname[m] != '/'  && 
               pathname[m] != ':'  && m >= 0) --m;	
	
	++m;
	strcpy(pathname + m, fblk.name);	/* Generated pathname */

	txt2fcb(R, R->getHL(), fblk.name);
	m = (-1);
	do {
		++m;
		R->storeB((word)(R->getHL() + m + 11), pathname[m]);
	} while (pathname[m]);
	return 1;
}
#undef FIND_ATTR

#endif	// def HAVE_WINDOWS_H
