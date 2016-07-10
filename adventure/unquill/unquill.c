/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996-2000,2013  John Elliott

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

                             - * -

    Written for gcc under Unix; compiles with Hi-Tech C under CP/M and 
    Pacific C under DOS.
 
    Under CP/M, compile with:  C -DTINY UNQUILL.C TABLES.C CONDACT.C INFORM.C

*/


/* Pull Quill data from a .SNA file.  *
 * Output to stdout.                  */

/* Format of the Quill database:

 Somewhere in memory (usually 6D04h for version 'A' and 6B85h for version 'C')
there is a colour definition table:

	DEFB	10h,ink,11h,paper,12h,flash,13h,bright,14h,inverse,15h,over
	DEFB	border

We use this as a magic number to find the Quill database in memory. After
the colour table is a table of values:

	DEFB	no. of objects player can carry at once
	DEFB	no. of objects
	DEFB	no. of locations
	DEFB	no. of messages

then a table of pointers:

	DEFW	rsptab	;Response table
	DEFW	protab	;Process table
	DEFW	objtop	;Top of object descriptions
	DEFW	loctop	;Top of locations.
	DEFW	msgtop	;Top of messages.
	**
	DEFW	conntab	;Table of connections.
	DEFW	voctab	;Table of words.
	DEFW	oloctab	;Table of object inital locations.
	DEFW	free	;[Early] Start of free memory
                    ;[Late]  Word/object mapping
	DEFW	freeend	;End of free memory
	DEFS	3	;?
	DEFB	80h,dict ;Expansion dictionary (for compressed games).
			         ;Dictionary is stored in ASCII, high bit set on last
			         ;letter of each word.
rsptab:	...		     ;response	- both stored as: DB verb, noun
protab: ...          ;process                     DW address-of-handler
                     ;                               terminated by verb 0.
                                         Handler format is:
                                          DB conditions...0FFh
                                          DB actions......0FFh
objtab:	...		;objects	- all are stored with all bytes
objtop:	DEFW	objtab        complemented to deter casual hackers.
        DEFW    object1
        DEFW    object2 etc.
loctab:	...		;locations       Texts are terminated with 0E0h
loctop:	DEFW    loctab           (ie 1Fh after decoding).
        DEFW    locno1
        DEFW    locno2 etc.
msgtab:	...		;messages
msgtop:	DEFW    msgtab
        DEFW    mess1
        DEFW    mess2 etc.
conntab: ...    ;connections    - stored as DB WORD,ROOM,WORD,ROOM...
                                  each entry is terminated with 0FFh.

voctab: ...		;vocabulary     - stored as DB 'word',number - the
                                  word is complemented to deter hackers.
                                  Table terminated with a word entry all
                                  five bytes of which are 0 before
                                  decoding.
oloctab: ...    ;initial locations of objects. 0FCh => not created
                                               0FDh => worn
                                               0FEh => carried
                                               0FFh => end of table

In later games (those made with Illustrator?), there is an extra byte 
(the number of system messages) after the number of messages, and at 
the label ** the following word is inserted:

        DEFW    systab  ;Points to word pointing to table of system
                    ;messages.
systab: DEFW    sysm0
        DEFW    sysm1 etc.

The address of the user-defined graphics is stored at 23675. In "Early" games,
the system messages are at UDGs+168.

CPC differences:

* I don't know where the UDGs are.
* Strings are terminated with 0xFF (0 after decoding) rather than 0xE0 
  (0x1F).
* No Spectrum colour codes in strings. Instead, the 
  code 0x14 means "newline" and 0x09 means "toggle reverse video".
  There are other CPC colour codes in the range 0x01-0x0A, but I don't
  know their meaning.
* I have assumed that the database always starts at 0x1BD1, which it does 
  in the snapshots I have examined.
  
Commodore 64 differences:

* I don't know where the UDGs are.
* Strings are terminated with 0xFF (0 after decoding) rather than 0xE0 
  (0x1F).
* No Spectrum colour codes in strings. 
* I have assumed that the database always starts at 0x0804, which it does 
  in the snapshots I have examined.

  
*/


#define MAINMODULE
#include "unquill.h"    /* Function prototypes */

char *tty_gets(char *buf, int count)
{
	fflush(stdout);
	return fgets(buf, count, stdin);
}

void tty_putchar(char ch)
{
	if (((uchr)ch) > 0x7f) ch = '?';
	fputc(ch, outfile);
}

void tty_clrscr(void)
{
	char n;

	for (n = 0; n < 24; n++) opch32('\n');
}

void tty_nop(uchr ch) { }
void tty_nop2(uchr *d, ushrt f, ushrt c) { }
void tty_setwindow(int w, int h) { }

CONSOLE tty = 
{
	tty_setwindow,	/* Set window dimensions */
	tty_gets,	/* Get input line */
	tty_putchar,	/* Output character */
	tty_clrscr,	/* Clear screen */
	tty_nop,	/* Show / hide cursor */
	tty_nop,	/* Set ink */
	tty_nop,	/* Set paper */
	tty_nop,	/* Set flash */
	tty_nop,	/* Set bright */
	tty_nop,	/* Set inverse */
	tty_nop,	/* Set over */
	tty_nop,	/* Set border */
	tty_nop2	/* Load UDGs */
};

CONSOLE *console = &tty;

#if DOS
/* Make this support the SWITCHAR in DRDOS and MSDOS 2-3 */

static uchr gl_switchar = 0;
static int  gl_display = -1;

#define isoptch(c) ( (c) == '/' || (c) == '-' || (c) == gl_switchar)
#endif

#if CPM
#define isoptch(c) ( (c) == '/' || (c) == '-' || (c) == '[')
#endif

#ifndef isoptch
#define isoptch(c) ( (c) == '-' )
#endif

ushrt check_signature(ushrt n);


int main(int argc, char *argv[])
{
	ushrt n, zxptr = 0, seekpos;
	char *ofptr = "";
	uchr endopt = 0;	/* Encountered a "--" option? */
#if DOS
	/* Get the system switch character */
	union REGS rg;

	rg.x.ax = 0x3700;
	intdos(&rg, &rg);
	if (rg.h.al == 0)
	{
		gl_switchar = rg.h.dl;
	}
#endif

	inname = NULL;
	
/* If command looks like a help command, print helpscreen */

	if ((argc == 1)   || (isoptch(AV10) && strchr("hH/?", AV11)))
		usage(AV0);
	
/* Parse command line.
 * [0.9.0] Don't require the input filename to come first. */

	for (n = 1;n < argc; n++)
	{
		/* Not an option character? => filename */
		if (endopt || !isoptch(argv[n][0]))
		{
			if (inname == NULL) inname = argv[n];
			else usage(AV0);
		}
		else	/* An option character */
		{
			switch (argv[n][1])
      			{
#if DOS
				case 'D':
				case 'd': switch (argv[n][2])
				{
					case '2':           gl_display = 6; break;
					case '4':           gl_display = 5; break;
					case '3':           gl_display = 4; break;
					case 'C': case 'c': gl_display = 2; break;
					case 'E': case 'e': gl_display =13; break;
					case 'M': case 'm': gl_display = 7; break;
					case 'T': case 't': gl_display = 0; break;
					case 'V': case 'v': gl_display =19; break;
					default:  fprintf(stderr,"%s: Incorrect -D option. Type %s -H for help \n",AV0, AV0);
            					  exit(1);
				}
				break;
					
#endif /* DOS */
      				case 'T': 
      				case 't': switch (argv[n][2])
          			{
          			        case 'Z':
					case 'z': oopt = 'Z'; 
						  if (argv[n][3] == 'B' || argv[n][3] == 'b') oopt = 'z';
						  break;

					case '5': oopt = '5'; break;

#ifndef TINY
					case 'I':
					case 'i': oopt = 'I'; break;
#endif
					case 'X':
					case 'x': oopt = 'X'; break;

					case 'r':
					case 'R': if (ofarg) fprintf(stderr, "%s: -O present. Ignoring -TR\n",AV0);
     						  else
     						  {
     						  	oopt    = 'R';
							running = 1;
              					  }
            					  break;
					default:  fprintf(stderr,"%s: Incorrect -T option. Type %s -H for help \n",AV0, AV0);
            					  exit(1);
           			}
           			break;
           			
			        case 'O':
			        case 'o': if (running)
				{
					fprintf(stderr, "%s: -O present. Ignoring -TR\n",AV0);
					running = 0;
					oopt    = 'T';
          			}
				ofarg = n;
				ofptr = (argv[n]+2);
				break;

			        case 'L':
			        case 'l': dbver = 10; break;

				case 'C': 
				case 'c': copt++; break;

				case 'g': 
				case 'G': gopt++; break;

				case 'm':
				case 'M': mopt++; break;

			        case 'S': 
			        case 's': switch(argv[n][2])
				{
					case 'C': case 'c': skipc = 1; break;
					case 'F': case 'f': skipf = 1; break;
					case 'G': case 'g': skipg = 1; break;
					case 'O': case 'o': skipo = 1; break;
					case 'M': case 'm': skipm = 1; break;
					case 'N': case 'n': skipn = 1; break;
					case 'L': case 'l': skipl = 1; break;
					case 'V': case 'v': skipv = 1; break;
					case 'S': case 's': skips = 1; break;
					case 'U': case 'u': skipu = 1; break;
				        default:
            				fprintf(stderr,"%s: Syntax error. -S only takes C,O,M,N,L,V,S,U\n",AV0);
            				exit(1);
            			}
			        break;
 
 				case 'V':
 				case 'v': verbose = 1; break;

				case 'Q':
				case 'q': nobeep = 1; break;
		
				case '-': endopt = 1; break;
	
				default:
        			fprintf(stderr,"%s: Invalid option: %s\n",AV0,argv[n]);
        			exit(1);
        		}	/* End switch */
		}	/* End For */
    	}	/* End If */

  /* If output file was specified, select it */

 	if (ofarg)
	{
		char opt[3];
		
		if (oopt == '5') strcpy(opt, "wb");
		else		 strcpy(opt, "w");
		if ((outfile = fopen(ofptr, opt)) == NULL)
		{
			perror(ofptr);
			exit(1);
		}
	}
	else
	{
		outfile=stdout;
		if (oopt == '5' && !copt)
		{
			fprintf(stderr,"%s: Z-Code files are not written to "
                                       "standard output. If this is really\n"
                                       "what you want to do, use the -C "
                                       "option.\n\n", AV0);
			exit(0);
		}
	}
 /* Load the snapshot. To save space, we ignore the printer
  buffer and the screen, which can contain nothing of value. */

	if (!inname) usage(AV0);

	if((infile = fopen(inname,"rb")) == NULL)
	{

/* Since perror() in the CP/M version of Hi-Tech C isn't terribly good, I'll
  sometimes make special arrangements for CP/M error reporting. */

#ifdef CPM
		fprintf(stderr,"%s: Cannot open file.\n",inname);
#else
		perror(inname);
#endif
		exit(1);
	}

/* << v0.7: Check for CPC6128 format */
	if (fread(snapid, sizeof(snapid), 1, infile) != 1)
	{
#ifdef CPM
		fprintf(stderr,"%s: Not in Spectrum, CPC or C64 snapshot format.\n",inname);
#else
		perror(inname);
#endif

	}

	arch       = ARCH_SPECTRUM;
	seekpos    = 0x1C1B;	/* Number of bytes to skip in the file */
	mem_base   = 0x5C00;	/* First address loaded */
	mem_size   = 0xA400;	/* Number of bytes to load from it */
	mem_offset = 0x3FE5;	/* Load address of snapshot in host system memory */
	
	if (!memcmp(snapid, "MV - SNA", 9))	/* CPCEMU snapshot */
	{
		arch = ARCH_CPC;

		seekpos    = 0x1C00;
		mem_base   = 0x1B00;
		mem_size   = 0xA500;
		mem_offset = -0x100;
		dbver = 10;	/* CPC engine is equivalent to  
                                 * the "later" Spectrum one. */

		fprintf(stderr,"CPC snapshot signature found.\n");
	}
	if (!memcmp(snapid, "VICE Snapshot File\032", 19))	/* VICE snapshot */
	{
		int n;
		unsigned char header[256];	

		arch = ARCH_C64;
		memset(header, 0, sizeof(header));
		/* [0.8.7] Do a quick, minimal parse of the VSF to find the 
		 * C64MEM block. Earlier unquills assumed it would be at a 
		 * fixed offset; it isn't. */
		fseek(infile, 0, SEEK_SET);
		if (fread(header, 1, sizeof(header), infile) < (int)sizeof(header))
		{
			fprintf(stderr,"Warning: Failed to read C64 snapshot header\n");
		}
		
		seekpos    =  0x874;
		mem_base   =  0x800;
		mem_size   = 0xA500;
		mem_offset =  -0x74;
		for (n = 0; n < (int)sizeof(header) - 6; n++)
		{
			if (!memcmp(header + n, "C64MEM", 6))
			{
				seekpos = 0x800 + n + 0x1A;
				mem_offset = - (n + 0x1A);
				break;
			}
		}		


		dbver = 5;	/* C64 engine is between the two Spectrum 
                     		 * ones. */

		fprintf(stderr,"C64 snapshot signature found.\n");
	}
/* >> v0.7 */

/* Skip screen/printer buffer/registers and load the rest */

#ifndef TINY

	if (fseek(infile,seekpos,SEEK_SET))
	{
		perror(inname);
		exit(1);
	}

/* Load file */

	if (fread(zxmemory, mem_size, 1, infile) != 1)
	{
		perror (inname);
		exit(1);
	}

#endif  /* ndef TINY */

/* .SNA read ok. Find a Quill signature */

	switch(arch)
	{
		case ARCH_SPECTRUM:

		/* I could _probably_ assume that the Spectrum database is 
		 * always at one location for "early" and another for "late"
		 * games (0x6D04 for "early", 0x6B85 for "late". Try these
		 * first; if that fails, scan the whole file. */
		
			if (check_signature(0x6D04)) 
				{ dbver = 0;  found = 1; zxptr = 0x6D11; break; }
			if (check_signature(0x6B85)) 
				{ dbver = 10; found = 1; zxptr = 0x6B92; break; }
	
		    	for (n = 0x5C00; n < 0xFFF5; n++)
				{
					if (check_signature(n))
					{
						fprintf(stderr,"Quill signature found.\n");
						found = 1;
						zxptr = n + 13;
						break;
					}
				}
				break;

		case ARCH_CPC: 
		        found = 1;
			zxptr = 0x1BD1;	/* From guesswork: CPC Quill files
			                 * always seem to start at 0x1BD1 */
			break;
		case ARCH_C64: 
		        found = 1;
			zxptr = 0x804;	/* From guesswork: C64 Quill files
			                 * always seem to start at 0x804 */
			break;
	}
	
	if (!found)
	{
		fprintf(stderr,"%s does not seem to be a valid Quill .SNA file. If you\n",
          		inname);
		fprintf(stderr,"know it is, you have found a bug in %s.\n",AV0);
  		exit(1);
  	}

#ifdef TINY      /* Fill the first buffer */

	if (fseek(infile, zxptr - mem_offset, SEEK_SET))
	{
#ifdef CPM
		fprintf(stderr, "Can't fseek to %x in %s\n", zxptr - mem_offset,
			inname);
#else
		perror(inname);
#endif
		exit(1);
	}

/* Load first buffer (pointers & dictionary) */

	b1base = zxptr;
	if (fread(buf1, sizeof(buf1), 1, infile) != 1)
	{
#ifdef CPM
		fprintf(stderr, "Can't load buffer at %x from %s\n", 
					b1base, inname);
#else
		perror (inname);
#endif
		exit(1);
	}
	b1full++;

#endif  /* def TINY */

	ucptr   = zxptr;
	maxcar1 = maxcar = zmem (zxptr);	/* Player's carrying capacity */
	nobj             = zmem (zxptr +  1);	/* No. of objects */
	nloc             = zmem (zxptr +  2);	/* No. of locations */
	nmsg             = zmem (zxptr +  3);	/* No. of messages */
	if (dbver)
 	{
  		++zxptr;
		nsys     = zmem (zxptr +  3);	/* No. of system messages */
		vocab    = zword(zxptr + 18);	/* Words list */
  		dict     = zxptr + 29;		/* Expansion dictionary */
	}
	else vocab = zword(zxptr + 16);

#ifdef TINY      /* Fill the second buffer */

	if (fseek(infile, vocab - mem_offset, SEEK_SET))
	{
#ifdef CPM
                fprintf(stderr, "Can't fseek to %x in %s\n", vocab - mem_offset,
                        inname);
#else
                perror(inname);
#endif
		exit(1);
	}

/* Load second buffer (vocabulary) */

	b2base = vocab;
	if (fread(buf2, sizeof(buf2), 1, infile) != 1)
	{
#ifdef CPM
                fprintf(stderr, "Can't load buffer at %x from %s\n", 
				b2base, inname);
#else
                perror (inname);
#endif
		exit(1);
	}
	b2full++;

#endif  /* def TINY */
	resptab            = zword(zxptr +  4);
        proctab            = zword(zxptr +  6);
	objtab             = zword(zxptr +  8);
	loctab             = zword(zxptr + 10);
	msgtab             = zword(zxptr + 12);
	if (dbver) sysbase = zword(zxptr + 14);
  	else       sysbase = zword(23675) + 168; /* Just after the UDGs */ 
	conntab            = zword(zxptr + 14 + ( dbver ? 2 : 0));
	if(dbver) objmap   = zword(zxptr + 22);
	postab             = zword(zxptr + 18 + ( dbver ? 2 : 0 ));

	switch(oopt)
	{
		case 'T':	/* Text */ 
		case 'Z':	/* ZXML */
		case 'z':	/* Formatted ZXML */

		write_header();
		if (!skipc)
		{
			fprintf(outfile, "%4x: Response [Event] table\n", zword(zxptr + 4));
			listcond(resptab);
			fprintf(outfile, "%4x: Process [Status] table\n", zword(zxptr + 6));
			listcond(proctab);
		}
		if (!skipo) listitems("Object",   (zword(objtab)), nobj);
		if (!skipl) listitems("Location", (zword(loctab)), nloc);
		if (!skipm) listitems("Message",  (zword(msgtab)), nmsg);
		if (!skipn) listconn (             zword(conntab), nloc);
		if (!skipv) listwords(vocab);
		if (!skipo) listpos(postab, nobj);
		if ((!skipo) && (dbver >= 10)) listmap(objmap, nobj);

		if (!skips)
		{
			if (dbver > 0) listitems("System message", (zword(sysbase)), nsys);
			 else       listitems("System message", sysbase, 32);
		}
		if (!skipu && arch == ARCH_SPECTRUM) listudgs(zword(23675));
		if (!skipf && arch == ARCH_SPECTRUM) listfont(zword(23606));
		if (!skipg && arch == ARCH_SPECTRUM && dbver > 0) listgfx();

/** WARNING ** gotos here */

closeout:
		write_trailer();
		if(fclose(infile))
		{
#ifdef CPM
			fprintf (stderr,"%s: cannot close file.\n",inname);
#else
			perror (inname);
#endif
			exit(1);
		}

		if (ofarg && fclose(outfile))
		{
			perror(ofptr);
			exit(1);
		}
		break;

		case '5':
		zcode_binary();
		goto closeout;	/** WARNING ** Uses goto */
#ifndef TINY	
		case 'I':	/* Inform source */
		inform_src(zxptr);
		goto closeout;	/** WARNING ** Uses goto */	
#endif	
		case 'X':
		write_header();
		if (!skipc)
		{
			xlistcond("response", resptab);
			xlistcond("process", proctab);
		}
		if (!skipo) xlistobjs(objtab, postab, objmap, nobj);
		if (!skipl) xlistlocs(loctab, conntab, nloc);
		if (!skipm) xlistmsgs("message", msgtab, nmsg);
		if (!skipv) xlistwords(vocab);

		if (!skips)
		{
			if (dbver > 0) xlistmsgs("sysmsg", sysbase, nsys);
			else           xlistsys("sysmsg", sysbase, 32);
		}
		if (!skipu && arch == ARCH_SPECTRUM) xlistudgs(zword(23675));
		if (!skipf && arch == ARCH_SPECTRUM) xlistfont(zword(23606));
		if (!skipg && arch == ARCH_SPECTRUM && dbver > 0) xlistgfx();
		
		goto closeout;	/** WARNING ** Uses goto */	

		case 'R':	/* Run the game */
#if DOS	
		if (gl_display != 0)
		{
			video_init(gl_display);
		}
#endif
		if (arch == ARCH_SPECTRUM)
			(console->set_window)(32, 24);
		else	(console->set_window)(40, 25);	
		ramsave[0x100] = 0; /* No position RAMsaved */
		while(running)
		{
/* Load default font, default UDGs */
			if (arch == ARCH_SPECTRUM)
			{
				uchr font[768];
				ushrt ubase = zword(23606) + 256;
				if (ubase >= 0x5C00)
				{
					for (n = 0; n < 768; n++)
					{
						font[n] = zmem(ubase + n);
					}
					(console->load_font)(font, 32, 96);
				}
				ubase = zword(23675);
				for (n = 0; n < 168; n++)
				{
					font[n] = zmem(ubase + n);
				}
				(console->load_font)(font, 144, 21);
			}

			estop = 0;   
			srand(1);
			oopt  = 'T';        /* Outputs in plain text */
			initgame(zxptr); /* Initialise the game */
			playgame(zxptr); /* Play it */ 
			if (estop)
			{
				estop=0;	/* Emergency stop operation, game restarts */
	    			continue;   /* automatically */
    			}
			sysmess(13);
			opch32('\n');
			if (yesno()=='N')
			{
				running=0;
				sysmess(14);
			}
		}
    		putchar('\n');
    		break;
	}	/* End switch */
	return 0;
}	/* End main() */




void usage(char *title)
{
  fprintf(stderr,"UnQuill v" VERSION " - John Elliott, " BUILDDATE "\n\n");
  fprintf(stderr,"Command format is %s {-Ooutput-file} {-opt -opt ... } quillfile\n\n",title);
  fprintf(stderr,"  Decompiles .SNA snapshots of Quilled games to text.\n");
  fprintf(stderr,"Options are:\n");
  fprintf(stderr,"-TR: Run the game!\n");
#ifndef TINY
  fprintf(stderr,"-TI: Output a source file for Inform\n");
#endif
  fprintf(stderr,"-T5: Output a Z5 file for Z-code interpreters\n");
  fprintf(stderr,"-TZ: Outputs text in ZXML (converts Spectrum colour/\n");
  fprintf(stderr,"     flash controls to HTML-style <ATTR> commands).\n");
  fprintf(stderr,"-C  : Force output of Z-code to standard output\n");

#if (DOS || CPM) 
  fprintf(stderr, "[more]"); fflush(stderr); getch(); fprintf(stderr, "\n");
#endif
#if DOS
  fprintf(stderr,"-D  : Display type when running the game\n");
  fprintf(stderr,"      -D4 : Use 4-colour CGA graphics mode\n");
  fprintf(stderr,"      -DC : Use 16-colour text mode\n");
  fprintf(stderr,"      -DE : Use 16-colour EGA graphics mode\n");
  fprintf(stderr,"      -DM : Use mono text mode\n");
  fprintf(stderr,"      -DT : 'Teletype' output, no colours or graphics\n");
  fprintf(stderr,"      -DV : Use 256-colour VGA/MCGA graphics mode\n");

#endif
  fprintf(stderr,"-G  : Make the Z-code file use graphical drawing characters\n");
  fprintf(stderr,"-M  : Make the Z-code file not use colours\n");
  fprintf(stderr,"-L  : Attempt to interpret as a 'later' type Quill file.\n");
  fprintf(stderr,"-O  : Redirect output to a file. If this option is not\n");
  fprintf(stderr,"     present, stdout is used.\n");
  fprintf(stderr,"-Sx : Skip section x. x can be one of:\n");
  fprintf(stderr,"     C - condition tables\n");
  fprintf(stderr,"     F - user-defined font\n");
  fprintf(stderr,"     G - location graphics\n");
  fprintf(stderr,"     L - location texts\n");
  fprintf(stderr,"     M - message texts\n");
  fprintf(stderr,"     N - coNNections\n");
  fprintf(stderr,"     O - object texts\n");
  fprintf(stderr,"     S - system messages\n");
  fprintf(stderr,"     U - User-defined graphics\n");
  fprintf(stderr,"     V - vocabulary tables\n");
#if DOS
  fprintf(stderr, "[more]"); fflush(stderr); getch(); fprintf(stderr, "\n");
#endif
  fprintf(stderr,"-V : Verbose. Annotate condition and connection tables\n");
  fprintf(stderr,"    with message, object and location texts.\n");
  fprintf(stderr,"-Q : Quiet (no beeping)\n");
  fprintf(stderr,"Mail bug reports to <jce@seasip.info>.\n");
  exit(1);
}

/* Based on an idea by Staffan Vilcans: Skip the fseek() if it isn't needed */
#ifdef TINY
static ushrt last_addr = 0;
#endif

uchr zmem(ushrt addr)
{
/* All Spectrum memory accesses are routed through this procedure.
 * If TINY is defined, this accesses the .sna file directly.
 */

	if (addr < mem_base || (arch != ARCH_SPECTRUM && 
	                       (addr >= (mem_base + mem_size))))
	{
		fprintf (stderr,"\nInvalid address %4.4x requested. ", addr);
		if (arch != ARCH_SPECTRUM)
		     fprintf(stderr,"Probably not a Quilled game.\n");
		else fprintf(stderr, "Try %s the -L option.\n",
		                ((dbver > 0) ? "omitting":"including"));
		exit(1);
	}
#ifdef TINY

	if     ((b1full) && (addr >= b1base)  && (addr < (b1base + 0x400)))
    		return (buf1[addr-b1base]);
	else if ((b2full) && (addr >= b2base) && (addr < (b2base + 0x400)))
		return (buf2[addr-b2base]);

/* If we're reading at address x+1, the fseek isn't necessary */

	if (addr != last_addr + 1)
	{
		if (fseek(infile,addr - mem_offset,SEEK_SET))
		{
			perror(inname);
			exit(1);	
		}
	}
	last_addr = addr;
	return (fgetc(infile));

#else

	return zxmemory[addr - mem_base];

#endif
}



ushrt zword(ushrt addr)
{
	return (ushrt)(zmem(addr) + (256 * zmem(addr + 1)));
}



void dec32(ushrt v)
{
	char decbuf[6];
#ifdef TINY
	char i;   /* 8-bit register on 8-bit computer */
#else
	int i;
#endif
	
	sprintf(decbuf,"%d",v);
	i=0;
	while (decbuf[i]) opch32(decbuf[i++]);
}


void opstr32(char *str)
{
	while (*str)
	{
		opch32(*str);
		++str;
	}
}

void opch32(char ch)	/* Output a character, assuming 32-column screen */
{
	char x;

	(console->putch)(ch);
	if   (ch == '\n')
	{
		xpos = 0;
		if (oopt == 'I' && comment_out) fputs("! ", outfile);
		if (indent)
		{
			for (x = 0; x < indent; x++) fputc(' ',outfile);
        		if (oopt != 'I') fputc(';',outfile);
        	}
      		else if (!running) fprintf(outfile,"      ");
                if (oopt == 'I') fputc('^', outfile);
    	}
    	else if (ch == 8) xpos--;
	else if (arch == ARCH_SPECTRUM && xpos == 31) opch32('\n');
	else if (arch != ARCH_SPECTRUM && xpos == 39) opch32('\n');
	else xpos++;
}


ushrt check_signature(ushrt n)
{
	if ((zmem(n  ) == 0x10) && (zmem(n+2) == 0x11) && 
	    (zmem(n+4) == 0x12) && (zmem(n+6) == 0x13) && 
	    (zmem(n+8) == 0x14) && (zmem(n+10) == 0x15)) return 1;
	return 0;
}	

#ifndef YES_GETCH


char getch(void)
{
	struct termios ts, ots;
        char c;
	int i;

	int filen = fileno(stdin);
        tcgetattr(filen, &ts);		/* Switch the terminal to raw mode */
        tcgetattr(filen, &ots);
        cfmakeraw(&ts);
        tcsetattr(filen, TCSANOW, &ts);
        
        fflush(stdout);
        fflush(stdin);
	do
	{
		i = read(filen, &c, 1);
	}
	while (i < 0);
        
        tcsetattr(filen, TCSANOW, &ots);

	fflush(stdin);
	return c;
}
#endif

void write_header(void)
{
	switch(oopt)
	{
		case 'T':	/* Text */ 
		case 'Z':	/* ZXML */
		case 'z':	/* Formatted ZXML */
			fprintf(outfile,"      ; Generated by UnQuill " VERSION "\n");	
			fprintf(outfile,"%4x: Player can carry %d objects.\n\n", ucptr, maxcar);
			break;
		case 'X':
			fprintf(outfile, "<?xml version=\"1.0\" ?>\n");
			fprintf(outfile, "<quill>\n");
			fprintf(outfile, "<!-- Generated by UnQuill " VERSION " -->\n");
			fprintf(outfile, "  <maxcarried>%d</maxcarried>\n", 
						maxcar);
			break;
	}
}


void write_trailer(void)
{
	switch(oopt)
	{
		case 'X':
			fprintf(outfile, "</quill>\n");
			break;
	}
}
