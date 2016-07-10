/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005  John Elliott

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

/* Merge one or more PSF files into a new PSF file */

#include "cnvmulti.h"
#include "psflib.h"
#include <ctype.h>


static char helpbuf[2048];
static char msgbuf[1000];
static int v1 = 0;
static PSF_FILE *psf_in, psf_out;

/* Program name */
char *cnv_progname = "PSFMERGE";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "psf1")) { v1 = 1; return NULL; }
	if (!stricmp(variable, "psf2")) { v1 = 0; return NULL; }
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
    sprintf(helpbuf, "Combines Unicode .PSF fonts\n\n"
		     "Syntax: %s source.psf source.psf "
		            " ... target.psf \n\n", cnv_progname);
    strcat(helpbuf,  "Options are:\n");
    strcat(helpbuf,  "             --psf1: Output in PSF1 format\n");
    strcat(helpbuf,  "             --psf2: Output in PSF2 format (default)\n");
    return helpbuf;
} 

/* Start of program */

/* This is similar to the Unicode buffer management in psfucs.c, but instead
 * of the fixed number of characters in a PSF file, we have a variable number
 * of characters tracked by unicodes_count. 
 *
 * unicodes[] holds unicodes_count head pointers, and can hold unicodes_max
 * of them. 
 * bufchain holds the Unicode entry buffers that have been allocated. 
 * freechain is the list of free entries in bufchain. 
 * dirents_nfree is the count of entries in freechain. */
static psf_unicode_dirent **unicodes = NULL;
static int unicodes_max = 0, unicodes_count = 0;
static psf_unicode_buffer *bufchain = NULL;
static psf_unicode_dirent *freechain = NULL;
static int dirents_nfree = 0;

/* Free all the above structures */
void free_buffers()
{
	psf_unicode_buffer *nbuf;
	void *v;
	nbuf = bufchain;
	while (nbuf)
	{
		v = nbuf;
		nbuf = nbuf->psfb_next;
		free(v);
	}
	bufchain = NULL;
	freechain = NULL;
	dirents_nfree = 0;
	free(unicodes);
	unicodes = NULL;
	unicodes_max = unicodes_count = 0;
}

/* Convert a Unicode token chain to text form, for ease of comparison
 * and searching */
static char *chain2str(psf_unicode_dirent *chain)
{
	char *ptr;

	if (psf_unicode_to_string(chain, &ptr)) return NULL;
	return ptr;
}


/* Allocate a psf_unicode_dirent */
psf_unicode_dirent *newentry(psf_dword token)
{
	psf_unicode_buffer *nbuf;
	psf_unicode_dirent *ude;

	if (!dirents_nfree)
	{
		nbuf = psf_malloc_unicode_buffer();
		if (!nbuf) return NULL;
		nbuf->psfb_next = bufchain;
		bufchain = nbuf;
		nbuf->psfb_dirents[0].psfu_next = freechain;
		freechain = &(nbuf->psfb_dirents[PSF_ENTRIES_PER_BUFFER - 1]);
		dirents_nfree += PSF_ENTRIES_PER_BUFFER;
	}
	/* Take first entry from the free list */
	ude = freechain;
	freechain = freechain->psfu_next;
	ude->psfu_next = NULL;
	ude->psfu_token = token;
	--dirents_nfree;
	return ude;
}

/* Given two chains of unicode tokens, return the union of them */
static int chain_merge(psf_unicode_dirent **dest, psf_unicode_dirent *src)
{
	char *str1, *str2, *tok2;
	psf_dword token;
	psf_unicode_dirent *ude, *ude2, *end;

	end = *dest;
	for (ude2 = *dest; ude2 != NULL; ude2 = ude2->psfu_next) end = ude2; 

	str1 = chain2str(*dest);
	str2 = chain2str(src);

	for (tok2 = strtok(str2, ";"); 
	     tok2 != NULL;
     	     tok2 = strtok(NULL, ";"))
	{
		if (strstr(str1, tok2)) 
		{
			continue;
		}
		if (strchr(tok2, '+'))
		{
			/* Add multiple token to end */	
			tok2++;
			/* Add sequence leadin */
			ude = newentry(0xFFFE);
			if (!ude) return PSF_E_NOMEM;
			end->psfu_next = ude;
			ude->psfu_next = NULL;
			end = ude;
			while (strlen(tok2) > 8)
			{
				sscanf(tok2, "%lx", &token);
				ude = newentry(token);
				if (!ude) return PSF_E_NOMEM;
				end->psfu_next = ude;
				ude->psfu_next = NULL;
				end = ude;
				tok2 += 9;	/* Skip hex & separator */
			}
		}
		else
		{
			sscanf(tok2 + 1, "%lx", &token);
			ude = newentry(token);
			if (!ude) return PSF_E_NOMEM;
/* Single token: Insert at front of chain */
			ude->psfu_next = *dest;
			*dest = ude;

		}
	}
	free(str1);
	free(str2);
	return 1;
}

/* Given two chains of unicode tokens, see if their intersection is 
 * nonempty. */
static int chain_matches(psf_unicode_dirent *c1, psf_unicode_dirent *c2)
{
	char *str1, *str2, *tok2;
	
	str1 = chain2str(c1);
	str2 = chain2str(c2);

	for (tok2 = strtok(str2, ";"); 
	     tok2 != NULL;
     	     tok2 = strtok(NULL, ";"))
	{
		if (strstr(str1, tok2))
		{
			free(str1);
			free(str2);
			return 1;	
		}
	}
	return 0;
}

/* Copy a chain of unicode tokens from a PSF file into the unicodes[] array */
static psf_errno_t chain_copy(psf_unicode_dirent **dest, psf_unicode_dirent *src)
{
	psf_unicode_dirent *ude;
	while (src)
	{
		ude = newentry(src->psfu_token);
		if (!ude) return PSF_E_NOMEM;
		*dest = ude;
		dest = &ude->psfu_next;
		src = src->psfu_next;
	}
	return PSF_E_OK;
}


/* Look up an entry in the unicodes array, and return its index. */
static psf_dword unicode_lookup(psf_unicode_dirent *chain)
{
	psf_dword n;

	for (n = 0; n < unicodes_count; n++)
	{
		if (chain_matches(unicodes[n], chain)) return n;
	}
	/* This should never happen because unicode_add() should have
	 * added the chain to the list already */
	return 0;
}


/* Add a chain of Unicode tokens. If it intersects with an existing chain,
 * make the result their union; otherwise, add it as a new chain */
static psf_errno_t unicode_add(psf_unicode_dirent *chain)
{
	psf_unicode_dirent **tmp;
	int n;

	for (n = 0; n < unicodes_count; n++)
	{
		if (chain_matches(unicodes[n], chain)) 
		{
			chain_merge(&unicodes[n], chain);
			return PSF_E_OK;
		}
	}

	if (unicodes_count >= unicodes_max)
	{
		tmp = calloc(unicodes_max + 256, sizeof(psf_unicode_dirent *));
		if (!tmp) return PSF_E_NOMEM;
		if (unicodes) memcpy(tmp, unicodes,
				unicodes_max * sizeof(psf_unicode_dirent *));
		free(unicodes);
		unicodes = tmp;
		unicodes_max += 256;
	}
	return chain_copy(&unicodes[unicodes_count++], chain);
}


/* Do the conversion */
char *cnv_multi(int nfiles, char **infiles, FILE *outfile)
{ 
        int rv, nf, nc;
	psf_dword mc;
        FILE *fp;
	int w = -1, h = -1;
	psf_unicode_dirent *ude;

/* Load all the source files */
	psf_in = malloc(nfiles * sizeof(PSF_FILE));
	if (!psf_in) return "Out of memory";
        for (nf = 0; nf < nfiles; nf++)
        { 
		psf_file_new(&psf_in[nf]);
                if (!strcmp(infiles[nf], "-")) fp = stdin;
                else  fp = fopen(infiles[nf], "rb");
          
                if (!fp) 
		{ 
			perror(infiles[nf]); 
			return "Cannot open font file."; 
		}
		rv = psf_file_read(&psf_in[nf], fp);
                if (fp != stdin) fclose(fp);
		if (rv != PSF_E_OK) 
		{
			sprintf(msgbuf, "%s: %s", infiles[nf],
					psf_error_string(rv));
			return msgbuf;
		}
		if (w == -1 && h == -1)
		{
			w = psf_in[nf].psf_width;
			h = psf_in[nf].psf_height;
		}
		else if (w != psf_in[nf].psf_width || h != psf_in[nf].psf_height)
		{
			return "All fonts must have the same dimensions.";
		}
		if (!psf_is_unicode(&psf_in[nf]))
		{
			sprintf(msgbuf, "%s has no Unicode table.", infiles[nf]);
			return msgbuf;
		}
	}
/* Work out how many unique characters there are */
        for (nf = 0; nf < nfiles; nf++)
        { 
		for (nc = 0; nc < psf_in[nf].psf_length; nc++)
		{
			/* Skip characters with no Unicode mapping */
			if (!psf_in[nf].psf_dirents_used[nc]) continue;
			rv = unicode_add(psf_in[nf].psf_dirents_used[nc]);
			if (rv) 
			{
				free_buffers();
				return psf_error_string(rv);
			}
		}
	}
	rv = psf_file_create(&psf_out, w, h, unicodes_count, 1);
	if (rv) 
	{
		free_buffers();
		return psf_error_string(rv);
	}
        for (nf = 0; nf < nfiles; nf++)
        { 
		for (nc = 0; nc < psf_in[nf].psf_length; nc++)
		{
			/* Skip characters with no Unicode mapping */
			if (!psf_in[nf].psf_dirents_used[nc]) continue;
			mc = unicode_lookup(psf_in[nf].psf_dirents_used[nc]);
			memcpy(psf_out.psf_data + mc * psf_out.psf_charlen,
			       psf_in[nf].psf_data + nc * psf_in[nf].psf_charlen,
			       psf_out.psf_charlen);
		}
	}
	/* Do the Unicode directory */
	rv = 0;
	for (nf = 0; nf < unicodes_count; nf++)
	{
		for (ude = unicodes[nf]; ude != NULL; ude = ude->psfu_next)
		{
			rv = psf_unicode_add(&psf_out, nf, ude->psfu_token);
			if (rv) break;
		}
		if (rv) break;
	}
	if (!rv) rv = psf_file_write(&psf_out, outfile);
	psf_file_delete(&psf_out);
	free_buffers();
        for (nf = 0; nf < nfiles; nf++)
        { 
		psf_file_delete(&psf_in[nf]);
	}
	free(psf_in);
	if (rv) return psf_error_string(rv);
	return NULL;
}

