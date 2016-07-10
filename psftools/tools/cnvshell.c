/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003  John Elliott

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

/* All the conversion programs use this code to provide their main(). It 
 * is a basic framework for a one-to-one conversion program, handling
 * option parsing and opening/closing files.
 */


#include "cnvshell.h"

#include "cnvshell.inc"

/* main() parses arguments, opens files, calls the converter. */

int main(int argc, char **argv)
    {
    int n;
    int stoparg = 0;
    char *fname1 = NULL,  *fname2 = NULL;
    FILE *fpin   = stdin, *fpout  = stdout;
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
        else if (!fname2) fname2 = argv[n];
        else 
            {
            fprintf(stderr, "%s: This program takes two filenames, so '%s' is ignored.\n", 
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

    if (fname2) 
        {
        fpout = fopen(fname2, "wb");
        if (!fpout) 
            {
            fclose(fpin);
            perror(fname2);
            exit(1);
            }
        }
    else fname2 = "<stdout>";

    s = cnv_execute(fpin, fpout);

    if (fpin  != stdin)  fclose(fpin);
    if (fpout != stdout) fclose(fpout);

    if (!s) return 0;
    if (fpout != stdout) remove(fname2);

    fprintf(stderr, "%s: %s\n", cnv_progname, s);
    return 1;
    }

