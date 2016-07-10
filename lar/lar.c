/*
 * Lar - LU format library file maintainer
 * by Stephen C. Hemminger
 *	linus!sch	or	sch@Mitre-Bedford
 *
 * [John Elliott, 2005-01-29] Merge patch from Steven N. Hirsch for 
 * compilation on SuSE Linux 9.2
 *      <jce@seasip.demon.co.uk>
 *
 * [John Elliott, 2003-01-11] Made all fopen()s use binary mode.
 *      <jce@seasip.demon.co.uk>
 *
 * [John Elliott, 1998-07-18] Updated to match the official LBR spec 
 * (LUDEF5.DOC). This includes CRC checking (so archives made by lar 
 * aren't frowned at by CP/M archivers). 
 *      <jce@seasip.demon.co.uk>
 */
#define VERSION "5.1.2"
/*
 *
 * Also added prototypes, and tried to shut 'gcc -Wall' up
 *
 *
 *  Usage: lar key library [files] ...
 *
 *  Key functions are:
 *	u - Update, add files to library
 *	t - Table of contents
 *	e - Extract files from library
 *	p - Print files in library
 *	d - Delete files in library
 *	r - Reorganise library
 *  Other keys:
 *	v - Verbose
 *	l - Produce ls -l listing
 *
 *  This program is public domain software, no warranty intended or
 *  implied.
 *
 *  DESCRPTION
 *     Lar is a Unix program to manipulate CP/M LU format libraries.
 *     The original CP/M library program LU is the product
 *     of Gary P. Novosielski. The primary use of lar is to combine several
 *     files together for upload/download to a personal computer.
 *
 *  PORTABILITY
 *     The code is modeled after the Software tools archive program,
 *     and is setup for Version 7 Unix.  It does not make any assumptions
 *     about byte ordering, explict and's and shift's are used.
 *     If you have a dumber C compiler, you may have to recode new features
 *     like structure assignment, typedef's and enumerated types.
 *
 *  BUGS/MISFEATURES
 *     The biggest problem is text files, the programs tries to detect
 *     text files vs. binaries by checking for non-Ascii (8th bit set) chars.
 *     If the file is text then it will throw away Control-Z chars which
 *     CP/M puts on the end.  All files in library are padded with Control-Z
 *     at the end to the CP/M sector size if necessary.
 *
 *     No effort is made to handle the difference between CP/M and Unix
 *     end of line chars.  CP/M uses Cr/Lf and Unix just uses Lf.
 *     The solution is just to use the Unix command sed when necessary.
 *
 *  * Unix is a trademark of Bell Labs.
 *  ** CP/M is a trademark of Digital Research.
 */

#include <stdio.h>
#include <assert.h>	/* [JCE] */
#include <stdlib.h>	/* [JCE] */
#include <unistd.h>	/* [JCE] */
#include <sys/stat.h>	/* [JCE] */
#include <time.h>	/* [JCE] */
#include <ctype.h>
#include <string.h>
#include <errno.h>

/* [JCE] CRC code */
#include "crcsubs.h"

#define ACTIVE	00
#define UNUSED	0xff
#define DELETED 0xfe
#define CTRLZ	0x1a

#define MAXFILES 256
#define SECTOR	 128
#define DSIZE	( sizeof(struct ludir) )
#define SLOTS_SEC (SECTOR/DSIZE)
#define equal(s1, s2) ( strcmp(s1,s2) == 0 )
/* if you don't have void type just define as blank */
#define VOID	(void)

/* if no enum's then define false as 0 and true as 1 and bool as int */
typedef enum {false=0, true=1} bool;

/* Globals */
char   *fname[MAXFILES];
bool ftouched[MAXFILES];

typedef struct {
    unsigned char   lobyte;
    unsigned char   hibyte;
} word;

/* convert word to int */
#define wtoi(w) ( (w.hibyte<<8) + w.lobyte)
#define itow(dst,src)	dst.hibyte = (src & 0xff00) >> 8;\
				dst.lobyte = src & 0xff;

struct ludir {			/* Internal library ldir structure */
    unsigned char   l_stat;	/*  status of file */
    char    l_name[8];		/*  name */
    char    l_ext[3];		/*  extension */
    word    l_off;		/*  offset in library */
    word    l_len;		/*  lengty of file */

/* [JCE] Extra fields from LUDEF5.DOC */

    word    l_crc;		/*  cyclic redundancy checksum of file */
    word    l_datec;		/*  date created     (days since 19771231) */
    word    l_dateu;		/*  last change date (days since 19771231) */
    word    l_timec;		/*  time created     (DOS format) */
    word    l_timeu;		/*  time updated     (DOS format) */
    char    l_pad;		/*  no. of bytes unused in last record */
    char    l_fill[5];		/*  pad to 32 bytes */
} ldir[MAXFILES];

byte    crcbuf[512];

int     errcnt, nfiles, nslots;
bool	verbose = false;
bool    ls_l    = false;
char	*cmdname;

/* [JCE] prototypes for functions, to shut "gcc -Wall" up */

#ifndef NO_PROTOTYPES
void help(void);
void version(void);
void conflict(void);
void error(char *str);
void filenames(int argc, char **argv);
int table(char *lib);
int update(char *lib);
int reorg(char *lib);
int extract(char *lib);
int print(char *lib);
int delete(char *lib);
void getdir(FILE *dir);
void putdir(FILE *dir);
char *getname (char *nm, char *ex);
int filarg(char *name);
void not_found();
void getfiles (char *name, bool pflag);
word16 acopy (FILE *fdi, FILE *fdo, unsigned int nsecs, char pad);
void addfil (char *name, FILE *lfd);
int fcopy (FILE *ifd, FILE *ofd, char *pad, word *crc);
void copymem(char *dst, char *src, unsigned int n);
void copyentry( struct ludir *old, FILE *of, struct ludir *new, FILE *nf );
void put_times  ( struct ludir *dst, struct stat *src);
void print_times( struct ludir *ld);
void print_mtime( struct ludir *ld);
int cpmdate(time_t *t);
word16 dostime(time_t *t);
#else
char   *getname(), *sprintf(); 
int     update(), reorg(), table(), extract(), print(), delete();
#endif	/* NO_PROTOTYPES */

int
main (argc, argv)
int	argc;
char  **argv;
{
    register char *flagp;
    char   *aname;			/* name of library file */
    int	   (*function)(char *) = NULL;	/* function to do on library */
/* set the function to be performed, but detect conflicts */
#define setfunc(val)	do { if(function != NULL) conflict(); \
			     else function = val; \
			} while(0) 

/* 5.1.1: Based on email from C. B. Falconer: Check that the structure size
 * is correct */
    assert(DSIZE == 32);

    /* [JCE] init CRC code */
    CRC_Init(crcbuf);

    cmdname = argv[0];
    if (argc < 3)
        {
	/* [JCE] Support "--version" command */

	if (argc > 1 && !strcmp(argv[1], "--version")) version();
	else                                           help ();
	}
    aname = argv[2];
    filenames (argc, argv);

    for(flagp = argv[1]; *flagp; flagp++)
	switch (*flagp) {
	case 'u': 
	    setfunc(update);
	    break;
	case 't': 
	    setfunc(table);
	    break;
	case 'e': 
	    setfunc(extract);
	    break;
	case 'p': 
	    setfunc(print);
	    break;
	case 'd': 
	    setfunc(delete);
	    break;
	case 'r': 
	    setfunc(reorg);
	    break;
	case 'v':
	    verbose = true;
	    break;
 	case 'l':
            ls_l = true;
            break;
	default: 
	    help ();
    }
    if (verbose && ls_l) conflict();  /* cbf */

    if(function == NULL) {
	fprintf(stderr,"No function key letter specified\n");
	help();
    }

    (*function)(aname);
    return 0;
    }

/* print error message and exit */
void 
help (void ) {
    fprintf (stderr, "Usage: %s {utepdr}[vl] library [files] ...\n", cmdname);
    fprintf (stderr, "Functions are:\n\tu - Update, add files to library\n");
    fprintf (stderr, "\tt - Table of contents\n");
    fprintf (stderr, "\te - Extract files from library\n");
    fprintf (stderr, "\tp - Print files in library\n");
    fprintf (stderr, "\td - Delete files in library\n");
    fprintf (stderr, "\tr - Reorganise library\n");

    fprintf (stderr, "Flags are:\n\tv - Verbose\n");
    fprintf (stderr, "\tl - Produce ls -l listing\n");
    exit (1);
}

/* [JCE] Report version */
void
version (void) {
    printf ("lar version " VERSION "\n");
    exit(0);
}


void 
conflict(void ) {
   fprintf(stderr,"Conficting keys\n");
   help();
}

void
error (str)
char   *str;
{
    fprintf (stderr, "%s: %s\n", cmdname, str);
    exit (1);
}

void
cant (name)
char   *name;
{
    fprintf (stderr, "%s: %s\n", name, strerror(errno));
    exit (1);
}

/* Get file names, check for dups, and initialize */
void
filenames (ac, av)
int ac;
char  **av;
{
    register int    i, j;

    errcnt = 0;
    for (i = 0; i < ac - 3; i++) {
	fname[i] = av[i + 3];
	ftouched[i] = false;
	if (i == MAXFILES)
	    error ("Too many file names.");
    }
    fname[i] = NULL;
    nfiles = i;
    for (i = 0; i < nfiles; i++)
	for (j = i + 1; j < nfiles; j++)
	    if (equal (fname[i], fname[j])) {
		fprintf (stderr, "%s", fname[i]);
		error (": duplicate file name");
	    }
}


int 
lelen(d)			/* [JCE] Calculate the precise length of an */
struct ludir *d;		/*       entry */
{
    int i;

    i = 128 * wtoi(d->l_len);
    i -= d->l_pad;
    return i;
}



int
table (lib)		/* [JCE] modified to do length in bytes, not sectors */
char   *lib;
{
    FILE   *lfd;
    register int    i, total;
    int active = 0, unused = 0, deleted = 0;
    char *uname;

    if ((lfd = fopen (lib, "rb")) == NULL)
	cant (lib);

    getdir (lfd);
    total = wtoi(ldir[0].l_len);
    if(verbose) {
 	printf("Name          Index Length   CRC         Dat"
               "e created         Date updated \n");
        printf("============================================"
               "==============================\n");
	printf("Directory           %6d  %04x  ", total * 128, 
                                                  wtoi(ldir[0].l_crc));
        print_times(&ldir[0]);
	putchar('\n');
    }

    for (i = 1; i < nslots; i++)
	switch(ldir[i].l_stat) {
	case ACTIVE:
		active++;
		uname = getname(ldir[i].l_name, ldir[i].l_ext);
		if (filarg (uname))
                {
		    if(verbose)
                    {
			printf ("%-12s   %4d %6d  %04x  ", uname,
			    wtoi (ldir[i].l_off), lelen (&ldir[i]),
                            wtoi (ldir[i].l_crc));
                        print_times(&ldir[i]);
			putchar('\n');
                    }
		    else if (ls_l)
                    {
                        printf("-rw-rw-rw- 1 %8d %8d %8d ", 
                               getuid(), getgid(), lelen(&ldir[i]));
                        print_mtime(&ldir[i]);
                        printf(" %s\n", uname);
                    }
                    else printf ("%s\n", uname);
                }
		total += wtoi(ldir[i].l_len);
		break;
	case UNUSED:
		unused++;
		break;
	default:
		deleted++;
	}
    if(verbose) {
	printf("---------------------------------------------"
               "-----------------------------\n");
	printf("Total bytes used    %6d\n", total * 128);
	printf("\nLibrary %s has %d slots: %d deleted, %d active, %d unused, 1 directory.\n",
		lib, nslots, deleted, active, unused);
    }

    VOID fclose (lfd);
    not_found ();
    return 0;
}

void
getdir (f)
FILE *f;
{
    byte *p;
    int i;
    word16 crc;
 
    rewind(f);

    if (fread ((char *) & ldir[0], DSIZE, 1, f) != 1)
	error ("No directory - this file is not a library.\n");

    if (memcmp((char *)&ldir[0], "\000           ", 12))
	error ("No LBR signature - is this file a library?\n");

    nslots = wtoi (ldir[0].l_len) * SLOTS_SEC;

    if (fread ((char *) & ldir[1], DSIZE, nslots, f) != nslots)
	error ("Can't read directory - is this file a library?\n");

    /* [JCE] CRC the directory */

    crc = wtoi(ldir[0].l_crc);

    itow(ldir[0].l_crc, 0);	/* When CRC-ing the directory, its CRC is 0 */
    p = (byte *) ldir;
    CRC_Clear();

    for (i = 0; i < (DSIZE * nslots); i++)
    {
        CRC_Update(p[i]);
    }

    if (crc != CRC_Done() )
    {
        fprintf(stderr,"Warning: LBR directory checksum is wrong - is this a "
                       "library?\n");
    }    
    itow(ldir[0].l_crc, crc);	/* Restore old CRC */
}

void
putdir (f)
FILE *f;
{
    byte *p;
    int i;
    time_t t;

    rewind(f);

    /* [JCE] Set LBR last modified time */

    time(&t);
    itow(ldir[0].l_dateu, cpmdate(&t));
    itow(ldir[0].l_timeu, dostime(&t));

    /* [JCE] CRC the directory */

    itow(ldir[0].l_crc, 0);
    p = (byte *) ldir;   
    CRC_Clear();

    for (i = 0; i < (DSIZE * nslots); i++)
    { 
        CRC_Update(p[i]);
    }
    itow(ldir[0].l_crc, CRC_Done());

    if (fwrite ((char *) ldir, DSIZE, nslots, f) != nslots)
	error ("Can't write directory - library may be botched");
}

void
initdir (f)
FILE *f;
{
    register int    i;
    int     numsecs;
    char    line[80];
    static struct ludir blankentry = {
	UNUSED,
	{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
	{ ' ', ' ', ' ' },
    };
    time_t t;

    for (;;) {
	printf ("Number of slots to allocate: ");
	if (fgets (line, 80, stdin) == NULL)
	    error ("Eof when reading input");
	nslots = atoi (line);
	if (nslots < 1)
	    printf ("Must have at least one!\n");
	else if (nslots > MAXFILES)
	    printf ("Too many slots\n");
	else
	    break;
    }
/* 5.1.1: Based on email from C. B. Falconer: Add 1 slot for the first entry
 * (the directory itself) */
    numsecs = (nslots / SLOTS_SEC) + 1;
    nslots = numsecs * SLOTS_SEC;

    for (i = 0; i < nslots; i++)
	ldir[i] = blankentry;
    ldir[0].l_stat = ACTIVE;
    itow (ldir[0].l_len, numsecs);

    /* [JCE] set directory creation time */

    time(&t);
    itow(ldir[0].l_datec, cpmdate(&t));
    itow(ldir[0].l_timec, dostime(&t));

    putdir (f);
}

/* convert nm.ex to a Unix style string */
char   *getname (nm, ex)
char   *nm, *ex;
{
    static char namebuf[14];
    register char  *cp, *dp;

    for (cp = namebuf, dp = nm; *dp != ' ' && dp != &nm[8];)
	*cp++ = isupper (*dp) ? tolower (*dp++) : *dp++;
    *cp++ = '.';

    for (dp = ex; *dp != ' ' && dp != &ex[3];)
	*cp++ = isupper (*dp) ? tolower (*dp++) : *dp++;

    *cp = '\0';
    return namebuf;
}

void
putname (cpmname, unixname)
char   *cpmname, *unixname;
{
    register char  *p1, *p2;

    for (p1 = unixname, p2 = cpmname; *p1; p1++, p2++) {
	while (*p1 == '.') {
	    p2 = cpmname + 8;
	    p1++;
	}
	if (p2 - cpmname < 11)
	    *p2 = islower(*p1) ? toupper(*p1) : *p1;
	else {
	    fprintf (stderr, "%s: name truncated\n", unixname);
	    break;
	}
    }
    while (p2 - cpmname < 11)
	*p2++ = ' ';
}

/* filarg - check if name matches argument list */
int
filarg (name)
char   *name;
{
    register int    i;

    if (nfiles <= 0)
	return 1;

    for (i = 0; i < nfiles; i++)
	if (equal (name, fname[i])) {
	    ftouched[i] = true;
	    return 1;
	}

    return 0;
}

void
not_found () {
    register int    i;

    for (i = 0; i < nfiles; i++)
	if (!ftouched[i]) {
	    fprintf (stderr, "%s: not in library.\n", fname[i]);
	    errcnt++;
	}
}

int
extract(name)
char *name;
{
	getfiles(name, false);
	return 0;
}

int
print(name)
char *name;
{
	getfiles(name, true);
	return 0;
}

void
getfiles (name, pflag)
char   *name;
bool	pflag;
{
    FILE *lfd, *ofd;
    register int    i;
    char   *unixname;
    word16 crc;		/* [JCE] */

    if ((lfd = fopen (name, "rb"))  == NULL)
	cant (name);

    ofd = pflag ? stdout : NULL;
    getdir (lfd);

    for (i = 1; i < nslots; i++) {
	if(ldir[i].l_stat != ACTIVE)
		continue;
	unixname = getname (ldir[i].l_name, ldir[i].l_ext);
	if (!filarg (unixname))
	    continue;
	fprintf(stderr,"%s", unixname);
	if (ofd != stdout)
	    ofd = fopen (unixname, "wb");
	if (ofd == NULL) {
	    fprintf (stderr, "  - can't create");
	    errcnt++;
	}
	else {
	    VOID fseek (lfd, (long) wtoi (ldir[i].l_off) * SECTOR, 0);
	    crc = acopy (lfd, ofd, wtoi (ldir[i].l_len), ldir[i].l_pad); 
							/* [JCE] get CRC */
	    if (ofd != stdout)
		VOID fclose (ofd);
            if (wtoi(ldir[i].l_crc) != crc)	/* [JCE] check CRC */
            {
		fprintf(stderr, "  - CRC error");
		errcnt++;
            }
	}
	putc('\n', stderr);
    }
    VOID fclose (lfd);
    not_found ();
}

word16				/* [JCE] returns CRC */
acopy (fdi, fdo, nsecs, pad)
FILE *fdi, *fdo;
register unsigned int nsecs;
char pad;
{
    register int    i, c;
    int	    textfile = 1;

    CRC_Clear();		/* [JCE] reset CRC */

    while( nsecs-- != 0) 
	for(i=0; i<SECTOR; i++) {
		c = getc(fdi);
		CRC_Update(c);	/* [JCE] add character to CRC */

		/* [JCE] bytewise correction on last sector of file */
                if( (!nsecs) && i > (SECTOR - pad))
                {
		    continue;	/* CRC it but don't putc() it */
                }
		if( feof(fdi) ) 
			error("Premature EOF\n");
		if( ferror(fdi) )
		    error ("Can't read");
		if( !isascii(c) )
		    textfile = 0;
		if( nsecs != 0 || !textfile || c != CTRLZ) {
			putc(c, fdo);
			if ( ferror(fdo) )
			    error ("write error");
		}
	 }
    return CRC_Done();	/* [JCE] Return final CRC */
}

int
update (name)
char   *name;
{
    FILE *lfd;
    register int    i;

    if ((lfd = fopen (name, "r+b")) == NULL) {
	if ((lfd = fopen (name, "w+b")) == NULL)
	    cant (name);
	initdir (lfd);
    }
    else
	getdir (lfd);		/* read old directory */

    if(verbose)
	    fprintf (stderr,"Updating files:\n");
    for (i = 0; i < nfiles; i++)
	addfil (fname[i], lfd);
    if (errcnt == 0)
	putdir (lfd);
    else
	fprintf (stderr, "fatal errors - library not changed\n");
    VOID fclose (lfd);
    return 0;
}

void
addfil (name, lfd)
char   *name;
FILE *lfd;
{
    FILE	*ifd;
    register int secoffs, numsecs;
    register int i;
    struct stat st;

    if (stat (name, &st))
    {
        fprintf (stderr, "%s: can't stat file\n",name);
        errcnt++;
        return;
    }

    if ((ifd = fopen (name, "rb")) == NULL) {
	fprintf (stderr, "%s: can't find to add\n",name);
	errcnt++;
	return;
    }
    if(verbose)
        fprintf(stderr, "%s\n", name);
    for (i = 0; i < nslots; i++) {
	if (equal( getname (ldir[i].l_name, ldir[i].l_ext), name) ) /* update */
	    break;
	if (ldir[i].l_stat != ACTIVE)
		break;
    }
    if (i >= nslots) {
	fprintf (stderr, "%s: can't add library is full\n",name);
	errcnt++;
	return;
    }

    ldir[i].l_stat = ACTIVE;
    putname (ldir[i].l_name, name);
    VOID fseek(lfd, 0L, 2);		/* append to end */
    secoffs = ftell(lfd) / SECTOR;

    itow (ldir[i].l_off, secoffs);
    numsecs = fcopy (ifd, lfd, &(ldir[i].l_pad), &(ldir[i].l_crc)); 
				/* bytewise correction & CRC */
    itow (ldir[i].l_len, numsecs);
    put_times( &ldir[i], &st );		/* [JCE] convert file times */

    VOID fclose (ifd);
}

int
fcopy (ifd, ofd, pad, crc)
FILE *ifd, *ofd;
char *pad;		/* [JCE] return pad count */	
word *crc;              /* [JCE] return CRC */ 
{
    register int total = 0;
    register int i, n;
    char sectorbuf[SECTOR];
    word crc1;

    CRC_Clear();

    *pad = 0;

    while ( (n = fread( sectorbuf, 1, SECTOR, ifd)) != 0) {
	if (n != SECTOR)
            {
	    for (i = n; i < SECTOR; i++)
		sectorbuf[i] = CTRLZ;
            *pad = SECTOR - n;
            }
	/* [JCE] Calculate CRC */
        for (i = 0; i < SECTOR; i++) CRC_Update(sectorbuf[i]);

	if (fwrite( sectorbuf, 1, SECTOR, ofd ) != SECTOR)
		error("write error");
	++total;
    }
    itow(crc1, CRC_Done());
    copymem((char *)crc, (char *)(&crc1), sizeof(crc1));
    return total;
}

int
delete (lname)
char   *lname;
{
    FILE *f;
    register int    i;

    if ((f = fopen (lname, "r+b")) == NULL)
	cant (lname);

    if (nfiles <= 0)
	error("delete by name only");

    getdir (f);
    for (i = 0; i < nslots; i++) {
	if (!filarg ( getname (ldir[i].l_name, ldir[i].l_ext)))
	    continue;
	ldir[i].l_stat = DELETED;
    }

    not_found();
    if (errcnt > 0)
	fprintf (stderr, "errors - library not updated\n");
    else
	putdir (f);
    VOID fclose (f);
    return 0;
}

int
reorg (name)
char  *name;
{
    FILE *olib, *nlib;
    int oldsize;
    register int i, j;
    struct ludir odir[MAXFILES];
    char tmpname[SECTOR];

    VOID sprintf(tmpname,"%-10.10s.TMP", name);

    if( (olib = fopen(name,"rb")) == NULL)
	cant(name);

    if( (nlib = fopen(tmpname, "wb")) == NULL)
	cant(tmpname);

    getdir(olib);
    printf("Old library has %d slots\n", oldsize = nslots);
    for(i = 0; i < nslots ; i++)
	    copymem( (char *) &odir[i], (char *) &ldir[i],
			sizeof(struct ludir));
    initdir(nlib);
    errcnt = 0;

    for (i = j = 1; i < oldsize; i++)
	if( odir[i].l_stat == ACTIVE ) {
	    if(verbose)
		fprintf(stderr, "Copying: %-8.8s.%3.3s\n",
			odir[i].l_name, odir[i].l_ext);
	    copyentry( &odir[i], olib,  &ldir[j], nlib);
	    if (++j >= nslots) {
		errcnt++;
		fprintf(stderr, "Not enough room in new library\n");
		break;
	    }
        }

    VOID fclose(olib);
    putdir(nlib);
    VOID fclose (nlib);

    if(errcnt == 0) {
	if ( unlink(name) < 0 || link(tmpname, name) < 0) {
	    VOID unlink(tmpname);
	    cant(name);
        }
    }
    else
	fprintf(stderr,"Errors, library not updated\n");
    VOID unlink(tmpname);

    return 0;
}

void
copyentry( old, of, new, nf )
struct ludir *old, *new;
FILE *of, *nf;
{
    register int secoffs, numsecs;
    char buf[SECTOR];

    new->l_stat = ACTIVE;
    copymem(new->l_name, old->l_name, 8);
    copymem(new->l_ext, old->l_ext, 3);
/* [JCE v5.1.1] Copy these across as well when reorganising */
    new->l_crc   = old->l_crc;
    new->l_datec = old->l_datec;
    new->l_dateu = old->l_dateu;
    new->l_timec = old->l_timec;
    new->l_timeu = old->l_timeu;
    new->l_pad   = old->l_pad;
    VOID fseek(of, (long) wtoi(old->l_off)*SECTOR, 0);
    VOID fseek(nf, 0L, 2);
    secoffs = ftell(nf) / SECTOR;

    itow (new->l_off, secoffs);
    numsecs = wtoi(old->l_len);
    itow (new->l_len, numsecs);

    while(numsecs-- != 0) {
	if( fread( buf, 1, SECTOR, of) != SECTOR)
	    error("read error");
	if( fwrite( buf, 1, SECTOR, nf) != SECTOR)
	    error("write error");
    }
}

void
copymem(dst, src, n)
register char *dst, *src;
register unsigned int n;
{
	while(n-- != 0)
		*dst++ = *src++;
}

/* [JCE] Date & time routines. */

int 
cpmdate(t)
time_t *t;
{
	struct tm *ptm;
	int days;
	int leap;

	ptm = localtime(t);

	/* Work out no. of days since 1 Jan 1976 */

	if (ptm->tm_year < 78) return 0;	/* Not a valid CP/M date */

	/* Count: Leap years since 1 Jan 1976 */
	
	leap = (ptm->tm_year - 76) / 4;
	leap *= 1461;		/* Days since 1 Jan 1976 */

	days = (ptm->tm_year % 4) * 365; /* Days since start of last leap yr */
	if (days) ++days;		 /* +1 for the leap year itself */
	days += ptm->tm_yday;
	days += leap;
	days -= 730;			/* Days since 1 Jan 1978 */

	return days;
}

word16 
dostime(t)
time_t *t;
{
	word16 w;
	struct tm *ptm;

	ptm = localtime(t);

	w =   (ptm->tm_sec)   / 2;
	w |=  (ptm->tm_min)  << 5;
        w |=  (ptm->tm_hour) << 11;

	return w;
}

time_t
unixdate(days, time)
int days;
int time;
{			/*    J  F  M  A  M  J  J  A  S  O  N  D */
	static int mday[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	static int mdayl[]= {31,29,31,30,31,30,31,31,30,31,30,31};

	/* Convert CP/M day count to Unix */
	struct tm tm1;
	int i, month;

	days += 730;	/* Days since 1 Jan 1976 */
	tm1.tm_year = 4 * (days / 1461) + 76;	
	days %= 1461;	/* Days since start of last leap year */
	month = 0;
	if (days < 366)	/* in a leap year */
	{
		for (i = 0; i < 12; i++)
		{
			month++;
			if (days < mdayl[i]) break;
			days -= mdayl[i];
		}		
	}
	else 
	{
		--days;
		tm1.tm_year += (days / 365);
		days %= 365;
		for (i = 0; i < 12; i++)
		{
			month++;
			if (days < mday[i]) break;
			days -= mday[i];
		}
	}
	tm1.tm_mon   = month;
	tm1.tm_mday  = 1 + days;	

        tm1.tm_isdst = -1;

	tm1.tm_hour = ((time >> 11) & 0x1F);	/* Hours */
        tm1.tm_min  = ((time >> 5)  & 0x3F);	/* Minutes */
	tm1.tm_sec  = ( time        & 0x1F) * 2;      /* Seconds */

	return mktime(&tm1);
}


void
put_times( dst, src)
struct ludir *dst;
struct stat *src;
{
	itow(dst->l_datec, cpmdate(&src->st_ctime));
        itow(dst->l_dateu, cpmdate(&src->st_mtime));
	itow(dst->l_timec, dostime(&src->st_ctime));
	itow(dst->l_timeu, dostime(&src->st_mtime));
}


void 
optime (ptm, days)
struct tm *ptm;
int days;
{
	if (ls_l)
        {
/* Keep the ls -l format using 2-digit years; mc may require it */
/*		if (!days) printf("01-01-1970 00:00");
		else       printf("%02d-%02d-%04d %02d:%02d",  
                          ptm->tm_mon+1, ptm->tm_mday, (ptm->tm_year + 1900),
                          ptm->tm_hour, ptm->tm_min);
*/
		if (!days) printf("01-01-70 00:00");
		else       printf("%02d-%02d-%02d %02d:%02d",  
                          ptm->tm_mon+1, ptm->tm_mday, (ptm->tm_year % 100),
                          ptm->tm_hour, ptm->tm_min);

		return;
        }
	

	if (!days) printf("%19s","");
	else	printf("%02d/%02d/%04d %02d:%02d:%02d  ", 
               ptm->tm_mday, ptm->tm_mon+1, (ptm->tm_year + 1900),       
               ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

void 
print_times(ld)
struct ludir *ld;
{
	time_t tcreate, tupdate;
	struct tm *ptm;

	tcreate = unixdate(wtoi(ld->l_datec), wtoi(ld->l_timec));
	tupdate = unixdate(wtoi(ld->l_dateu), wtoi(ld->l_timeu));

	ptm = gmtime(&tcreate);	
	optime(ptm, wtoi(ld->l_datec));
	ptm = gmtime(&tupdate);
	optime(ptm, wtoi(ld->l_dateu));
}

void
print_mtime(ld)
struct ludir *ld;
{
        time_t tupdate;
        struct tm *ptm;

        tupdate = unixdate(wtoi(ld->l_dateu), wtoi(ld->l_timeu));

        ptm = gmtime(&tupdate);
        optime(ptm, wtoi(ld->l_dateu));
}
