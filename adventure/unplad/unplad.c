/*
    UnPLAD: Dumps the database in Artic adventure games A-D
    Copyright 2012, John Elliott <jce@seasip.demon.co.uk>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

unsigned char snap[49179];
const char *filename = NULL;
FILE *outfile;


/* It would be simpler to use a 64k buffer rather than subtracting an
 * offset the entire time, but Pacific C can't handle a buffer that big */
#ifndef __PACIFIC__
inline 
#endif
unsigned char peek8(unsigned offset)
{
	return snap[offset - 0x3FE5];
}

unsigned peek16(unsigned offset)
{
	return peek8(offset+1) * 256 + peek8(offset);
}

unsigned prg_start, prg_end;
unsigned room_base, room_count;
unsigned obj_base, obj_count, obj_loc;
unsigned msgs_base, msgs_count;
unsigned conn_base;
unsigned vocab;
unsigned response;
unsigned process;
unsigned carry;
unsigned print;

/* Operation flags */
unsigned verbose;
unsigned skip_cond, skip_msg, skip_loc, skip_obj, skip_voc, skip_conn, skip_sys;
int charset;	/* Charset = 0 for ZX81, 1 for ASCII */
int platform;	/* Platform = 1 for ZX81, 2 for Spectrum */

int analyse(void)
{
	unsigned n;
/* Attempt to locate the tables. Note: Adventures A and B seem to have a 
 * different version of the game engine -- Adventure A uses IX to index 
 * internal variables, Adventure B uses IY, so we have to check for both
 * sets of code sequences. */

	carry = 0;
	room_base = obj_base = conn_base = process = response = msgs_base = 
	vocab = obj_loc = 0;	
	switch (platform)
	{
		case 1:	prg_start = peek16(0x4029);	/* NXTLIN */
			prg_end   = peek16(0x4010);	/* VARS */
			break;
		case 2: prg_start = peek16(0x5C4B);	/* VARS */
			prg_end   = peek16(0x5CB2);	/* RAMTOP */
			charset = 1;
			break;
	}
	for (n = prg_start; n < prg_end; n++)
	{
		/* Detect Adventure D on the ZX81, which uses ASCII 
		 * internally */
		if (peek8(n  ) == 0xFE && peek8(n+1) == 0x60 &&
		    peek8(n+2) == 0x38 && peek8(n+3) == 0x02)
		{
			charset = 1;
		}
		/* Adventure B */
		if (peek8(n  ) == 0x11 && peek8(n+3) == 0xFD && 
		    peek8(n+4) == 0x6E && peek8(n+5) == 0x42)
		{
			room_base = peek16(n + 1);
		}
		/* Adventure A */
		if (peek8(n  ) == 0x11 && peek8(n+3) == 0xDD && 
		    peek8(n+4) == 0x6E && peek8(n+5) == 0xFF)
		{
			room_base = peek16(n + 1);
		}
		if (peek8(n  ) == 0xE5 && peek8(n+1) == 0x21 &&
		    peek8(n+4) == 0x06 && peek8(n+5) == 0x00 &&
		    peek8(n+6) == 0x09 && peek8(n+7) == 0x09 && !obj_base)
		{
			obj_base = peek16(n + 2);
		}
		/* Adventure B */
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0xFD && 
		    peek8(n+4) == 0x36 && peek8(n+5) == 0x3D &&
		    peek8(n+6) == 0x00)
		{
			response = peek16(n + 1);
		}
		/* Adventure A */
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0xDD && 
		    peek8(n+4) == 0x36 && peek8(n+5) == 0xFA &&
		    peek8(n+6) == 0x00)
		{
			response = peek16(n + 1);
		}
		/* Adventures A & B */
		if (peek8(n  ) == 0x23 && peek8(n+1) == 0x0C &&
		    peek8(n+2) == 0x18 && peek8(n+3) == 0xBA &&
		    peek8(n+4) == 0x21)
		{
			process = peek16(n + 5);
		}
		/* Adventure B */
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0x06 && 
		    peek8(n+4) == 0x00 && peek8(n+5) == 0xFD &&
		    peek8(n+6) == 0x4E && peek8(n+7) == 0x3C &&
		    peek8(n+8) == 0x09 && peek8(n+9) == 0x09 &&
		    peek8(n+10)== 0x5E && peek8(n+11)== 0x23 &&
		    peek8(n+12)== 0x56)
		{
			msgs_base = peek16(n + 1);
			print     = peek16(n + 15);
		}
		/* Adventure A */
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0x06 && 
		    peek8(n+4) == 0x00 && peek8(n+5) == 0xDD &&
		    peek8(n+6) == 0x4E && peek8(n+7) == 0xF9 &&
		    peek8(n+8) == 0x09 && peek8(n+9) == 0x09 &&
		    peek8(n+10)== 0x5E && peek8(n+11)== 0x23 &&
		    peek8(n+12)== 0x56)
		{
			msgs_base = peek16(n + 1);
			print     = peek16(n + 15);
		}
		/* Adventure B */
		if (peek8(n  ) == 0x16 && peek8(n+1) == 0x00 && 
		    peek8(n+2) == 0xFD && peek8(n+3) == 0x5E &&
		    peek8(n+4) == 0x42 && peek8(n+5) == 0x21)
		{
			conn_base = peek16(n + 6);
		}
		/* Adventure A */
		if (peek8(n  ) == 0x16 && peek8(n+1) == 0x00 && 
		    peek8(n+2) == 0xDD && peek8(n+3) == 0x5E &&
		    peek8(n+4) == 0xFF && peek8(n+5) == 0x21)
		{
			conn_base = peek16(n + 6);
		}
		/* Adventure B */
		if (peek8(n  ) == 0xFD && peek8(n+1) == 0x7E && 
		    peek8(n+2) == 0x44 && peek8(n+3) == 0xFE)
		{
			carry = n + 4;
		}
		/* Adventure A */
		if (peek8(n  ) == 0xDD && peek8(n+1) == 0x7E && 
		    peek8(n+2) == 0x01 && peek8(n+3) == 0xFE)
		{
			carry = n + 4;
		}
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0x3E && 
		    peek8(n+4) == 0xFF && peek8(n+5) == 0xF5)
		{
			vocab = peek16(n + 1);	
		}
		/* Adventure B */
		if (peek8(n  ) == 0xED && peek8(n+1) == 0xB0 && 
		    peek8(n+2) == 0xFD && peek8(n+3) == 0x36 &&
		    peek8(n+4) == 0x42 && peek8(n+4) == 0x00)
		{
			obj_loc = peek16(n - 8);
			obj_count = peek16(n - 2);
		}	
		/* Adventure A */
		if (peek8(n  ) == 0xED && peek8(n+1) == 0xB0 && 
		    peek8(n+2) == 0xDD && peek8(n+3) == 0x36 &&
		    peek8(n+4) == 0xFF && peek8(n+4) == 0x00)
		{
			obj_loc = peek16(n - 8);
			obj_count = peek16(n - 2);
		}	
	}
	if (!room_base || !conn_base || !obj_base || 
	    !msgs_base || !process   || !response ||
	    !vocab)
	{
		fprintf(stderr, "Cannot find database\n"
				"room_base=%04x conn_base=%04x obj_base=%04x\n"
				"msgs_base=%04x process=%04x response=%04x\n"
				"vocab=%04x\n",
			room_base, conn_base, obj_base, msgs_base, process,
			response, vocab);
		return -1;
	}
/* The game does not hold a count of rooms or objects, so try to guess at
 * one by running down the pointers and stopping if one seems 'wrong'. */
	for (n = room_base; peek16(n) >= prg_start && peek16(n) < prg_end &&
				n != peek16(room_base) && n < prg_end; n += 2)
	{
		++room_count;
	}
	for (n = msgs_base; peek16(n) >= prg_start && peek16(n) < prg_end &&
				n != peek16(msgs_base) && n < prg_end; n += 2)
	{
		++msgs_count;
	}

	return 0;
}

/* Output a string, either in the ZX81 character set or in ASCII. */
void prstring(unsigned addr, int indent)
{
	static const char zx81cs[64] = " abcdefghij\"£$:?()><=+-*/;,.0123"
		                       "456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int rv = 0;	
	char ch, eos;

	eos = charset ? 0 : 1;
	while ( (ch = peek8(addr)) != eos)
	{
		if (charset)	/* ASCII */
		{
			if (ch >= ' ' && ch < '{') 
			{
				fputc(ch, outfile);
			}
			else if (ch == '\r' || ch == '\n')
			{
				fprintf(outfile, "\n%-*.*s", indent, indent, " ");
				if (indent > 6) fputc(';', outfile);
			}
			else
			{
				fprintf(outfile, "{%02x}", ch & 0xFF);
			}
			++addr;
		}
		else	/* ZX81 charset */
		{
			if ((0x80 & ch) && !rv)
			{
				fputc('[', outfile);
				rv = 1;
			}
			if ((!(0x80 & ch)) && rv)
			{
				fputc(']', outfile);
				rv = 0;
			}
			ch &= 0x7F;
			if (ch == 0x76) 
			{
				fprintf(outfile, "\n%-*.*s", indent, indent, " ");
				if (indent > 6) fputc(';', outfile);
			}
			else if (ch > 0x3F) fputc('?', outfile);
			else fputc(zx81cs[ch & 0x3f], outfile);
			++addr;
		}
	}
}



void dump_rooms(void)
{
	unsigned n, addr;

	for (n = 0; n < room_count; n++)
	{
		addr = peek16(room_base + 2 * n);
		fprintf(outfile, "\n%04x: Location %d\n      ", addr, n);
		prstring(addr, 6);	
	}
	fprintf(outfile, "\n");
}


void dump_msgs(void)
{
	unsigned n, addr;

	for (n = 0; n < msgs_count; n++)
	{
		addr = peek16(msgs_base + 2 * n);
		fprintf(outfile, "\n%04x: Message %d\n      ", addr, n);
		prstring(addr, 6);	
	}
	fprintf(outfile, "\n");
}


/* Notes: Unlike Quilled games, the system messages live in the database 
 * engine rather than in an easily-dumpable table. Fortunately, as far as I
 * can see, they're all printed by a standardised code snipped (LD HL, addr
 * followed by CALL PRINT) so a scan for this snippet should pick them all
 * up. */
void dump_sysmsgs(void)
{
	unsigned n, addr;

	for (n = prg_start; n < prg_end; n++)
	{
		if (peek8(n  ) == 0x21 && peek8(n+3) == 0xCD &&
		    peek8(n+4) == (print & 0xFF) && peek8(n+5) == (print >> 8))
		{
			addr = peek16(n + 1);
			fprintf(outfile, "\n%04x: System message:\n      ",
					addr);
			prstring(addr, 6);
		}
	}
}

void dump_objects(void)
{
	unsigned n, addr;

	for (n = 0; n < obj_count; n++)
	{
		addr = peek16(obj_base + 2 * n);
		fprintf(outfile, "\n%04x: Object %d\n      ", addr, n);
		prstring(addr, 6);	
	}
	fprintf(outfile, "\n");
}


void dump_objpos(void)
{
	unsigned n, addr = obj_loc;

	for (n = 0; n < obj_count; n++, addr++)
	{
		fprintf(outfile, "\n%04x: Object %3d is initially ", addr, n);
		switch (peek8(addr))
		{
			case 0xFF: fprintf(outfile, "invalid location."); 
				   fprintf(stderr, "Warning: Invalid location"
					" in object positions table.\n"); 
				   break;
			case 0xFE: fprintf(outfile, "carried."); break;
			case 0xFD: fprintf(outfile, "worn."); break;
			case 0xFC: fprintf(outfile, "not created."); break;
			default: fprintf(outfile, "in room %3d.", peek8(addr));
				 break;
		}
		if (verbose)
		{
			fprintf(outfile, "\n%-44.44s;", "");
			prstring(peek16(obj_base + 2 * n), 44);
			if (peek8(addr) < 252)
			{
				fprintf(outfile, "\n%-44.44s;", "");
				prstring(peek16(room_base + 2 * peek8(addr)), 44);
			}
		}
	}
}

void putword(unsigned w)
{
	unsigned n, m;

	if (w == 0xFF) 
	{
		fprintf(outfile, "_   ");
		return;
	}
	for (n = vocab; peek8(n) != 0xFF; n += 5)
	{
		if (peek8(n+4) == w)
		{
			for (m = 0; m < 4; m++)
				snap[m] = peek8(n + m);
			snap[m] = charset ? 0 : 1;
			prstring(0x3FE5, 0);
			return;
		}
	}
	fprintf(outfile, "?%02x ", w);
}

void dump_vocab()
{
	unsigned n, m;

	for (n = vocab; peek8(n) != 0xFF; n += 5)
	{
		for (m = 0; m < 4; m++)
			snap[m] = peek8(n + m);
		snap[m] = charset ? 0 : 1;
		fprintf(outfile, "%04x: Word %3d: ", n, peek8(n+4));
		prstring(0x3FE5, 0);
		fprintf(outfile, "\n");	
	}
}

void show_obj(unsigned obj, int sp)
{
	fprintf(outfile, "%-*.*s;", 13 - sp, 13 - sp, " ");
	prstring(peek16(obj_base + 2 * obj), 44);
}

void show_msg(unsigned obj)
{
	fprintf(outfile, "             ;");
	prstring(peek16(msgs_base + 2 * obj), 44);
}

void show_loc(unsigned obj)
{
	fprintf(outfile, "             ;");
	prstring(peek16(room_base + 2 * obj), 44);
}

void condtable(unsigned addr)
{
	unsigned cond, stop;

	while (peek8(addr) != 0 && peek8(addr + 1) != 0)
	{
		fprintf(outfile, "%04x: ", addr);
		putword(peek8(addr));
		fprintf(outfile, " ");
		putword(peek8(addr+1));
		fprintf(outfile, "  Conditions:\n");
		cond = peek16(addr + 2);
		while (peek8(cond) != 0xFF)
		{
			fprintf(outfile, "%04x:%15s", cond, "");
			switch (peek8(cond))
			{
				case 0:  fprintf(outfile, "AT      %3d",
						 peek8(cond + 1));
					 if (verbose) show_loc(peek8(cond + 1)); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;	 
				case 1:  fprintf(outfile, "PRESENT %3d",
						 peek8(cond + 1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;	 
				case 2:  fprintf(outfile, "CHANCE  %3d\n",
						 peek8(cond + 1));
					 cond += 2;
					 break;	 
				case 3:  fprintf(outfile, "ABSENT  %3d",
						 peek8(cond + 1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;	
				case 4:  fprintf(outfile, "NOTWORN %3d",
						 peek8(cond + 1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;	
				case 5:  fprintf(outfile, "NOTZERO %3d\n",
						 peek8(cond + 1));
					 cond += 2;
					 break;	 
				case 6:  fprintf(outfile, "EQ      %3d  %3d\n",
						 peek8(cond + 1), peek8(cond + 2));
					 cond += 3;
					 break;	 
				case 7:  fprintf(outfile, "ZERO    %3d\n",
						 peek8(cond + 1));
					 cond += 2;
					 break;	 
				case 8:  fprintf(outfile, "CARRIED %3d",
						 peek8(cond + 1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;	
				default: fprintf(outfile, "?%02x\n", peek8(cond));
					 	++cond;
					 break;	 
			}
		}
		stop = 0;
		cond = peek16(addr + 4);
		fprintf(outfile, "%17sActions:\n", "");
		while (!stop)
		{
			fprintf(outfile, "%04x:%15s", cond, "");
			switch (peek8(cond))
			{
				case 0:  fprintf(outfile, "INVEN\n");
					 ++cond;
					 break;
				case 1:  fprintf(outfile, "REMOVE  %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 2:  fprintf(outfile, "GET     %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 3:  fprintf(outfile, "DROP    %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 4:  fprintf(outfile, "WEAR    %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 5:  fprintf(outfile, "MESSAGE %3d",
							 peek8(cond+1));
					 if (verbose) show_msg(peek8(cond + 1)); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 6:  fprintf(outfile, "DESC\n");
					 stop = 1; 
					 break;
				case 7:  fprintf(outfile, "DONE\n");
					 stop = 1; 
					 break;
				case 8:  fprintf(outfile, "GOTO    %3d",
							 peek8(cond+1));
					 if (verbose) show_loc(peek8(cond + 1)); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 9:  fprintf(outfile, "SET     %3d\n",
						peek8(cond + 1));
					 cond += 2;
					 break;
				case 10: fprintf(outfile, "CLEAR   %3d\n",
						peek8(cond + 1));
					 cond += 2;
					 break;
				case 11: fprintf(outfile, "SWAP    %3d  %3d",
						 peek8(cond+1), 1+peek8(cond+1));
					 if (verbose) 
					 {
						 show_obj(peek8(cond + 1),5); 
						 fprintf(outfile, 
							"\n%-31.31s", "");
						 show_obj(1+peek8(cond + 1),0);
					 } 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 12: fprintf(outfile, "EXIT\n");
					 stop = 1; 
					 break;
				case 13: fprintf(outfile, "OKAY\n");
					 stop = 1; 
					 break;
				case 14: fprintf(outfile, "QUIT\n");
					 stop = 1; 
					 break;
				case 15: fprintf(outfile, "LET     %3d  %3d\n",
						peek8(cond + 1), peek8(cond + 2));
					 cond += 3;
					 break;
				case 16: fprintf(outfile, "CREATE  %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 17: fprintf(outfile, "DESTROY %3d",
							 peek8(cond+1));
					 if (verbose) show_obj(peek8(cond + 1),0); 
					 fprintf(outfile, "\n");
					 cond += 2;
					 break;
				case 18: fprintf(outfile, "ADDSCORE %03d  %03d",
						peek8(cond+1), peek8(cond+2));
					 cond += 3;
				     	 break;

				case 19: fprintf(outfile, "SCORE\n");
					 cond++;
				     	 break;
				case 21: fprintf(outfile, "NEWTEXT\n");
					 stop = 1;	
				     	 break;
				case 20: 
				case 22: 
				case 23:
				case 24:
					 fprintf(outfile, "DONE\n");
					 stop = 1;
				     	 break;
				default: fprintf(outfile, "?%02x\n", peek8(cond));
				 	++cond;
				break;	 
			}
		}

		addr += 6;
	}
	fprintf(outfile, "\n");
}

void dump_cond()
{
	fprintf(outfile, "%04x: Response [Event] table\n", response);
	condtable(response);
	fprintf(outfile, "%04x: Process [Status] table\n", process);
	condtable(process);
}


void dump_conn()
{
	unsigned n, addr, dest;

	for (n = 0; n < room_count; n++)
	{
		addr = peek16(conn_base + 2 * n);
		fprintf(outfile, "\n%04x: Connections from %3d:", addr, n);
		if (verbose) 
		{
			fprintf(outfile, " ;");
			prstring(peek16(room_base + 2 * n), 28);
		}
		fputc('\n', outfile);
		while (peek8(addr) != 0xFF)
		{
			fprintf(outfile, "%6s", "");
			putword(peek8(addr));
			dest = peek8(addr + 1);
			fprintf(outfile, " to %3d", dest);
			if (verbose) 
			{
				fprintf(outfile, "%11s;", "");
				prstring(peek16(room_base + 2 * dest), 28);
			}
			fputc('\n', outfile);
			addr += 2;
		}
	}
	fprintf(outfile, "\n");
}




void unplad()
{
	if (analyse()) return;

	fprintf(outfile, "      ; Generated by UnPLAD 0.0.1\n");

	if (carry) fprintf(outfile, "%04x: Player can carry %d objects.\n\n",
			carry, peek8(carry));

	if (!skip_cond) dump_cond();
	if (!skip_obj) dump_objects();
	if (!skip_loc) dump_rooms();
	if (!skip_msg) dump_msgs();
	if (!skip_conn) dump_conn();
	if (!skip_voc) dump_vocab();
	if (!skip_obj) dump_objpos();
	if (!skip_sys) dump_sysmsgs();
}

void syntax(const char *arg)
{
	fprintf(stderr, "Syntax: %s {options} filename\n\n"
			"Options:\n"
			"    -o filename: Output to specified file\n"
			"    -sc: Skip Conditions\n"
			"    -sl: Skip Location descriptions\n"
			"    -sm: Skip Messages\n"
			"    -sn: Skip coNNections\n"
			"    -so: Skip Objects\n"
			"    -ss: Skip System messages\n"
			"    -sv: Skip Vocabulary\n"
			"    -v:  Verbose output\n", arg);
}


int main(int argc, char **argv)
{
	int n;
	FILE *fp;
	char *fname;

	outfile = stdout;
	for (n = 1; n < argc; n++)
	{
#ifdef __MSDOS__
		if (argv[n][0] == '-' || argv[n][0] == '/')
#else
		if (argv[n][0] == '-')
#endif
		{
			int badarg = 1;
			switch(tolower(argv[n][1]))
			{
				case 'h':
				case '?':
#ifdef __PACIFIC__
					  syntax("UNPLAD");
#else
					  syntax(argv[0]);
#endif
					  return 0;

				case 'o': if (argv[n][2]) fname = &argv[n][2];
				          else if (n + 1 < argc)
					  {
						n++;
						fname = argv[n];
					  }
					  else
					  {
						fprintf(stderr, "-o option should be followed by a filename.\n");
						return 1;
					  }
					  outfile = fopen(fname, "w");
					  if (!outfile)
					  {
						perror(fname);
						return 1;
					  }
					  badarg = 0;
					  break;

				case 'v': verbose = 1; badarg = 0; break;
				case 's': switch (tolower(argv[n][2]))
					  {
						case 'c': skip_cond= 1;
						  	  badarg   = 0;
							  break;	  
						case 'l': skip_loc = 1;
						  	  badarg   = 0;
							  break;	  
						case 'm': skip_msg = 1;
						  	  badarg   = 0;
							  break;	  
						case 'n': skip_conn= 1;
						  	  badarg   = 0;
							  break;	  
						case 'o': skip_obj = 1;
						  	  badarg   = 0;
							  break;	  
						case 's': skip_sys = 1;
						  	  badarg   = 0;
							  break;	  
						case 'v': skip_voc = 1;
						  	  badarg   = 0;
							  break;	  
					  }
					  break;
			}
			if (badarg)
			{
				fprintf(stderr, "Unknown option: %s\n", argv[n]);
				return 1;
			}
		}
		else if (filename == NULL)
		{
			filename = argv[n];
		}
		else
		{
			syntax(argv[0]);
			return 0;
		}
	}
	if (filename == NULL)
	{
		syntax(argv[0]);
		return 0;
	}
	fp = fopen(filename, "rb");
	if (!fp)
	{
		perror(filename);
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	if (ftell(fp) >= 49179)	/* Spectrum SNA */
	{
		fseek(fp, 0, SEEK_SET);
		fread(&snap[0], 1, 49179, fp);
		if (platform == 0) platform = 2;
	}
	else
	{
		fseek(fp, 0, SEEK_SET);
		fread(&snap[36], 1, 0xbff7, fp);
		if (platform == 0) platform = 1;
	}
	fclose(fp);
	unplad();
	return 0;
}
