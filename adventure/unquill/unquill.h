/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996,1999,2011  John Elliott

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if DOS
#include <dos.h>
#endif

#include <time.h>
#ifdef WIN32
# include <conio.h>	/* Win32 has getch() */
# define YES_GETCH 1
# else
#  if (DOS || CPM)	/* DOS and CP/M have getch() */
#  include <conio.h>
#  define YES_GETCH 1
# else
#  include <termios.h>	/* Otherwise simulate it with termios */
#  include <unistd.h>
# endif
#endif

#define VERSION "0.10.1"
#define BUILDDATE "27 December 2015"

/* Hi-Tech C for CP/M doesn't support const */
#if CPM
#define const
#endif

#ifndef PATH_MAX
#ifdef TINY
#define PATH_MAX 20
#else
#define PATH_MAX 80
#endif
#endif	

typedef unsigned char uchr;     /* for brevity */
typedef unsigned short ushrt;   /* -- ditto -- */

#define NIL  100
#define LOC  101
#define MSG  102
#define OBJ  103
#define SWAP 104
#define PLC  105
#define FLG  106
#define BEEP 107
#define PSE  108
#define COL  109

#ifndef YES_GETCH
char getch();
#endif

uchr zmem(ushrt);
ushrt zword(ushrt);
void xoneitem(ushrt,uchr);
void oneitem(ushrt,uchr);
void listitems(char *,ushrt,ushrt);
void xlistobjs(ushrt otab, ushrt postab, ushrt objmap, uchr num);
void xlistlocs(ushrt ltab, ushrt ctab, uchr num);
void xlistwords(ushrt);
void xlistmsgs(char *, ushrt mtab, uchr num);
void xlistsys (char *, ushrt base, uchr num);
void xlistudgs(ushrt);
void xlistfont(ushrt);
void xlistcond(char *, ushrt);
void xopcond(ushrt *);
void xopact(ushrt *);
void listwords(ushrt);
void listcond(ushrt);
void listconn(ushrt,uchr);
void listpos(ushrt,uchr);
void listmap(ushrt,uchr);
void listudgs(ushrt);
void listfont(ushrt);
void xlistgfx(void);
void opword(ushrt);
void opcond(ushrt *);
void opact(ushrt *);
void opch32(char);
void opstr32(char *);
void opcact(char,char *);
void expch (uchr, ushrt *);
void xexpch (uchr, ushrt *);
void expdict (uchr, ushrt *);

char present(uchr);
uchr doproc(ushrt,uchr,uchr);
void listat(uchr);
void listgfx(void);
void initgame(ushrt);
void playgame(ushrt);
void sysmess(uchr);
uchr ffeq(uchr,uchr);
uchr condtrue(ushrt);
ushrt condskip(ushrt);
uchr autobj(uchr);
uchr runact(ushrt,uchr);
uchr cplscmp(ushrt, char *);
uchr matchword(char **);
void usage(char *);
void dec32(ushrt);
char yesno(void);
void savegame(void);
void loadgame(void);

void inform_src(ushrt);
void zcode_binary(void);

void write_header(void);
void write_trailer(void);

void def_colours(void);

/* Vtable for video I/O. This abstracts all operations touching console I/O,
 * so that the DOS version can make full use of hardware-level CGA/EGA/VGA
 * functions (if that's what it wants to do).
 *
 * For historical reasons getch() is not part of this structure, but it 
 * would have to be added if (for example) the UNIX port were to acquire 
 * SDL I/O.
 */
typedef struct
{
	void (*set_window)(int w, int h);	
/* Set display window size. Usually 32x24 for Spectrum, 40x25 for CPC & C64 */

	char *(*gets)(char *buf, int count);
/* Input a command or filename. Populates buf with up to 'count' characters,
 * null-terminated. */

        void (*putch)(char c);			
/* Output a character at the current location. */

	void (*clrscr)(void);			
/* Clear the display window to the current PAPER colour, and home the cursor. */

	void (*showcursor)(uchr ch);		
/* Show / hide text entry cursor at the current location. ch=0 to hide, other
 * values to show. */

	void (*ink)(uchr ch);			
/* Set text foreground. Uses Spectrum colours, ie: 
 *  0=black 1=blue 2=red 3=magenta 4=green 5=cyan 6=yellow 7=white  
 *  8=transparent (leave existing foreground colour untouched)
 *  9=contrast (white on paper 0-3, black on paper 4-7) */

	void (*paper)(uchr ch);			
/* Set text background. Colours as for ink */

	void (*flash)(uchr ch);
/* Set text flashing mode. ch=0 for off, 1 for on, 8 for transparent */

	void (*bright)(uchr ch);	
/* Set text high intensity. ch=0 for off, 1 for on, 8 for transparent */

	void (*inverse)(uchr ch);
/* Set text inverse video (swap foreground / background). 0 for off, 1 for on */

	void (*over)(uchr ch);
/* Set text XOR mode. 0 for off, 1 for on */

	void (*border)(uchr ch);		
/* Set screen border colour. ch=0-7 (colours as for ink) */

	void (*load_font)(uchr *buf, ushrt first, ushrt count);
/* Redefine the text font. 
 * buf is the 8x8 bitmap to load (8 bytes / character)
 * first is the first character to redefine
 * count is the count of characters to redefine */
} CONSOLE;

#if DOS
void video_init(int display_type);
#else
#define video_init(x) ;
#endif

/* Supported architectures */
#define ARCH_SPECTRUM 0
#define ARCH_CPC      1
#define ARCH_C64      2


#ifdef MAINMODULE

#ifndef TINY
uchr zxmemory[0xA500];    /* Spectrum memory from 0x5C00 up - no room for it
                           * in the TINY version. 
						   * In CPC mode, CPC memory from 0x1B00 to 0xBFFF */
#else                     /* In the TINY version, we read in from the file 
                             as required, with two 1k caches of the common 
                             bits: */
uchr buf1[0x400];         /* This buffer holds database pointers and the
                             expansion dictionary */
uchr buf2[0x400];         /* This one holds the vocabulary table */
ushrt b1base,b2base;      /* Spectrum addresses of buffers */
uchr b1full=0,b2full=0;   /* Do buffers contain data? */
#endif

char arch = ARCH_SPECTRUM;		  /* Architecture */
char copt=0;		  /* -C option */
char gopt=0;		  /* -G option */
char mopt=0;		  /* -M option */
char oopt='T';            /* Output option */
char xpos=0;              /* Used in msg display */
char dbver=0;             /* Which format of database? 0 = early Spectrum
                           *                          10 = later Spectrum/CPC */
ushrt ofarg=0;            /* Output to file or stdout? */
ushrt vocab,dict;	  /* Spectrum address of vocabulary & dictionary */
char verbose=0;           /* Output tables verbosely? */
char nobeep=0;            /* Peace and quiet? */
char running=0;           /* Actually playing the game? */
ushrt loctab;
ushrt objtab;             /* Tables of locations, objects, messages */
ushrt msgtab;
ushrt sysbase;		  /* Base of system messages */
ushrt conntab;            /* Connections table */
ushrt postab;             /* Object start positions table */
ushrt objmap;		  /* Word-to-object map */
ushrt proctab, resptab;	  /* Process and Response tables */
ushrt ucptr;		  /* -> Count of takeable objects */
char indent;              /* Listing in single or multiple mode? */
char *inname;             /* Input filename */

uchr fileid=0xFF;	  /* Save file identity byte */
uchr ramsave[0x101];	  /* RAM save buffer */
uchr flags[37];           /* The Quill flags, 0-36. */
#define TURNLO flags[31]  /* (31-36 are special */
#define TURNHI flags[32]  /* Meanings of the special flags */
#define VERB flags[33]
#define NOUN flags[34]
#define CURLOC flags[35]  /* flags[36] == 0 */
uchr objpos[255];         /* Positions of objects */
uchr maxcar;		  /* Max no. of portable objects */
uchr maxcar1;		  /* As maxcar - later games can change it on the fly */
#define NUMCAR flags[1]   /* Number of objects currently carried */
uchr nobj;                /* No. of objects */
uchr nsys;		  /* No. of system messages */
uchr alsosee=1;		  /* Message "You can also see" */
uchr estop;		  /* Emergency stop flag */

FILE *infile, *outfile;

uchr found = 0, skipc = 0, skipf = 0, skipo = 0, skipm = 0, skipn = 0,skipl = 0,
             skipv = 0, skips = 0, skipu = 0, skipg = 0, nloc, nmsg;
uchr comment_out = 0;

ushrt mem_base;          /* Minimum address loaded from snapshot     */
ushrt mem_size;          /* Size of memory loaded from snapshot      */
short mem_offset;        /* Load address of snapshot in physical RAM */
char snapid[20];


#else /* def MAINMODULE */


extern CONSOLE *console;
#ifndef TINY
extern uchr zxmemory[0xA400];
#else
extern uchr buf1[0x400];
extern uchr buf2[0x400];
extern ushrt b1base,b2base;      
extern uchr b1full,b2full; 
#endif

extern char copt, gopt, mopt;
extern char oopt, xpos, dbver,verbose,nobeep,running,indent,*inname;
extern ushrt ofarg,vocab,dict,loctab,objtab,msgtab,sysbase,conntab,objmap;
extern ushrt postab, proctab, resptab, ucptr;
extern uchr comment_out;

extern uchr fileid,ramsave[0x101],flags[37];
#define TURNLO flags[31] 
#define TURNHI flags[32] 
#define VERB flags[33]
#define NOUN flags[34]
#define CURLOC flags[35]
#define NUMCAR flags[1]
extern uchr objpos[255],maxcar,maxcar1,nobj,nsys,alsosee,estop;		  /* Emergency stop flag */
extern FILE *infile, *outfile;

extern uchr skipc, skipo, skipm, skipn, skipl, skipv, skips, skipu, skipg;
extern uchr found, nloc, nmsg;
extern char arch;

extern ushrt mem_base;          /* Minimum address loaded from snapshot     */
extern ushrt mem_size;          /* Size of memory loaded from snapshot      */
extern short mem_offset;        /* Load address of snapshot in physical RAM */
extern char snapid[20];

#endif /* def MAINMODULE */

#ifndef SEEK_SET        /* Hi-tech C doesn't include SEEK_???. */
#define SEEK_SET 0
#endif

#if (CPM || __PACIFIC__) /* CP/M & Pacific don't support argv[0] */
#define AV0 "UNQUILL"
#else
#define AV0 argv[0]
#endif

#define AV10 argv[1][0]
#define AV11 argv[1][1]

