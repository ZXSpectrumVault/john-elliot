/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2003, 2017  John Elliott <seasip.webmaster@gmail.com>  *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

#include "drvi.h"

/* Translation between UTF-8 (as used by LibDsk in LDBS) and codepage 437
 * (as used by QRST, ApriDisk etc.) 
 */

/* Mapping of code page 437 to UTF-8 sequences (high half of codepage only) */
static const unsigned char cp437[129][4] =
{
	{0xE2, 0x8C, 0x82, 0x00 }, /* 7f */
	{0xc3, 0x87, 0x00 },	/* 80 */ 
	{0xc3, 0xbc, 0x00 },	/* 81 */ 
	{0xc3, 0xa9, 0x00 },	/* 82 */ 
	{0xc3, 0xa2, 0x00 },	/* 83 */ 
	{0xc3, 0xa4, 0x00 },	/* 84 */ 
	{0xc3, 0xa0, 0x00 },	/* 85 */ 
	{0xc3, 0xa5, 0x00 },	/* 86 */ 
	{0xc3, 0xa7, 0x00 },	/* 87 */ 
	{0xc3, 0xaa, 0x00 },	/* 88 */ 
	{0xc3, 0xab, 0x00 },	/* 89 */ 
	{0xc3, 0xa8, 0x00 },	/* 8a */ 
	{0xc3, 0xaf, 0x00 },	/* 8b */ 
	{0xc3, 0xae, 0x00 },	/* 8c */ 
	{0xc3, 0xac, 0x00 },	/* 8d */ 
	{0xc3, 0x84, 0x00 },	/* 8e */ 
	{0xc3, 0x85, 0x00 },	/* 8f */ 
	{0xc3, 0x89, 0x00 },	/* 90 */ 
	{0xc3, 0xa6, 0x00 },	/* 91 */ 
	{0xc3, 0x86, 0x00 },	/* 92 */ 
	{0xc3, 0xb4, 0x00 },	/* 93 */ 
	{0xc3, 0xb6, 0x00 },	/* 94 */ 
	{0xc3, 0xb2, 0x00 },	/* 95 */ 
	{0xc3, 0xbb, 0x00 },	/* 96 */ 
	{0xc3, 0xb9, 0x00 },	/* 97 */ 
	{0xc3, 0xbf, 0x00 },	/* 98 */ 
	{0xc3, 0x96, 0x00 },	/* 99 */ 
	{0xc3, 0x9c, 0x00 },	/* 9a */ 
	{0xc2, 0xa2, 0x00 },	/* 9b */ 
	{0xc2, 0xa3, 0x00 },	/* 9c */ 
	{0xc2, 0xa5, 0x00 },	/* 9d */ 
	{0xe2, 0x82, 0xa7, 0x00 },	/* 9e */ 
	{0xc6, 0x92, 0x00 },	/* 9f */ 
	{0xc3, 0xa1, 0x00 },	/* a0 */ 
	{0xc3, 0xad, 0x00 },	/* a1 */ 
	{0xc3, 0xb3, 0x00 },	/* a2 */ 
	{0xc3, 0xba, 0x00 },	/* a3 */ 
	{0xc3, 0xb1, 0x00 },	/* a4 */ 
	{0xc3, 0x91, 0x00 },	/* a5 */ 
	{0xc2, 0xaa, 0x00 },	/* a6 */ 
	{0xc2, 0xba, 0x00 },	/* a7 */ 
	{0xc2, 0xbf, 0x00 },	/* a8 */ 
	{0xe2, 0x8c, 0x90, 0x00 },	/* a9 */ 
	{0xc2, 0xac, 0x00 },	/* aa */ 
	{0xc2, 0xbd, 0x00 },	/* ab */ 
	{0xc2, 0xbc, 0x00 },	/* ac */ 
	{0xc2, 0xa1, 0x00 },	/* ad */ 
	{0xc2, 0xab, 0x00 },	/* ae */ 
	{0xc2, 0xbb, 0x00 },	/* af */ 
	{0xe2, 0x96, 0x91, 0x00 },	/* b0 */ 
	{0xe2, 0x96, 0x92, 0x00 },	/* b1 */ 
	{0xe2, 0x96, 0x93, 0x00 },	/* b2 */ 
	{0xe2, 0x94, 0x82, 0x00 },	/* b3 */ 
	{0xe2, 0x94, 0xa4, 0x00 },	/* b4 */ 
	{0xe2, 0x95, 0xa1, 0x00 },	/* b5 */ 
	{0xe2, 0x95, 0xa2, 0x00 },	/* b6 */ 
	{0xe2, 0x95, 0x96, 0x00 },	/* b7 */ 
	{0xe2, 0x95, 0x95, 0x00 },	/* b8 */ 
	{0xe2, 0x95, 0xa3, 0x00 },	/* b9 */ 
	{0xe2, 0x95, 0x91, 0x00 },	/* ba */ 
	{0xe2, 0x95, 0x97, 0x00 },	/* bb */ 
	{0xe2, 0x95, 0x9d, 0x00 },	/* bc */ 
	{0xe2, 0x95, 0x9c, 0x00 },	/* bd */ 
	{0xe2, 0x95, 0x9b, 0x00 },	/* be */ 
	{0xe2, 0x94, 0x90, 0x00 },	/* bf */ 
	{0xe2, 0x94, 0x94, 0x00 },	/* c0 */ 
	{0xe2, 0x94, 0xb4, 0x00 },	/* c1 */ 
	{0xe2, 0x94, 0xac, 0x00 },	/* c2 */ 
	{0xe2, 0x94, 0x9c, 0x00 },	/* c3 */ 
	{0xe2, 0x94, 0x80, 0x00 },	/* c4 */ 
	{0xe2, 0x94, 0xbc, 0x00 },	/* c5 */ 
	{0xe2, 0x95, 0x9e, 0x00 },	/* c6 */ 
	{0xe2, 0x95, 0x9f, 0x00 },	/* c7 */ 
	{0xe2, 0x95, 0x9a, 0x00 },	/* c8 */ 
	{0xe2, 0x95, 0x94, 0x00 },	/* c9 */ 
	{0xe2, 0x95, 0xa9, 0x00 },	/* ca */ 
	{0xe2, 0x95, 0xa6, 0x00 },	/* cb */ 
	{0xe2, 0x95, 0xa0, 0x00 },	/* cc */ 
	{0xe2, 0x95, 0x90, 0x00 },	/* cd */ 
	{0xe2, 0x95, 0xac, 0x00 },	/* ce */ 
	{0xe2, 0x95, 0xa7, 0x00 },	/* cf */ 
	{0xe2, 0x95, 0xa8, 0x00 },	/* d0 */ 
	{0xe2, 0x95, 0xa4, 0x00 },	/* d1 */ 
	{0xe2, 0x95, 0xa5, 0x00 },	/* d2 */ 
	{0xe2, 0x95, 0x99, 0x00 },	/* d3 */ 
	{0xe2, 0x95, 0x98, 0x00 },	/* d4 */ 
	{0xe2, 0x95, 0x92, 0x00 },	/* d5 */ 
	{0xe2, 0x95, 0x93, 0x00 },	/* d6 */ 
	{0xe2, 0x95, 0xab, 0x00 },	/* d7 */ 
	{0xe2, 0x95, 0xaa, 0x00 },	/* d8 */ 
	{0xe2, 0x94, 0x98, 0x00 },	/* d9 */ 
	{0xe2, 0x94, 0x8c, 0x00 },	/* da */ 
	{0xe2, 0x96, 0x88, 0x00 },	/* db */ 
	{0xe2, 0x96, 0x84, 0x00 },	/* dc */ 
	{0xe2, 0x96, 0x8c, 0x00 },	/* dd */ 
	{0xe2, 0x96, 0x90, 0x00 },	/* de */ 
	{0xe2, 0x96, 0x80, 0x00 },	/* df */ 
	{0xce, 0xb1, 0x00 },	/* e0 */ 
	{0xc3, 0x9f, 0x00 },	/* e1 */ 
	{0xce, 0x93, 0x00 },	/* e2 */ 
	{0xcf, 0x80, 0x00 },	/* e3 */ 
	{0xce, 0xa3, 0x00 },	/* e4 */ 
	{0xcf, 0x83, 0x00 },	/* e5 */ 
	{0xc2, 0xb5, 0x00 },	/* e6 */ 
	{0xcf, 0x84, 0x00 },	/* e7 */ 
	{0xce, 0xa6, 0x00 },	/* e8 */ 
	{0xce, 0x98, 0x00 },	/* e9 */ 
	{0xce, 0xa9, 0x00 },	/* ea */ 
	{0xce, 0xb4, 0x00 },	/* eb */ 
	{0xe2, 0x88, 0x9e, 0x00 },	/* ec */ 
	{0xcf, 0x86, 0x00 },	/* ed */ 
	{0xce, 0xb5, 0x00 },	/* ee */ 
	{0xe2, 0x88, 0xa9, 0x00 },	/* ef */ 
	{0xe2, 0x89, 0xa1, 0x00 },	/* f0 */ 
	{0xc2, 0xb1, 0x00 },	/* f1 */ 
	{0xe2, 0x89, 0xa5, 0x00 },	/* f2 */ 
	{0xe2, 0x89, 0xa4, 0x00 },	/* f3 */ 
	{0xe2, 0x8c, 0xa0, 0x00 },	/* f4 */ 
	{0xe2, 0x8c, 0xa1, 0x00 },	/* f5 */ 
	{0xc3, 0xb7, 0x00 },	/* f6 */ 
	{0xe2, 0x89, 0x88, 0x00 },	/* f7 */ 
	{0xc2, 0xb0, 0x00 },	/* f8 */ 
	{0xe2, 0x88, 0x99, 0x00 },	/* f9 */ 
	{0xc2, 0xb7, 0x00 },	/* fa */ 
	{0xe2, 0x88, 0x9a, 0x00 },	/* fb */ 
	{0xe2, 0x81, 0xbf, 0x00 },	/* fc */ 
	{0xc2, 0xb2, 0x00 },	/* fd */ 
	{0xe2, 0x96, 0xa0, 0x00 },	/* fe */ 
	{0xc2, 0xa0, 0x00 },	/* ff */ 
};


static int append(const char *src, int len, char *dst, int *dpos, int *limit)
{
	while (len)
	{
		/* Append a character */
		if (dst)
		{
			dst[*dpos] = *src;
		}
		/* If we're limiting... */
		if (limit[0] > 0)
		{
			/* Decrease limit */
			--limit[0];
			/* If it's now zero, that's as full as the buffer
			 * can get. Terminate it and return. */
			if (!limit[0])
			{
				if (dst)
				{
					dst[*dpos] = 0;
				}
				return 0;
			}
		}
		++src;
		++(*dpos);
		--len;
	}
	return 1;
}


/* Convert a CP437 string to UTF-8. Returns the number of UTF-8 bytes 
 * generated. If 'dst' is not null, the string is written to 'dst'. 
 * 
 * If 'limit' is >= 0, it is the maximum size of the output buffer 
 * (including the terminating null) */
int cp437_to_utf8(const char *src, char *dst, int limit)
{
	int count = 0;
	char sc;
	int l;
	int dpos = 0;

	while (*src)
	{
		sc = *src++;

		if (sc < 0x7f)	/* Characters below 0x7F map as single bytes */
		{
			count++;
			if (!append(&sc, 1, dst, &dpos, &limit))
			{
				return count;
			}
		}
		else
		{
			l = strlen((char *)cp437[sc - 0x7f]);
			count += l;
			if (!append((char *)cp437[sc - 0x7f], l, dst, &dpos, &limit))
			{
				return count;
			}
		}
	}
	/* Add a terminating zero */
	sc = 0;
	count++;
	append(&sc, 1, dst, &dpos, &limit);
	return count;
}


/* Match UTF8 token 't' against the UTF8 dictionary for CP437. */
static unsigned char findtoken(const unsigned char *t)
{
	int n;

	for (n = 0; n < 129; n++)
	{
		if (!strcmp((char *)cp437[n], (char *)t)) return 127 + n;
	}
	return 0xFF;
}


/* Convert a UTF-8 string to CP437. Returns the number of CP437 bytes
 * generated. Characters outside the CP437 range get replaced with 0xFF. 
 * If 'dst' is not null, the string is written to 'dst'.
 * 
 * If 'limit' is >= 0, it is the maximum size of the output buffer 
 * (including the terminating null) */
int utf8_to_cp437(const char *src, char *dst, int limit)
{
	int count = 0;
	unsigned char token[5];
	unsigned char result;
	int dpos = 0;

	while (*src)
	{
		token[0] = *src++;
		result = 0xFF;

		if (token[0] < 0x7F) /* Characters below 0x7F map */
		{		  /* as single bytes */
			count++;
			result = token[0];
		}
		else if (token[0] >= 0x7F && token[0] < 0xC2)
		{
			result = 0xFF;	/* DEL, stray continuation byte,
					 * or overlong sequence */
		}
		else if (token[0] >= 0xC2 && token[0] < 0xE0)
		{
/* 2-byte sequence */
			token[1] = *src++;
			token[2] = 0;
			result = findtoken(token);
		}
		else if (token[0] >= 0xE0 && token[0] < 0xF0)
		{
/* 3-byte sequence */
			token[1] = *src++;
			token[2] = *src++;
			token[3] = 0;
			result = findtoken(token);
		}
		else if (token[0] >= 0xF0)
		{
/* 4-byte sequence */
			token[1] = *src++;
			token[2] = *src++;
			token[3] = *src++;
			token[0] = 0;
			result = findtoken(token);
		}
		if (!append((char *)&result, 1, dst, &dpos, &limit))
		{
			return count;
		}
	}
	/* Add a terminating zero */
	result = 0;
	count++;
	append((char *)&result, 1, dst, &dpos, &limit);
	return count;
}




