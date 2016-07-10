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

/* fnt2fon uses this code to provide their main(). It 
 * is a basic framework for a many-to-one conversion program, handling
 * option parsing and opening/closing the target file.
 */


#include "cnvmulti.h"


#include "cnvshell.inc"

/* The fun of short command lines... */
#ifdef CPM
#define SHORT_CMDLINE 1
#define PATH_MAX 20
#endif

#ifdef __MSDOS__
#define SHORT_CMDLINE 1
#ifndef PATH_MAX
#define PATH_MAX 68
#endif
#endif


static char **fname;
static int  nfname = 0, mfname = 10;

static void nomem(void)
{
	fprintf(stderr, "Out of memory building argument list\n");
#ifdef EXIT_FAILURE
	exit(EXIT_FAILURE);
#else
	exit(1);
#endif
}

static void add_fname(char *s, int a)
{
	if (a)
	{
		char *t = malloc(1 + strlen(s));
		if (!t) nomem();
		strcpy(t, s);
		fname[nfname++] = t;
	}
	else fname[nfname++] = s;
	if (nfname == mfname)
	{
		fname = realloc(fname, 2 * mfname * sizeof(char *));
		if (!fname) nomem();
		mfname *= 2;
	}
}

/* Read in a list of filenames */
#ifdef SHORT_CMDLINE
static void parse_listfile(char *s)
{
	FILE *fp = fopen(s, "r");
	char buf[PATH_MAX + 20], *p;
	int ch;

	if (!fp)
	{
		perror(s);
		exit(1);
	}
	while (fgets(buf, sizeof(buf), fp))
	{
		p = strchr(buf, '\n');
		if (p) *p = 0;	
		else do	/* Swallow remainder of line */
		{
			ch = fgetc(fp);
		} while (ch != EOF && ch != '\n');
		for (p = strtok(buf, " \t"); p != NULL; 
			 p = strtok(NULL, " \t"))
		{
			/* Don't include EOF */
			if (p[0] != 0x1A) add_fname(p, 1);
		}
	}
}
#endif


/* main() parses arguments, opens files, calls the converter. */

int main(int argc, char **argv)
{
	int n;
	int stoparg = 0;
	char *s;
	FILE *fpout = stdout;

	fname = malloc (10 * sizeof(char *));
	if (!fname) nomem();

	/* Some CP/M and DOS compilers don't support argv[0] */
	if (argv[0][0]) cnv_progname = argv[0];
	/* Argument parsing */
	for (n = 1; n < argc; n++) if (isarg(argv[n]) && !stoparg)
	{
		if (!strcmp(argv[n], "--")) { stoparg = 1; continue; }

		/* Check for likely help commands */
		if (!stricmp(argv[n],   "--help")) help();
		if (!stricmp(argv[n]+1, "h"	 )) help();
#ifdef __MSDOS__
		if (!stricmp(argv[n]+1, "?"	 )) help();
#endif
#ifdef CPM
		if (!stricmp(argv[n]+1, "?"	 )) help();
		if (!stricmp(argv[n],   "//"	)) help();
		if (!stricmp(argv[n],   "[help]")) help();
		if (!stricmp(argv[n],   "[h]"   )) help();
#endif
		/* OK, it isn't a help command. */
		if (argv[n][0] == '-' && argv[n][1] == '-')
		{
			handle_option(1, argv[n]+2);
			continue;
		}
		/* CP/M-style [VARIABLE=VALUE,VARIABLE=VALUE] options */
#ifdef CPM
		if (argv[n][0] == '[')
		{
			char *s;
			
			do
			{
				s = handle_option(1, argv[n]+2);
			} while ( s && (*s) && (*s != ']'));
			continue;
		}
#endif
		/* Short option */
		handle_option(0, argv[n]+1);
	}
	else 
	{
#ifdef SHORT_CMDLINE
		if (argv[n][0] == '@')
		{
			parse_listfile(argv[n] + 1);
		}
		else
#endif
		add_fname(argv[n], 0);
	}
	/* Options parsed */
	/* If filenames omitted, use stdio */
	if (nfname < 1) { fname[0] = "-"; ++nfname; }
	if (nfname < 2) { fname[1] = "-"; ++nfname; }

	if (!strcmp(fname[nfname-1], "-")) 
	{
		fname[nfname-1] = "stdin";
	fpout = stdout;
	}
	else
	{
		fpout = fopen(fname[nfname-1], "wb");
		if (!fpout) 
		{
			perror(fname[nfname-1]);
			free(fname);
			exit(1);
		}
	}
	
	s = cnv_multi(nfname-1, fname, fpout);

	if (fpout != stdout) fclose(fpout);

	if (!s) { free(fname); return 0; }
	if (fpout != stdout) remove(fname[nfname-1]);
	
	free(fname);
	fprintf(stderr, "%s: %s\n", cnv_progname, s);
	return 1;
}

