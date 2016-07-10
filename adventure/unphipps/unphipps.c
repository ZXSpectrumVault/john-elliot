/*
    UnPhipps: Dumps the database in games based on the 'ZX81 Pocket Book' 
	     engine
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
#include <math.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

void show_room(unsigned room, int indent);
void show_obj(unsigned obj, int indent);
void show_msg(unsigned msg, int indent);
void show_word(int word);


unsigned char snap[49179];
const char *filename = NULL;
FILE *outfile;
char txtbuf[512];

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

/* Big-endian peek (ZX81 stores line numbers in big-endian format) */
unsigned peek16be(unsigned offset)
{
	return peek8(offset) * 256 + peek8(offset+1);
}


float peekf(unsigned addr)
{
	double dm, val;
	signed char exp;
	long mantissa;
	int sign;

	exp = (signed char)(peek8(addr));
	sign = peek8(addr+1) & 0x80;
	mantissa = ((unsigned long)((peek8(addr + 1) & 0x7F)) << 24);
	mantissa |= (unsigned long)((peek8(addr + 2))) << 16;
	mantissa |= (unsigned long)((peek8(addr + 3))) << 8;
	mantissa |= (unsigned long)((peek8(addr + 4)));

	dm = (mantissa | 0x80000000);
	dm /= 65536.0;
	dm /= 65536.0;


	if (exp == 0 && mantissa == 0 && sign == 0) return 0;

	val = dm;
	if (exp & 0x80) 
	{
		exp &= 0x7F;
		val = dm * pow(2.0, exp);
		return val;
	}
	else
	{
		fprintf(stderr, "Floating-point parser can't handle this\n");
//		exit(1);
		return -1;
	}	
}


/* Operation flags */
unsigned verbose;
unsigned skip_cond, skip_msg, skip_loc, skip_obj, skip_voc, skip_conn, skip_sys,
	 skip_word;

/* Database details */
unsigned msgs_base, msgs_count;
unsigned room_base, room_count;
unsigned conn_format;
unsigned obj_count;
unsigned voc_format;
unsigned word_count;
unsigned voc_base;
unsigned vars;
unsigned e_line;
unsigned carry;

const char *zx81font[] = 
{
	" ", "a", "b", "c",  "d", "e", "f", "g",
	"h", "i", "j", "\"", "£", "$", ":", "?",
	"(", ")", ">", "<",  "=", "+", "-", "*",
	"/", ";", ",", ".",  "0", "1", "2", "3",
	"4", "5", "6", "7",  "8", "9", "A", "B",
	"C", "D", "E", "F",  "G", "H", "I", "J",
	"K", "L", "M", "N",  "O", "P", "Q", "R",
	"S", "T", "U", "V",  "W", "X", "Y", "Z",
	" RND", " INKEY$", " PI"
};

const char *keyword[] = 
{
	"\"\"", "AT", "TAB", "?", "CODE", "VAL", "LEN", "SIN",
	"COS", "TAN", "ASN", "ACS", "ATN", "LN", "EXP", "INT",
	"SQR", "SGN", "ABS", "PEEK", "USR", "STR$", "CHR$", "NOT",
	"**", "OR", "AND", "<=", ">=", "<>", "THEN", "TO",
	"STEP", "LPRINT", "LLIST", "STOP", "SLOW", "FAST", "NEW", "SCROLL",
	"CONT", "DIM", "REM", "FOR", "GOTO", "GOSUB", "INPUT", "LOAD",
	"LIST", "LET", "PAUSE", "NEXT", "POKE", "PRINT", "PLOT", "RUN",
	"SAVE", "RAND", "IF", "CLS", "UNPLOT", "CLEAR", "RETURN", "COPY"
};


int inverse = 0;

void showchar(unsigned char c, int indent)
{
	if (c < 128 && inverse)
	{
		fputc(']', outfile);
		inverse = 0;
	}
	else if (c >= 128 && c < 192 && !inverse)
	{
		fputc('[', outfile);
		inverse = 1;
	}
	if ((c & 0x7F) < 64 || (c >= 64 && c <= 66))
	{
		fprintf(outfile, "%s", zx81font[c & 0x7F]);		
		return;
	}
	if (c >= 192)
	{
		fputc(' ', outfile);
		fprintf(outfile, "%s", keyword[c & 0x3F]);		
		fputc(' ', outfile);
		return;
	}
	if (c == 118)
	{
		fprintf(outfile, "\n%-*.*s", indent, indent, " ");
		return;
	}
	if (c < 192)
	{
		fputc('_', outfile);
		return;
	}
	fprintf(outfile, "<%02x>", c);
}


	
void listline(unsigned addr, int indent)
{
	unsigned len;
	int in_q = 0;
	int last_cr = 0;
	int in_at = 0;

	printf("%04d", peek16be(addr));
	len = peek16(addr+2);
	addr += 4;

	--len;	/* Drop the terminating CR */
	inverse = 0;
	while (len)
	{
		unsigned char ch = peek8(addr);

		switch (ch)
		{
			case 126:	/* Number */
				addr += 6;
				len -= 6;
				break;
			default: showchar(ch, indent);
				if (ch == 11) 
				{
					in_q = !in_q;		
					in_at = 0;
				}
				if (ch == 193)
				{
/* If we encounter an AT, don't break at commas until the next quoted string */
					in_at = 1;
				}
/* Break PRINTed strings at a comma, if it's outside a quoted string */
				if ((ch == 25 || ch == 26) && 
					 !in_at && !in_q && !last_cr)
				{
					fprintf(outfile, "\n%-*.*s",
						indent, indent, " ");
					if (indent > 6) fputc(';', outfile);
					last_cr = 1;
				}
				else	last_cr = 0;
				++addr;
				--len;
				break;
		}
	}
}



/* Given that 'addr' is the address of a ZX81 variable, return the address of
 * the next variable in memory. */
unsigned next_var(unsigned addr)
{
	unsigned type = peek8(addr) & 0xE0;

	switch (type)
	{
			   /* String */
		case 0x40: return addr + 3 + peek16(addr + 1);
			   /* Single letter */
		case 0x60: return addr + 6;	
			   /* Numeric array */
		case 0x80: return 3 + peek16(addr + 1) + addr; 
			   /* Multiple letters */
		case 0xA0: do			
			   {
				addr++;
			   }
			   while ((peek8(addr) & 0xC0) == 0);
			   return addr + 6;
			   /* String array */
		case 0xC0: return 3 + peek16(addr + 1) + addr; 
			   /* FOR/NEXT control variable */
		case 0xE0: return addr + 18;
		default:   fprintf(stderr, "Unsupported variable type %02x\n",
				type);
			   exit(1);
	}
	return 0;
}




float get_float(char var)
{
	unsigned addr = vars;
	unsigned zvar = (var - 'A') + 38;	/* -> ZX81 character set */
	while (addr < e_line)
	{
		if (peek8(addr) == (0x60 | (zvar & 0x1F)))
		{
			return peekf(addr + 1);
		}
		else
		{
			addr = next_var(addr);
		}
	}
	fprintf(stderr, "Variable not found: %c\n", var);
	exit(1);
	return 0;
}

/* Get a float from a 1-dimensional array */
float get_float1(char var, int index, unsigned *paddr)
{
	unsigned ndim;
	unsigned addr = vars;
	unsigned zvar = (var - 'A') + 38;	/* -> ZX81 character set */
	unsigned d0;

	while (addr < e_line)
	{
		if (peek8(addr) == (0x80 | (zvar & 0x1F)))
		{
			addr += 3;	// -> Number of dimensions
			ndim = peek8(addr);
			if (ndim != 1)
			{
				fprintf(stderr, "Unexpected array layout: %c()\n", var);
				exit(1);	
			}
			d0 = peek16(addr + 1);		
			if (index < 1 || index > d0)
			{
				fprintf(stderr, "Subscript %d out of range in %c()\n", index, var);
				exit(1);
			}
			addr += 3 + (index - 1) * 5;
			*paddr = addr;
			return peekf(addr);
		}
		else
		{
			addr = next_var(addr);
		}
	}
	fprintf(stderr, "Variable not found: %c$()\n", var);
	exit(1);
	return 0;
}




void decode_string(unsigned addr, unsigned len, char *result)
{
	unsigned n;
	int inverse = 0;

	for (n = 0; n < len; n++)
	{
		if (0 == inverse && (peek8(addr) & 0x80))
		{
			*result++ = '[';
			inverse = 1;
		}
		if (0 != inverse && (!(peek8(addr) & 0x80)))
		{
			*result++ = ']';
			inverse = 0;
		}
		*result++ = zx81font[peek8(addr) & 0x3F][0];
		++addr;
	}
	*result++ = 0;
}



/* Find an entry in a 2-d string array */
void get_string2(char var, int index, char *result, unsigned *paddr)
{
	unsigned ndim;
	unsigned addr = vars;
	unsigned zvar = (var - 'A') + 38;	/* -> ZX81 character set */
	unsigned d0, d1;

	while (addr < e_line)
	{
		if (peek8(addr) == (0xC0 | (zvar & 0x1F)))
		{
			addr += 3;	// -> Number of dimensions
			ndim = peek8(addr);
			if (ndim != 2)
			{
				fprintf(stderr, "Unexpected array layout: %c$()\n", var);
				exit(1);	
			}
			d0 = peek16(addr + 1);		
			if (index < 1 || index > d0)
			{
				fprintf(stderr, "Subscript %d out of range in %c$()\n", index, var);
				exit(1);
			}
			d1 = peek16(addr + 3);		
			addr += 5 + (index - 1) * d1;
			decode_string(addr, d1, result);
			*paddr = addr;
			return;
		}
		else
		{
			addr = next_var(addr);
		}
	}
	fprintf(stderr, "Variable not found: %c$()\n", var);
	exit(1);
}


/* Find an entry in a 3-d string array */
void get_string3(char var, int index, int idx2, char *result, unsigned *paddr)
{
	unsigned ndim;
	unsigned addr = vars;
	unsigned zvar = (var - 'A') + 38;	/* -> ZX81 character set */
	unsigned d0, d1, d2;

	while (addr < e_line)
	{
		if (peek8(addr) == (0xC0 | (zvar & 0x1F)))
		{
			addr += 3;	// -> Number of dimensions
			ndim = peek8(addr);
			if (ndim != 3)
			{
				fprintf(stderr, "Unexpected array layout: %c$()\n", var);
				exit(1);	
			}
			d0 = peek16(addr + 1);		
			if (index < 1 || index > d0)
			{
				fprintf(stderr, "Subscript %d out of range in %c$()\n", index, var);
				exit(1);
			}
			d1 = peek16(addr + 3);		
			d2 = peek16(addr + 5);		
			addr += 7 + (index - 1) * (d2 * d1);
			addr += d2 * (idx2 - 1);
			decode_string(addr, d2, result);
			*paddr = addr;
			return;
		}
		else
		{
			addr = next_var(addr);
		}
	}
	fprintf(stderr, "Variable not found: %c$()\n", var);
	exit(1);
}




int get_array_dimensions(char var, unsigned *d0)
{
	unsigned addr = vars;
	unsigned zvar = (var - 'A') + 38;	/* -> ZX81 character set */

	while (addr < e_line)
	{
		if (peek8(addr) == (0x40 | (zvar & 0x1F)))
		{
			return 0;	/* Not an array */
		}
		if (peek8(addr) == (0xC0 | (zvar & 0x1F)))
		{
			addr += 3;		/* -> Number of dimensions */
			*d0 = peek16(addr + 1);  /* First dimension */
			return peek8(addr);
		}
		else
		{
			addr = next_var(addr);
		}
	}
	return -1;
}



/* Find a BASIC line */
unsigned find_basic(unsigned line)
{
	unsigned addr = 16509;	/* ZX81 program is at a fixed address */
	
	while (peek16be(addr) < 10000)
	{
		if (peek16be(addr) == line) return addr;
		addr += peek16(addr + 2) + 4;
	}
	return 0;
}



void analyse()
{
	unsigned addr;

	vars   = peek16(0x4010);	/* VARS */
	e_line = peek16(0x4014);	/* E LINE */

	addr = find_basic(4310);	/* Find line 4310 */
	if (!addr)
	{
		fprintf(stderr, "Cannot find BASIC line 4310\n");
		exit(1);
	}
	msgs_base  = (unsigned)peekf(addr + 10);	
	room_base  = 8000;
	conn_format = get_array_dimensions('M', &room_count);
/* If messages come before rooms, assume they run up to the room base. */
	if (msgs_base < room_base)
	{
		msgs_count = (room_base - 1 - msgs_base) / 10;
	}
	else /* If they come after, assume they run up to line 9000. */
	{
		msgs_count = (9000      - 1 - msgs_base) / 10;
	}
	obj_count = (unsigned)get_float('O');
	voc_format = get_array_dimensions('V', &word_count);
/* Knight's Quest uses machine code for vocabulary lookup; accordingly, the 
 * format and location of the vocab table are different */
	if (voc_format < 2)
	{
		unsigned d_file = peek16(0x400c);
		voc_base = d_file - peek16(0x408D);
		
		word_count = peek16(0x408d) / 5;
	}
	
	addr = find_basic(4100);	/* Find line 4100 */
	if (addr)
	{
		carry = (unsigned)peekf(addr + 18);
	}
}



void dump_objects(void)
{
	unsigned n, addr;

	for (n = 1; n <= obj_count; n++)
	{
		get_string2('O', n, txtbuf, &addr);
		fprintf(outfile, "\n%04x: Object %d\n      %s\n", addr, n,
			txtbuf);
	}
	fprintf(outfile, "\n");
}

void dump_objpos(void)
{
	unsigned n, addr, loc;

	for (n = 1; n <= obj_count; n++)
	{
		loc = (unsigned)get_float1('Q', n, &addr);	
		fprintf(outfile, "\n%04x: Object %3d is initially ", addr, n);
		switch (loc)
		{
			case 0x00: fprintf(outfile, "not created."); 
				   break;
			default: fprintf(outfile, "in room %3d.", loc);
				 break;
		}
		if (verbose)
		{
			fprintf(outfile, "\n%-44.44s;", "");
			show_obj(n, 44);
			if (loc != 0)
			{
				fprintf(outfile, "\n%-44.44s;", "");
				show_room(loc, 44);
			}
		}
	}
}


void dump_objword(void)
{
	unsigned n, addr;
	unsigned d0;
	char w1[3];
	char w2[3];

	if (get_array_dimensions('T', &d0) <= 0) return;

	for (n = 1; n <= d0; n++)
	{
		get_string2('T', n, txtbuf, &addr);
		sprintf(w1, "%-2.2s", txtbuf+2);
		sprintf(w2, "%-2.2s", txtbuf);
		fprintf(outfile, "\n%04x: Object %3d name ", addr, atoi(w1));
		show_word(atoi(w2));
		if (verbose)
		{
			fprintf(outfile, " ;");
			show_obj(atoi(w1), 44);
		}
	}
}



void dump_vocab()
{
	unsigned n, addr;
	char wordbuf[20];
	char wordid[3];

	if (voc_format < 2)
	{
		addr = voc_base;
		n=  0;
		while (addr < peek16(0x400c))
		{
			decode_string(addr, 4, wordbuf);
			fprintf(outfile, "%04x: Word %3d: %-4.4s\n", 
					addr, peek8(addr + 4), wordbuf);
			addr += 5;
			n++;
			if (n == peek8(0x4093)) { n = 0; addr += 6; }
		}
		return;
	}
	for (n = 1; n <= word_count; n++)
	{
		get_string2('V', n, wordbuf, &addr);
		strncpy(wordid, wordbuf, 2);
		wordid[2] = 0;

		fprintf(outfile, "%04x: Word %3d: %-4.4s\n", 
				addr, atoi(wordid), wordbuf + 2);
	}
}

void show_obj(unsigned obj, int sp)
{
	unsigned addr;
	char buf[120];

	get_string2('O', obj, buf, &addr);
	fprintf(outfile, "%s", buf);
}


void condtable(char type)
{
	unsigned count;
	int n, arg;
	char w1[3];
	char w2[3];
	char *p;
	unsigned addr;

	get_array_dimensions(type, &count);

	for (n = 1; n <= count; n++)
	{
		get_string2(type, n, txtbuf, &addr);
		fprintf(outfile, "%04x: ", addr);
		if (type == 'A')
		{
			sprintf(w1, "%-2.2s", txtbuf);
			sprintf(w2, "%-2.2s", txtbuf+2);
			if (atoi(w1)) show_word(atoi(w1)); 
			else fprintf(outfile, "_   ");
			fprintf(outfile, " ");
			if (atoi(w2)) show_word(atoi(w2));
			else fprintf(outfile, "_   ");
			p = txtbuf + 4;
		}
		else 
		{
			fprintf(outfile, "_    _   ");
			p = txtbuf;
		}
		fprintf(outfile, "  Conditions:\n");
		while (*p && *p != '.' && *p != ' ')
		{
			fprintf(outfile, "%20s", "");
			sprintf(w1, "%-2.2s", p+1);
			arg = atoi(w1);
			switch (*p)
			{
				case 'A':
					 fprintf(outfile, "AT      %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_room(arg, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3; 
					 break;
				case 'B':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "PRESENT %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(arg, 44);
					 }
					 fprintf(outfile, "\n");
					 p += 3; 
					 break;
				case 'C':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "ABSENT  %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(atoi(w1), 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3; 
					 break;
				case 'D':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "CARRIED %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(atoi(w1), 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3; 
					 break;
				case 'E':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "NOTZERO %3d\n", arg);
					 p += 3; 
					 break;
				case 'F':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "ZERO    %3d\n", arg);
					 p += 3; 
					 break;
				case 'G':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "TIMEOUT %3d\n", arg);
					 p += 3; 
					 break;
				case 'H':sprintf(w1, "%-2.2s", p+1);
					 fprintf(outfile, "CHANCE  %3d\n", arg);
					 p += 3; 
					 break;

				default: fprintf(outfile, "??%c\n", *p);
					 ++p;
					 break;
			}
		}
		fprintf(outfile, "%17sActions:\n", "");
		if (*p == '.') ++p;
		while (*p && *p != '.' && *p != ' ')
		{
			fprintf(outfile, "%20s", "");
			sprintf(w1, "%-2.2s", p+1);
			arg = atoi(w1);
			switch (*p)
			{
				case 'A':fprintf(outfile, "INVEN\n");
					 p++;
					 break;
				case 'B':fprintf(outfile, "GET     %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(atoi(w1), 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;
				case 'C':fprintf(outfile, "DROP    %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(atoi(w1), 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;
				case 'D': fprintf(outfile, "MESSAGE %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_msg(atoi(w1), 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;
				case 'E':fprintf(outfile, "SET     %3d\n", arg);
					 p += 3;
					 break;
				case 'F':fprintf(outfile, "CLEAR   %3d\n", arg);
					 p += 3;
					 break;
				case 'G':sprintf(w2, "%-2.2s", p+3);
					 fprintf(outfile, "LET     %3d  %3d\n",
					 	arg, atoi(w2));
					 p += 5;
					 break;
				case 'H':fprintf(outfile, "SWAP    %3d  %3d", 
						arg, arg + 1);
					 if (verbose) 
					 {
						fprintf(outfile, "        ;");
						show_obj(arg, 44); 
						fprintf(outfile, "\n%44s;", "");
						show_obj(arg + 1, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;

				case 'I':fprintf(outfile, "CREATE  %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(arg, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;
				case 'J':fprintf(outfile, "DESTROY %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_obj(arg, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;
				case 'K':
					 fprintf(outfile, "GOTO    %3d", arg);
					 if (verbose) 
					 {
						fprintf(outfile, "%13s;", "");
						show_room(arg, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3; 
					 break;

				case 'L':fprintf(outfile, "OKAY\n");
					 p++;
					 break;
				case 'M':fprintf(outfile, "DONE\n");
					 p++;
					 break;
				case 'N':fprintf(outfile, "NEWTEXT\n");
					 p++;
					 break;
				case 'O':fprintf(outfile, "DESC\n");
					 p++;
					 break;
				case 'P':fprintf(outfile, "QUIT\n");
					 p++;
					 break;
				case 'Q':fprintf(outfile, "EXIT\n");
					 p++;
					 break;
				case 'R':fprintf(outfile, "SWAP    %3d  %3d", 
						arg, arg - 1);
					 if (verbose) 
					 {
						fprintf(outfile, "        ;");
						show_obj(arg, 44); 
						fprintf(outfile, "\n%44s;", "");
						show_obj(arg - 1, 44); 
					 }
					 fprintf(outfile, "\n");
					 p += 3;
					 break;

				case 'S':switch (arg)
					 {
						case 70: fprintf(outfile, "AUTOG\n"); break;
						case 71: fprintf(outfile, "AUTOD\n"); break;
						case 72: fprintf(outfile, "SCORE\n"); break;
						default: fprintf(outfile, "?S%02d\n", arg); break;
					 }
					 p += 3;
					 break;
				case 'T':fprintf(outfile, "DEC     %3d\n", arg);
					 p += 3;
					 break;
				default: fprintf(outfile, "??%c\n", *p);
				 	 ++p;
					 break;	 
			}
		}
	}
	fprintf(outfile, "\n");
}


void dump_cond()
{
	unsigned addr;

	get_string2('A', 1, txtbuf, &addr);
	fprintf(outfile, "%04x: Response [Event] table\n", addr);
	condtable('A');
	get_string2('C', 1, txtbuf, &addr);
	fprintf(outfile, "%04x: Process [Status] table\n", addr);
	condtable('C');
}



void show_word(int word)
{
	unsigned n, addr;
	char wordbuf[20];
	char wordid[3];

	if (voc_format < 2)
	{
		addr = voc_base;
		n = 0;
		while (addr < peek16(0x400c))
		{
			if (peek8(addr + 4) == word)
			{
				decode_string(addr, 4, wordbuf);
				fprintf(outfile, "%-4.4s", wordbuf);
				return;
			} 
			addr += 5;
			n++;
			if (n == peek8(0x4093)) { n = 0; addr += 6; }
		}
		fprintf(outfile, "W%03d", word);		
		return;
	}

	for (n = 1; n <= word_count; n++)
	{
		get_string2('V', n, wordbuf, &addr);
		strncpy(wordid, wordbuf, 2);
		wordid[2] = 0;
		if (atoi(wordid) == word)
		{
			fprintf(outfile, "%-4.4s", wordbuf + 2);
			return;
		}
	}
	fprintf(outfile, "W%03d", word);
}






/* We grok two connection table formats: the one defined in 'City of Alzan',
 * which is a 2D array, and the one used in 'Greedy Gulch' and 'Pharaoh's 
 * Tomb', which is a 3D array containing N/S/E/W entries for each room.
 */
void dump_conn()
{
	unsigned n, addr, c, max;
	char val[3];
	unsigned verb, dest;

	for (n = 1; n <= room_count; n++)
	{
		if (conn_format == 2)
		{
			get_string2('M', n, txtbuf, &addr);
			fprintf(outfile, "\n%04x: Connections from %3d:", addr, n);
			if (verbose) 
			{
				fprintf(outfile, " ;");
				show_room(n, 28);
			}
			fputc('\n', outfile);
			max = strlen(txtbuf);
			for (c = 0; c < max; c += 4)
			{
				if (txtbuf[c] == '0' && txtbuf[c+1] == '0')
					break;
				if (txtbuf[c] == ' ') 
					break;
				memcpy(val, &txtbuf[c], 2);
				val[2] = 0;
				verb = atoi(val);
				memcpy(val, &txtbuf[c+2], 2);
				val[2] = 0;
				dest = atoi(val);
				fprintf(outfile, "%6s", "");
				show_word(verb);
				fprintf(outfile, " to %3d", dest);
				if (verbose) 
				{
					fprintf(outfile, "%11s;", "");
					show_room(dest, 28);
				}
				fputc('\n', outfile);
			}
		}
		else
		{
			get_string3('M', n, 1, txtbuf, &addr);
			fprintf(outfile, "\n%04x: Connections from %3d:", addr, n);
			if (verbose) 
			{
				fprintf(outfile, " ;");
				show_room(n, 28);
			}
			fputc('\n', outfile);
			for (c = 1; c <= 4; c++)
			{
				get_string3('M', n, c, txtbuf, &addr);
				dest = atoi(txtbuf);
				if (!dest) continue;
				fprintf(outfile, "%6s", "");
				show_word(c);
				fprintf(outfile, " to %3d", dest);
				if (verbose) 
				{
					fprintf(outfile, "%11s;", "");
					show_room(dest, 28);
				}
				fputc('\n', outfile);
			}
		}
	}
	fprintf(outfile, "\n");
}


void show_room(unsigned room, int indent)
{
	unsigned n, addr, first = 1;

	for (n = 0; n < 9; n++)
	{
		addr = find_basic(room * 10 + room_base + n);
		if (addr)
		{
/* Don't list the RETURN at the end of each location description subroutine */
			if (peek16(addr + 2) == 2 && peek8(addr+4) == 254)
			{
				continue;
			}
			if (!first)
			{
				fprintf(outfile, "\n%-*.*s",
						indent, indent, " ");
				if (indent > 6) fputc(';', outfile);
			}
			first = 0;
			listline(addr, indent);
		}
	}
}


void dump_rooms()
{
	unsigned n, addr;

	for (n = 1; n <= room_count; n++)
	{
		addr = find_basic(n * 10 + 8000);
		if (!addr) continue;
		fprintf(outfile, "\n%04x: Location %d\n      ", addr, n);
		show_room(n, 6);
	}
	fprintf(outfile, "\n");
}

void show_msg(unsigned msg, int indent)
{
	unsigned n, addr;
	int first = 1;

	for (n = 0; n < 9; n++)
	{
		addr = find_basic(msg * 10 + msgs_base + n);
		if (addr)
		{
/* Don't list the RETURN at the end of each message subroutine */
			if (peek16(addr + 2) == 2 && peek8(addr+4) == 254)
			{
				continue;
			}
			if (!first)
			{
				fprintf(outfile, "\n%-*.*s",
						indent, indent, " ");
				if (indent > 6) fputc(';', outfile);
			}
			first = 0;
			listline(addr, indent);
		}
	}
}



void dump_msgs()
{
	unsigned n, addr;

	for (n = 1; n <= msgs_count; n++)
	{
		addr = find_basic(n * 10 + msgs_base);
		if (!addr) continue;
		fprintf(outfile, "\n%04x: Message %d\n      ", addr, n);
		show_msg(n, 6);
	}
	fprintf(outfile, "\n");
}


void dump_sysmsgs(void)
{
	unsigned addr = 16509;	/* ZX81 program is at a fixed address */
	unsigned line;
	
	while (peek16be(addr) < 10000)
	{
		line = peek16be(addr);
		if (line >= room_base || line >= msgs_base) break;
		if (peek8(addr + 4) == 0xF5)
		{
			fprintf(outfile, "\n%04x: System message\n      ", addr);
			listline(addr, 6);
		}
		addr += peek16(addr + 2) + 4;
	}
	fprintf(outfile, "\n");
}






void unphipps()
{
	analyse();

	fprintf(outfile, "      ; Generated by UnPhipps 0.0.3\n");

	if (carry)
	{
		unsigned addr = find_basic(4100) + 18;
		fprintf(outfile, "%04x: Player can carry %d objects.\n\n",
			addr, carry);	
	}

	if (!skip_cond) dump_cond();
	if (!skip_obj) dump_objects();
	if (!skip_loc) dump_rooms();
	if (!skip_msg) dump_msgs();
	if (!skip_conn) dump_conn();
	if (!skip_voc) dump_vocab();
	if (!skip_obj) dump_objpos();
	if (!skip_sys) dump_sysmsgs();
	if (!skip_word) dump_objword();
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
			"    -sw: Skip object Word associations\n"
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
					  syntax("UNPHIPPS");
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
						case 'w': skip_word= 1;
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
	fread(&snap[36], 1, 0xbff7, fp);
	fclose(fp);
	unphipps();
	return 0;
}
