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
#include "keyboard.hxx"

#ifdef UNIX
void txt2fcb(Z80 *R, word addr, char *s);

void alert(const string s)
{
	fprintf(stderr, "%s\n", s.c_str());
}


word jdos_wildex(Z80 *R, JDOSMode mode)
{
	char pathname[4 * _POSIX_PATH_MAX];
	int m = 0, n = 0;
	char c;
	glob_t results;

	if (mode == JM_NEVER) return 0;

        do {
                c = R->fetchB(R->getDE() + m); /* Extract pathname */
                pathname[m] = c;
                ++m;
        } while (c);

	/* Case-insensitivity (from v1.31) removed, now that utilities have
          been rewritten for case-sensitivity */

	if (glob(pathname, GLOB_MARK, NULL, &results)) return 0;
	
        m = R->b;

	for (n = 0; n < (int)results.gl_pathc; n++)
        {	
		char *s = results.gl_pathv[n];

		/* Skip directories */
		if (s[0] && s[strlen(s) - 1] == '/') continue;
		if (m == 0) break;  
		--m;
        }
	/* Was a match found? */
	if ((unsigned)n >= results.gl_pathc) { globfree(&results); return 0; }

	/* Work out what we got. */

        txt2fcb(R, R->getHL(), results.gl_pathv[n]);
        m = (-1);
        do {
                ++m;
                R->storeB(R->getHL() + m + 11, results.gl_pathv[n][m]);
        } while (results.gl_pathv[n][m]);

	globfree(&results);

	return 1;
}


#endif	// def UNIX

