/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2006, 2007  John Elliott

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
#include "config.h"
#include <ctype.h>

#ifndef HAVE_STRICMP
 #ifdef HAVE_STRCMPI
  #define stricmp strcmpi
 #else
  #ifdef HAVE_STRCASECMP
   #define stricmp strcasecmp
  #else
   extern int stricmp(char *s, char *t);
  #endif
 #endif
#endif 

/* Maximum number of "external" codepages to load */
#define MAX_CODEPAGES 64

/* Codepage aliases: Different codepage numbers referring to the same 
 * codepage, or codepages with both numbers and names */

/* The mapping of codepage numbers to ISO 8859-x comes from isocp101.zip at
 * <http://www.kostis.net/freeware/indexe.htm> 
 */
static char *aliases[] = 
{
	"CP813",  "8859-7",
	"CP819",  "8859-1",
	"CP912",  "8859-2",
	"CP913",  "8859-3",
	"CP914",  "8859-4",
	"CP915",  "8859-5",
	"CP916",  "8859-8",
	"CP919",  "8859-10",
	"CP920",  "8859-9",
/* This one I'm not sure about. The documentation says "1089" but the actual
 * CPI file has "189" and I don't know which one is right and which one is
 * wrong */
/*	"CP1089", "8859-6", */

/* From <http://web.archive.org/web/20040225223721/http://www.sharmahd.com/tm/codepages.html> 
 * by way of Wikipedia. These values supposedly come from Windows SDK 
 * documentation. */
	"CP28591", "8859-1",
	"CP28592", "8859-2",
	"CP28593", "8859-3",
	"CP28594", "8859-4",
	"CP28595", "8859-5",
	"CP28596", "8859-6",
	"CP28597", "8859-7",
	"CP28599", "8859-8",
	"CP28599", "8859-9",
	"CP28600", "8859-10",
	"CP28603", "8859-13",
	"CP28604", "8859-14",
	"CP28605", "8859-15",
	"CP28606", "8859-16",
	NULL,     NULL
};

/* Moved this outside the #ifdef __PACIFIC__; I'm allowing other platforms
 * to load .CP2 codepages. To create them, use page2cp2 on a text file
 * (eg: page2cp2 CP437.TXT CP850.TXT  creates CP437.CP2 and CP850.CP2) */
static PSF_MAPPING *codepages[MAX_CODEPAGES];

/* In DOS on an 8086, we don't care about unaligned access or byte order;
 * to read an Intel-format dword, just point and read. */
#ifdef __PACIFIC__
#define peek4(b)  (*((psf_dword *)(b)))
#else
/* Otherwise a little more care is needed. */
static inline psf_dword peek4(psf_byte *b)
{
	return	((psf_dword)b[0])         | 
		(((psf_dword)b[1]) << 8)  |
		(((psf_dword)b[2]) << 16) |
		(((psf_dword)b[3]) << 24);
}
#endif


/* Allocate a codepage with room for 'count' entries and 'tokens' tokens */
int cp_alloc(psf_dword count, psf_dword tokens, const char *cpname)
/* Allocate space for the header and data */
{
	int n, m;
	unsigned memreq;
	psf_dword *value;

/* See if we have a free slot */
	for (n = 0; n < MAX_CODEPAGES; n++) if (codepages[n] == NULL) break;
	if (n >= MAX_CODEPAGES) return -1;

/* Work out how big it should be. */
	memreq =  count * sizeof(psf_dword);
	memreq += sizeof(PSF_MAPPING);
	memreq += tokens * sizeof(psf_dword *);
	memreq += 1 + strlen(cpname);
	codepages[n] = malloc(memreq);
	if (!codepages[n]) return -1;
/* Set up the pointers */
	codepages[n]->psfm_name = (((char *)codepages[n])
		+ sizeof(PSF_MAPPING)
		+ tokens * sizeof(psf_dword *));
	strcpy(codepages[n]->psfm_name, cpname);
	codepages[n]->psfm_count = tokens;
	value = (psf_dword *)(codepages[n]->psfm_name + 1 + strlen(cpname));
	for (m = 0; m < tokens; m++)
	{
		codepages[n]->psfm_tokens[m] = value;
	}
	return n;
}


/* Loads a mapping file in CP2 format. Returns the mapping if successful,
 * NULL if error. cp2name is the pathname of the codepage while cpname
 * is its textual name in the program. */
static PSF_MAPPING *psf_load_cp2(char *pathname, char *cpname)
{
	int n, m;
	FILE *fp;
	psf_dword tokens;
	psf_dword count;
	psf_dword *value;
	psf_byte header[128];

/* See if we have a free slot */
	for (n = 0; n < MAX_CODEPAGES; n++) if (codepages[n] == NULL) break;
	if (n >= MAX_CODEPAGES) return NULL;
/* OK. Now try to load it */
	fp = fopen(pathname, "rb");
	if (fp == NULL) return NULL;
	if (fread(header, 1, 128, fp) < 128)
	{
		fclose(fp);
		return NULL;
	}
	if (!strcmp((const char *)header, "PSFTOOLS CODEPAGE MAP\r\n\032"))
	{
/* Fixed 256 entries */
		tokens = 256;
	}
	else if (!strcmp((const char *)header, "PSFTOOLS CODEPAGE MAP 2\r\n\032"))
	{
/* Number of entries defined in file */
		tokens = peek4(header + 0x44);
	}
	else
	{
		fclose(fp);
		return NULL;
	}
/* Retrieve the count of Unicode characters */
	count = peek4(header + 0x40);
/* Allocate space for the header and data */
	n = cp_alloc(count, tokens, cpname);
	if (n < 0) return NULL;
	value = codepages[n]->psfm_tokens[0];
/* Load the data */
	for (m = 0; m < tokens; )
	{
		if (fread(header, 1, 4, fp) < 4)
		{
			*value = 0xFFFF;
			m++;
			value++;
			if (m < tokens) codepages[n]->psfm_tokens[m] = value;
		}
		else
		{
			*value = peek4(header);
			if (*value == 0xFFFF) 
			{
				++m;
				value++;	
				if (m < tokens) codepages[n]->psfm_tokens[m] = value;
			}
			else value++;	
		}
	}
	fclose(fp); 
	return codepages[n];
}

void addtoken(int pass, int ncp, psf_dword *count, 
		psf_dword **ptr, psf_dword value)
{
	if (pass == 1) 
	{
		(*ptr)[0] = value;
		++(*ptr);
	}
	else
	{
		++(*count);
	}
}

static PSF_MAPPING *psf_load_uni(char *pathname, char *cpname)
{
	char linebuf[100];
	psf_dword count, uni;
	FILE *fp;
	int pass, from, to, ncp = -1;
	psf_dword *ptr = NULL;
	char *tmp, *p, *right_half;
	int lineno;
	int tokens;

	count = 0;
	tokens = 0;
	for (pass = 0; pass < 2; pass++)
	{
		switch(pass)
		{
			case 0: count = 0; 
				break;
			case 1: ncp = cp_alloc(count, tokens, cpname);
				if (ncp < 0) return NULL;
				count = 0; 
				ptr = codepages[ncp]->psfm_tokens[0];
				break;
		}
		addtoken(pass, ncp, &count, &ptr, 0xFFFF);
		fp = fopen(pathname, "r");
		if (!fp) return NULL;
		lineno = 0;

/* Parse each line of the file. Format is:
 *
 * <line> = <character spec> <whitespace> <unicode>
 *
 * A character spec is either "0xnn" or "0xnn-0xnn"
 * A unicode spec is one of:
 * 		idem
 * 		0xnnnn
 * 		U+nnnn U+nnnn ....
 * 		U+nnnn U+nnnn .... U*nnnn U*nnnn ....
 * */
		while (fgets(linebuf, sizeof(linebuf), fp))
		{
			++lineno;
			p = strchr(linebuf, '#');
			if (p) *p = 0;
/* Chop off any trailing \n or whitespace */
			p = linebuf + strlen(linebuf) - 1;
			while (p >= linebuf &&
			      (p[0] == '\n' || 
			       p[0] == ' '  || 
			       p[0] == '\t'))
			{
				p[0] = 0;
				--p;
			}
			if (linebuf[0] == 0) continue;	/* Blank line */
			if (linebuf[0] != '0') 
			{
				fprintf(stderr, "%s: File format error on line %d\n", pathname, lineno);
				fprintf(stderr, "Line does not start with a '0'.\n");
				fclose(fp);
				return NULL;
			}
/* Now split the two halves. */
			++tokens;
			p = linebuf;
			while (p[0] != ' ' && p[0] != '\t' && p[0] != 0)
			{
				++p;	
			}
			if (*p)
			{
				*p = 0;
				do
				{
					++p;
				} while (*p == ' ' || *p == '\t');
				right_half = p;	
/* Make the right-hand half all lower-case. */
				for (; *p; ++p)
				{
					*p = tolower(*p);
				}
			}
			else	right_half = p;
/* Now parse out the character IDs from the left half */
			if (strchr(linebuf, '-'))
			{
				if (sscanf(linebuf, "0x%x-0x%x", &from, 
							&to) < 2)
					goto numerr;
			}
			else
			{
				if (sscanf(linebuf, "0x%x", &from) < 1)
				{
numerr:
					fprintf(stderr, "%s: File format error on line %d\n", pathname, lineno);
					fprintf(stderr, "Character range not formed 0xnn or 0xnn-0xnn.\n");
					fclose(fp);
					return NULL;
				}
				to = from;
			}
			while(from <= to && from < codepages[ncp]->psfm_count) 
			{
				if (pass == 1)
				{
					codepages[ncp]->psfm_tokens[from] = ptr;
				}
				p = right_half;
				tmp = strstr(p, "0x");
				if (strstr(p, "idem"))
				{
					addtoken(pass, ncp, &count, &ptr, from);
				}
/* Check for a unicode.org format line: 0xnn  0xnnnn\n */
				else if (tmp)
				{
					if (sscanf(tmp, "0x%lx", &uni) < 1)
					{
						fprintf(stderr, "%s: File format error on line %d\n", pathname, lineno);
						fprintf(stderr, "'%s' starts 0x but is not a hex number.\n", tmp);
						fclose(fp);
						return NULL;
					}
					addtoken(pass, ncp, &count, &ptr, uni);
				}
/* Check for a .uni format line: 0xnn  U+nnnn U+nnnn ... \n */
				else while ((tmp = strstr(p, "u+")))
				{
					if (sscanf(tmp, "u+%lx", &uni) < 1)
					{
						fprintf(stderr, "%s: File format error on line %d\n", pathname, lineno);
						fprintf(stderr, "'%s' starts with U+ but is not a hex number.\n", tmp);
						fclose(fp);
						return NULL;
					}
					addtoken(pass, ncp, &count, &ptr, uni);
					p = tmp + 1;
				}
				if (strstr(p, "u*"))
				{
					addtoken(pass, ncp, &count, &ptr, 0x1FFFFL);
					while ((tmp = strstr(p, "u*")))
					{
						if (sscanf(tmp, "u*%lx", &uni) == 0)
						{
							fprintf(stderr, "%s: File format error on line %d\n", pathname, lineno);
							fprintf(stderr, "'%s' starts with U* but is not a hex number.\n", tmp);
							fclose(fp);
							return NULL;
						}
						addtoken(pass, ncp, &count, &ptr, uni);
						p = tmp + 1;
					}
				}
				addtoken(pass, ncp, &count, &ptr, 0xFFFFL);
				++from;
			}
		}

		fclose(fp);
	}
	return codepages[ncp];
}

/* Load a CP file in whatever formats are supported */
static PSF_MAPPING *psf_load_cp(char *pathname, char *cpname)
{
	PSF_MAPPING *m;
	m = psf_load_cp2(pathname, cpname);
	if (m) return m;
	m = psf_load_uni(pathname, cpname);
	return m;
}

#ifdef __PACIFIC__
/* 16-bit DOS hasn't got enough memory to keep all the codepages compiled 
 * into all the programs; so load them from disk as required. 
 *
 * PSFTOOLS 1.0.4 allows other platforms to load CP2 files as a secondary
 * source of codepage information.
 *
 * Format of a codepage .CP2 file is:
 * 
 * -- First 128 bytes are the header.
 *    0000-0019: Magic number, "PSFTOOLS CODEPAGE MAP\r\n\032\000"
 *    0040-0043: Number of long integers that follow the header
 *              (little-endian)
 *    All other bytes 0 (reserved)
 *
 * -- Remainder of file is 256 sequences of long integers, each terminated
 *    with 0x0000FFFF. Again, little-endian.
 * 
 * PSFTOOLS 1.1.0 uses a slightly different format:
 * 
 * -- First 128 bytes are the header.
 *    0000-001B: Magic number, "PSFTOOLS CODEPAGE MAP 2\r\n\032\000"
 *    0040-0043: Number of long integers that follow the header
 *              (little-endian)
 *    0044-0047: Number of overall sequences (always 256 in the old version) 
 *    All other bytes 0 (reserved)
 *
 * -- Remainder of file is the stated number of sequences of long integers, 
 *    each terminated with 0x0000FFFF. Again, little-endian.
 *
 */
#include <dos.h>


typedef unsigned char far *LPBYTE;
typedef unsigned short far *LPWORD;

/* Try to find the directory with our program files */
static int psf_lib_dir(char *path)
{
	union REGS rg;
	LPWORD envp;
	LPBYTE env;
	int n;

/* Check we have DOS 3.0 or later */
	rg.x.ax = 0x3000;
	intdos(&rg, &rg);
	if (rg.h.al < 3) return -1;

/* Now find the environment. Start by getting the PSP address */
	rg.x.ax = 0x6200;
	intdos(&rg, &rg);
/* PSP:2C holds the segment of the environment */
	envp = (LPWORD)MK_FP(rg.x.bx, 0x2C);
	env  = (LPBYTE)MK_FP(*envp, 0);
/* Environment is terminated by two zero bytes */
	while (env[0] != 0 || env[1] != 0) ++env;

/* Skip the two zeroes and the next two bytes */
	env += 4;
	n = 0;
/* And env now points at the program path */
	while (*env) path[n++] = *env++;
	path[n] = 0;
	--n;
	while (n >= 0)
	{
		if (path[n] == '\\' || path[n] == '/' || path[n] == ':')
		{
			path[n + 1] = 0;
			break;
		}
		--n;
	}
	if (n < 0) return -1;
	return 0;
}



PSF_MAPPING *psf_find_mapping(char *name)
{
	int n;
	char **p;
	char path[256];
	char cpname[20];
	PSF_MAPPING *map;

/* See if the name is an alias */
	for (p = aliases; *p; p += 2)
	{
		if (!stricmp(p[0], name))
		{
			name = p[1];
			break;
		}
		if (p[0][0] == 'C' && p[0][1] == 'P' && !stricmp(p[0]+2, name))
		{
			name = p[1];
			break;
		}
	}	
/* See if the codepage is one of those already loaded */
	for (n = 0; n < MAX_CODEPAGES; n++)
	{
		if (!stricmp(codepages[n]->psfm_name, name)) 
			return codepages[n];

	}
	if (isdigit(name[0])) for (n = 0; n < MAX_CODEPAGES; n++)
	{
		if (codepages[n]->psfm_name[0] == 'C' &&
		    codepages[n]->psfm_name[1] == 'P' &&
		    !stricmp(codepages[n]->psfm_name, name)) 
			return codepages[n];
	}
/* Try to load the codepage. */
/* [1.0.4] If it's an absolute pathname, then don't bother searching or
 * prepending "CP".*/
	if (strchr(name, ':') || strchr(name, '.') || strchr(name, '/') ||
	    strchr(name, '\\'))
	{
		return psf_load_cp(name, name);
	}
	else
	{
		if (psf_lib_dir(path)) return NULL;
/* If the name is big enough to overflow our buffers it definitely isn't
 * valid */
		if (strlen(path) + strlen(name) + 8 > sizeof(path)) return NULL;
		if (strlen(name) + 3 > sizeof(cpname)) return NULL;
		strcat(path, name);
		strcpy(cpname, name);
		strcat(path, ".cp2");
		map = psf_load_cp(path, cpname);
		if (map == NULL && isdigit(name[0]))
		{
			if (psf_lib_dir(path)) return NULL;
			strcpy(cpname, "CP");
			strcat(cpname, name);
			strcat(path, "CP");
			strcat(path, name);
			strcat(path, ".cp2");
			map = psf_load_cp(path, cpname);
		}
	}
	return map;
}

void psf_list_mappings(FILE *fp)
{
	char path[256];
	char buf[256];
	union REGS rg;
	struct SREGS sg;
	char cpid[9], *sp, **p;

	if (!psf_lib_dir(path))
	{
		strcat(path, "*.CP2");
/* Set DTA to the buffer */
		rg.x.ax = 0x1A00;
		rg.x.dx = FP_OFF(buf);
		sg.ds   = FP_SEG(buf);
		intdosx(&rg, &rg, &sg);
/* Findfirst */
		rg.x.ax = 0x4E00;
		rg.x.cx = 0x27;
		rg.x.dx = FP_OFF(path);
		sg.ds   = FP_SEG(path);
		intdosx(&rg, &rg, &sg);
		while (!rg.x.cflag)
		{
			sprintf(cpid, "%-8.8s", buf + 0x1E);
			sp = strrchr(cpid, '.'); if (sp) *sp = 0;
			sp = strrchr(cpid, ' '); if (sp) *sp = 0;
			fprintf(fp, "%s ", cpid);		
			rg.x.ax = 0x4F00;
			intdos(&rg, &rg);
		}
	}		

	for (p = aliases; *p; p += 2)
	{
		fprintf(fp, "%s ", *p);
	}
}

#else

extern PSF_MAPPING m_8859_1,  m_8859_2,  m_8859_3,  m_8859_4,  m_8859_5,  
	           m_8859_6,  m_8859_7,  m_8859_8,  m_8859_9,  m_8859_10, 
		   m_8859_13, m_8859_14, m_8859_15, m_8859_16, 
		   m_CP367,   m_CP437,   m_CP737,   m_CP775, 
		   m_CP850,   m_CP852, /*m_CP853,*/ m_CP855,   m_CP857,
		   m_CP858, 
	           m_CP860,   m_CP861,   m_CP862,   m_CP863,   m_CP864,
	           m_CP865,   m_CP866,   m_CP869,   m_CP874, 
		   m_CP1250,  m_CP1251,  m_CP1252,  m_CP1253,  m_CP1254,
		   m_CP1255,  m_CP1256,  m_CP1257,  m_CP1258, 
		   
		   m_AMSTRAD, m_LS3,       m_QX10,  m_PRINTIT,
		   m_PCGEM,   m_BBCMICRO,  m_PPC437,
		   m_PPC860,  m_PPC865, m_PPCGREEK;

static PSF_MAPPING *builtin_codepages[] =
{
&m_8859_1,  
&m_8859_2,  
&m_8859_3,
&m_8859_4,
&m_8859_5,
&m_8859_6, 
&m_8859_7, 
&m_8859_8,
&m_8859_9,
&m_8859_10, 
&m_8859_13,
&m_8859_14,
&m_8859_15,
&m_8859_16, 
&m_AMSTRAD,
&m_BBCMICRO,
&m_CP367,
&m_CP437,
&m_CP737,
&m_CP775, 
&m_CP850,
&m_CP852,
/*&m_CP853, */
&m_CP855,
&m_CP857,
&m_CP858,
&m_CP860, 
&m_CP861,
&m_CP862,
&m_CP863,
&m_CP864,
&m_CP865, 
&m_CP866,
&m_CP869,
&m_CP874,
&m_CP1250,
&m_CP1251, 
&m_CP1252, 
&m_CP1253,
&m_CP1254,
&m_CP1255,
&m_CP1256, 
&m_CP1257, 
&m_CP1258,
&m_LS3,
&m_QX10,
&m_PRINTIT,
&m_PCGEM,
&m_PPC437,
&m_PPC860,
&m_PPC865,
&m_PPCGREEK,
NULL };


/* Find an ASCII->Unicode mapping */
PSF_MAPPING *psf_find_mapping(char *name)
{
	char **p;
	PSF_MAPPING **m;
	PSF_MAPPING *map;

/* See if the name matches an external codepage that has already been
 * loaded. */
	for (m = codepages; *m; m++) 
	{
		if (!stricmp((*m)->psfm_name, name)) return *m;
	}
/* See if the name looks like a filename. Our compiled-in codepages only
 * contain letters, numbers and dashes, so any punctuation that looks 
 * vaguely filename-like is highly suspicious. */ 
	if (strchr(name, '.') || strchr(name, '/') || strchr(name, '\\') ||
	    strchr(name, ':'))
	{
		map = psf_load_cp(name, name);
		if (map) return map;
		/* In the future, other codepage formats (eg, unicode.org
		 * mapping) should be supported here. */
	}
/* See if the name is an alias */
	for (p = aliases; *p; p += 2)
	{
		if (!stricmp(p[0], name))
		{
			name = p[1];
			break;
		}
		if (p[0][0] == 'C' && p[0][1] == 'P' && !stricmp(p[0]+2, name))
		{
			name = p[1];
			break;
		}
	}	

	for (m = builtin_codepages; *m; m++) 
	{
		if (!stricmp((*m)->psfm_name, name)) return *m;
	}
/* See if a codepage was supplied as just a number */
	if (isdigit(name[0])) for (m = builtin_codepages; *m; m++) 
	{
		if ((*m)->psfm_name[0] == 'C' &&
		    (*m)->psfm_name[1] == 'P' &&
		    !stricmp((*m)->psfm_name + 2, name)) return *m;
	}
/* And if all else fails hit the disk again */
	return psf_load_cp(name, name);
}


void psf_list_mappings(FILE *fp)
{
	PSF_MAPPING **m;
	char **p;

	for (m = builtin_codepages; *m; m++) 
	{
		fprintf(fp, "%s ", (*m)->psfm_name);
	}
	for (m = codepages; *m; m++) 
	{
		fprintf(fp, "%s ", (*m)->psfm_name);
	}
	for (p = aliases; *p; p += 2)
	{
		fprintf(fp, "%s ", *p);
	}
}
#endif

