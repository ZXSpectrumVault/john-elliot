/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2007  John Elliott

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
#include "psfio.h"
#include <ctype.h>

psf_errno_t psf_unicode_to_string(psf_unicode_dirent *chain, char **str)
{
	psf_unicode_dirent *n;
	int bytes = 1;
	char *txt;
	int inmulti = 0;
	char tokbuf[9];

	/* Each character takes at most 12 bytes */
	for (n = chain; n != NULL; n = n->psfu_next) bytes += 12;
	txt = malloc(bytes);
	if (!txt) return PSF_E_NOMEM;
	txt[0] = 0;
	for (n = chain; n != NULL; n = n->psfu_next)
	{
		sprintf(tokbuf, "%08lx", n->psfu_token);
		if (inmulti)
		{
			if (n->psfu_token == 0xFFFE) 
			{
				inmulti = 1;
				continue;
			}
			if (inmulti == 1) 
			{
				strcat(txt, "[");
				inmulti = 2;
			}
			strcat(txt, tokbuf);
			if ((n->psfu_next && n->psfu_next->psfu_token == 0xFFFE)
			|| n->psfu_next == NULL)
			{
				strcat(txt, "];");
			}
			else	strcat(txt, "+");
		}
		else
		{
			if (n->psfu_token == 0xFFFE) 
			{
				inmulti = 1;
				continue;
			}
			strcat(txt, "[");
			strcat(txt, tokbuf);
			strcat(txt, "];");
		}
	}	
	*str = txt;
	return PSF_E_OK;
}


/* Convert a string of the form 
 *
 *     [token];[token];...[token+token];[token+token];...
 *
 * to a Unicode chain.
 */

psf_errno_t psf_unicode_from_string(PSF_FILE *f, psf_word nchar, const char *str)
{
	int rv;
	psf_dword token;
	char *buf, *tok, *c;
	int inmulti = 0;

	if (nchar >= f->psf_length) return PSF_E_RANGE;

	buf = malloc(1 + strlen(str));
	if (!buf) return PSF_E_NOMEM;
	strcpy(buf, str);

	for (tok = strtok(buf, ";"); tok != NULL; tok = strtok(NULL, ";"))
	{
		if (strchr(tok, '+'))
		{
			tok++;
			inmulti = 1;
			rv = psf_unicode_add(f, nchar, 0xFFFE);
			if (rv) { free(buf); return rv; }
			while (isalnum(tok[0]))
			{
				if (!sscanf(tok, "%lx", &token))
				{
					free(buf);
					return PSF_E_PARSE;
				}
				rv = psf_unicode_add(f, nchar, token);
				if (rv) { free(buf); return rv; }
				c = strchr(tok, '+');
				if (c) tok = c + 1;
				else
				{
					c = strchr(tok, ']');
					if (!c) c = strchr(tok, ';');
					if (!c) c = tok + strlen(tok);
					tok = c;
				}	
			}
		}
		else	/* Single char */
		{
			if (inmulti) { free(buf); return PSF_E_PARSE; }
			if (!sscanf(tok + 1, "%lx", &token))
			{
				free(buf);
				return PSF_E_PARSE;
			}
			rv = psf_unicode_add(f, nchar, token);	
			if (rv) { free(buf); return rv; }
		}
	}
	free(buf);
	return PSF_E_OK;
}

