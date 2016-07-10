
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
extern char *cnv_set_option(int ddash, char *variable, char *value);
/* Return help string */
extern char *cnv_help(void);
/* Do the conversion */
extern char *cnv_execute(FILE *infile, FILE *outfile);
/* Program name */
extern char *cnv_progname;

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
