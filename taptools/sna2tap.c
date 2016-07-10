/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014 John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/

#include "config.h"
#include "cnvshell.h"
#include "tapeio.h"

typedef unsigned char byte;
typedef unsigned short word;

/* Length of final-stage loader that must be injected into the SNA image 
 * somewhere */
#define PRELUDE_MAX 65	/* If not immediately below the stack, needs 3 bytes
			   for a LD SP */
#define PRELUDE_MIN 62	/* If immediately below the stack, these can be
			   skipped */

static int st_prelude_size = PRELUDE_MAX;

/* Options */
static int st_base = -1;
static TIO_FORMAT st_format = TIOF_TAP;
static char st_name[11] = "\020\007\021\000\023\001.sna";

/* The SNA, and the values of its registers */
static byte sna[49179];
static word i, r, hl1, de1, bc1, af1;
static word hl, de, bc, iy, ix;
static word af, sp, imm, iff, border;


char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (variable[0] == 'n' || variable[0] == 'N')
	{
		if (!value) return "No filename supplied";
		sprintf(st_name, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 'b' || variable[0] == 'B')
	{
		if (value && (isdigit(value[0]) || value[0] == '&'))
		{
			if ((value[0] == '0' && value[1] == 'x') ||
			    (value[0] == '0' && value[1] == 'X'))
			{
				if (!sscanf(value + 2, "%x", &st_base))
				{
					return "Invalid base address supplied";
				}
			}
			else if (value[0] == '&')
			{
				if (!sscanf(value + 1, "%x", &st_base))
				{
					return "Invalid base address supplied";
				}
			}
			else if (!sscanf(value, "%d", &st_base))
			{
				return "Invalid base address supplied";
			}
			return NULL;
		}
		return "No base address supplied";	
	}
	if (variable[0] == 't' || variable[0] == 'T')
	{
		st_format = TIOF_TAP;
		return NULL;
	}
	if (variable[0] == 'z' || variable[0] == 'Z')
	{
		st_format = TIOF_ZXT;
		return NULL;
	}
	if (variable[0] == 'f' || variable[0] == 'F')
	{
		if (variable[1] == 0) 
			return (char *)tio_parse_format(value, &st_format);
		else	return (char *)tio_parse_format(variable + 1, &st_format);
	}
	return "Unrecognised option";
}

static char helpbuf[2000];

char *cnv_progname = "sna2tap";

char *cnv_help(void)
{
	sprintf(helpbuf, "Syntax: %s { options } "
			"{ infile.SNA { outfile.TAP }}\n\n"
		"Options are:\n"
	/*	 1...5...10...15...20...25...30...35...40 */
		"--base: The address of an area of %d "
		"bytes which will be overwritten by the\n"
	        "        loader. If not specified, the ar"
		"ea just below the Z80 stack will be\n"
	        "        used.\n"
		"--name: The filename to use for the BASIC"
		" loader.\n"
		"--ftap:  Output in .TAP format (default)\n"
		"--fzxt:  Output in .ZXT format\n" 
		"--ftzx:  Output in .TZX format\n", 
		cnv_progname, st_prelude_size);

	return helpbuf;
}




word peek2(int a)
{
	word v = sna[a + 1];
	return (v << 8) | sna[a];
}

void poke2(byte *array, int a, word v)
{
	array[a] = (v & 0xFF);
	array[a+1] = (v >> 8);
}


void parse_sna(void)
{
	i   = sna[0]; i *= 0x101;
	hl1 = peek2(1);
	de1 = peek2(3);
	bc1 = peek2(5);
	af1 = peek2(7);
	hl  = peek2(9);
	de  = peek2(11);
	bc  = peek2(13);
	iy  = peek2(15);
	ix  = peek2(17);
	iff = (sna[19] & 1);
	r   = sna[20]; r *= 0x101;
	af  = peek2(21);
	sp  = peek2(23);
	imm = sna[25];
	border = sna[26]; border *= 0x101;
}


static byte launch[] =
{
/* Launch code, injected into memory. */
	0xE1, 0xD1, 0xC1, 0xF1,	/* +00 POP HL ! POP DE ! POP BC ! POP AF */
	0x08, 0xD9,		/* +04 EX AF, AF' ! EXX */
	0xFD, 0xE1, 0xDD, 0xE1, /* +06 POP IY ! POP IX */
	0xE1, 0xD1, 0xC1, 	/* +10 POP HL ! POP DE ! POP BC */
	0xF1, 0xD3, 0xFE,	/* +13 POP AF ! OUT (254),A */
	0xF1, 0xED, 0x47, 	/* +16 POP AF ! LD I,A */
	0xF1, 0xED, 0x4F,	/* +19 POP AF ! LD R,A */
	0xF1,			/* +22 POP AF */
	0x31, 0x00, 0x00,	/* +23 LD SP, nnnn */
	0xED, 0x00,		/* +26 IM 0/1/2 */
	0x00,			/* +28 EI or DI */
	0xED, 0x45		/* +29 RETN */	
				/* +31 */
};

/* The launch code must be followed by 34 bytes of stack:
 * 	DW 0, 0, 0		; +31 Workspace for tape loader
 * 	DW launch_code		; +37 
 * 	DW HL', DE', BC', AF'	; +39
 * 	DW IY, IX, HL, DE, BC	; +47
 * 	DW border, I, R, AF 	; +57 
 * 				; +65 */


static byte basic[] =
{
	0x00, 0x0A, 		/* +00 Line 10 */
	0x13, 0x00, 		/* +02 Length 19 */
	0xEA,			/* +04 REM */

	0xF3,			/* +05 DI */
	0x31, 0x00, 0x00,	/* +06 LD SP, nnnn */
	0xDD, 0x21, 0xE5, 0x3F,	/* LD IX, 3FE5h */
	0x11, 0x1B, 0xC0,	/* LD DE, 0C01Bh */
	0x3E, 'S',		/* LD A, 'S' */
	0x37,			/* SCF */
	0xC3, 0x56, 0x05,	/* JP 556h */	

	0x0D,				/* End of line 10 */
	0x00, 0x14, 			/* Line 20 */
	0x32, 0x00,			/* <len 50> */
	0xF1, 'x', '=', 0xC0, '(', '5',	/* LET x=USR (5 */
/*	0xF1, 'x', '=', 0x20, '(', '5',	 * LET x= (5         {for testing} */
	0x0E, 0, 0, 5, 0, 0, '+', 	/* [number 5] + */
	0xBE, '2', '3', '6', '3', '5',	/* PEEK 23635 */
	0x0E, 0, 0, 0x53, 0x5C, 0, '+',	/* [number 23635] + */
	'2', '5', '6',			/* + 256 */
	0x0E, 0, 0, 0, 1, 0, '*',	/* [number 256] * */
	0xBE, '2', '3', '6', '3', '6',	/* PEEK 23636 */
	0x0E, 0, 0, 0x54, 0x5C, 0,	/* [number 23636] */
	')', 0x0D	


};

static byte tapheader[17] = 
{
	0x00,		/* BASIC */
	'0', '1', '2', '3', '4', 
	'5', '6', '7', '8', '9', 
	0x00, 0x00,	/* program+vars size */
	0x00, 0x00,	/* autostart line */
	0x00, 0x00,	/* program size */
};




char *mk_loader(int base)
{
	unsigned char prelude[PRELUDE_MAX];
	unsigned baseu;
	unsigned newsp;
	int n;

	memset(prelude, 0, sizeof prelude);
	if (base == -1) 
	{
		baseu = sp - PRELUDE_MIN;
		st_prelude_size = PRELUDE_MIN;
		fprintf(stderr, "Automatically placing loader code at 0x%x\n",
				baseu);
	}
	else
	{
		baseu = base;
		st_prelude_size = PRELUDE_MAX;
/* If they just so happened to pick the right place at the bottom of the
 * stack, take advantage of it */
		if (baseu == (sp - PRELUDE_MIN))
		{
			st_prelude_size = PRELUDE_MIN;
		}
/*		fprintf(stderr, "Manually placing loader code at 0x%x\n",
				baseu); */
	}
	if (baseu < 0x4000 || baseu >= (0xFFFF - st_prelude_size))
	{
		return "Loader code will not fit; specify a different "
			"address with --base.";
	}

/*	fprintf(stderr, "base=%04x\n", baseu); */

	memcpy(prelude, launch, sizeof(launch));

	poke2(prelude, 24, sp);
	switch (imm)
	{
		case 0: prelude[26] = 0xED; prelude[27] = 0x46; break;
		default:
		case 1: prelude[26] = 0xED; prelude[27] = 0x56; break;
		case 2: prelude[26] = 0xED; prelude[27] = 0x5E; break;
	}
	prelude[28] = (iff & 1) ? 0xFB : 0xF3;	/* EI or DI */

/* If we're at the bottom of the stack, don't need to load SP. Cut that
 * instruction out. */
	if (st_prelude_size == PRELUDE_MIN)
	{
		for (n = 23; n < PRELUDE_MIN; n++)
		{
			prelude[n] = prelude[n+3];
		}
		newsp = 34;
	}
	else newsp = 37;
	/* Point BASIC at our last-stage-launcher code */
	poke2(basic, 7, baseu + newsp + 2);
	poke2(prelude, newsp, baseu); newsp += 2;
	poke2(prelude, newsp, hl1); newsp += 2;
	poke2(prelude, newsp, de1); newsp += 2;
	poke2(prelude, newsp, bc1); newsp += 2;
	poke2(prelude, newsp, af1); newsp += 2;
	poke2(prelude, newsp, iy); newsp += 2;
	poke2(prelude, newsp, ix); newsp += 2;
	poke2(prelude, newsp, hl); newsp += 2;
	poke2(prelude, newsp, de); newsp += 2;
	poke2(prelude, newsp, bc); newsp += 2;
	poke2(prelude, newsp, border); newsp += 2;
	poke2(prelude, newsp, i); newsp += 2;
	poke2(prelude, newsp, r); newsp += 2;
	poke2(prelude, newsp, af); newsp += 2;


	memcpy(sna + baseu - 0x3fe5, prelude, st_prelude_size);
	return NULL;
}


char *save(FILE *fp, byte *addr, unsigned count)
{
	if ((unsigned)fwrite(addr, 1, count, fp) < count)
	{
		return "Cannot write to output file";
	}
	return NULL;
}

char *saveblock(FILE *fp, byte *data, unsigned count, byte type)
{
	byte blockhead[3];
	byte sum;
	unsigned n;
	char *boo;

	poke2(blockhead, 0, 2 + count);
	blockhead[2] = type;

	boo = save(fp, blockhead, 3); if (boo) return boo;
	boo = save(fp, data, count); if (boo) return boo;

	for (n = 0, sum = type; n < count; n++) sum ^= data[n];
	return save(fp, &sum, 1);
}


char *cnv_execute(FILE *infp, FILE *outfp)
{
	const char *boo;
	TIO_PFILE tape;

	if ((unsigned)fread(sna, 1, sizeof(sna), infp) < sizeof(sna))
	{
		return "Premature EOF on SNA file";
	}
	parse_sna();
	boo = mk_loader(st_base); if (boo) return (char *)boo;

	memcpy(tapheader+1, st_name, 10);
	poke2(tapheader, 11, sizeof(basic));
	poke2(tapheader, 15, sizeof(basic));

	boo = tio_mktemp(&tape, st_format); 
	if (boo) return (char *)boo;
/* Save the BASIC header */
	boo = tio_writezx(tape, 0, tapheader, sizeof(tapheader)); 
	if (boo) return (char *)boo;
/* Save the BASIC block */
	boo = tio_writezx(tape, 0xFF, basic, sizeof(basic)); 
	if (boo) return (char *)boo;
	boo = tio_writezx(tape, 'S', sna, sizeof(sna));
	if (boo) return (char *)boo;

	return (char *)tio_closetemp(&tape, outfp);
}

