/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2001  John Elliott

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

/* This is a mini version of CNVSHELL.C, that handles a single file only.
 * It is used in loading PSF fonts. 
 */


#include "cnvshell.h"

/* Is an argument an option? */

static int isarg(char *s)
    {
    if (s[0] == '-') return 1;
#ifdef __MSDOS__
    if (s[0] == '/') return 1;
#endif
#ifdef CPM
    if (s[0] == '[' || s[0] == '/') return 1;
#endif
    return 0;
    }

/* Case-insensitive string comparison */

#ifndef HAVE_STRICMP

int stricmp(char *s, char *t)
    {
    char a,b;

    while (*s && *t)
        {
        a = *s; b = *t;
        if (isupper(a)) a = tolower(a);
        if (isupper(b)) b = tolower(b);

        if (a != b) return (a-b);
     	++s;
	++t;
        }
    return (*s) - (*t);
    }

#endif

/* Display help screen */

static void help(void)
    {
    printf("%s\n", cnv_help());
    exit(0);
    }

/* Having received an option, parse it as "variable" or "variable=value" 
 * and pass it to the program. */

static char *handle_option(int ddash, char *s)
    {
    char *var, *val, *eq, *term;
    /* Option is of the form: VARIABLE=VALUE  or  VARIABLE */
    /* Under CP/M, may be followed by a comma or ] */

    var = malloc(1 + strlen(s)); 
    val = malloc(1 + strlen(s));

    if (!var || !val)
        {
        if (var) free(var);
        if (val) free(val);
        fprintf(stderr, "%s: Out of memory while parsing arguments.\n", cnv_progname);
        return s + strlen(s);
        }
    strcpy(var, s);
    val[0] = 0;

#ifdef CPM
    /* Check for ] */
    eq = strchr(var, ']');
    if (eq) *eq = 0;        /* Blank out ] */
    eq = strchr(var, ',');
    if (eq) *eq = 0;        /* Blank out , */
#endif

    eq = strchr(var, '=');
    if (eq)
        {
        strcpy(val, eq + 1);
        *eq = 0;
        }
    term = cnv_set_option(ddash, var, val);
    free(var);
    free(val);
#ifdef CPM
    eq = strchr(s,','); if (eq) return eq;
    eq = strchr(s,']'); if (eq) return eq;
#endif
    if (term) { fprintf(stderr, "%s: %s.\n", cnv_progname, term); exit(1); }
    return s + strlen(s);
    }


/* main() parses arguments, opens files, calls the converter. */

int main(int argc, char **argv)
    {
    int n;
    int stoparg = 0;
    char *fname1 = NULL;
    FILE *fpin   = stdin;
    char *s;

    /* Some CP/M and DOS compilers don't support argv[0] */
    if (argv[0][0]) cnv_progname = argv[0];
    /* Argument parsing */
    for (n = 1; n < argc; n++) if (isarg(argv[n]) && !stoparg)
        {
        if (!strcmp(argv[n], "--")) { stoparg = 1; continue; }

        /* Check for likely help commands */
        if (!stricmp(argv[n],   "--help")) help();
        if (!stricmp(argv[n]+1, "h"     )) help();
#ifdef __MSDOS__
        if (!stricmp(argv[n]+1, "?"     )) help();
#endif
#ifdef CPM
        if (!stricmp(argv[n]+1, "?"     )) help();
        if (!stricmp(argv[n],   "//"    )) help();
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
        if      (!fname1) fname1 = argv[n];
        else 
            {
            fprintf(stderr, "%s: This program takes one filenames, so '%s' is ignored.\n", 
                            cnv_progname, argv[n]);
            }
        }
    /* Options parsed */
    if (fname1) 
        {
        fpin = fopen(fname1, "rb");
        if (!fpin) 
            {
            perror(fname1);
            exit(1);
            }
        }
    else fname1 = "<stdin>";

    s = cnv_execute(fpin, stdout);

    if (fpin  != stdin)  fclose(fpin);

    if (!s) return 0;

    fprintf(stderr, "%s: %s\n", cnv_progname, s);
    return 1;
    }

