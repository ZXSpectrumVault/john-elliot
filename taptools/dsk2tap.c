/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "dskbits.h"

#ifdef __PACIFIC__
#define AV0 "DSK2TAP"
#else
#define AV0 argv[0]
#endif

/* Based on dsktrans from libdsk, suitably amended */

/* The TAP file will consist of:
 * 1. BASIC loader plus diskette writer
 * 2. One or more blocks of diskette data. These will be a stream of 
 *   instructions:
 * 	0x46, nn:	Format track nn (logical track no.)
 *	0x4C:		Load next data block
 *	0x51:		Quit
 *	0x57, nn, ss	Write logical track nn, logical sector ss
 *	0x58:		Set geometry. Followed by a 10-byte PCW geometry.
 *
 * The diskette writer will live in video RAM, including the stack.
 * Memory from 24000 - top of RAM will be used for playback.
 * For a normal 173k diskette, each track will be saved as:
 *   0x46, nn:  Format track nn
 *           	36 bytes sector headers
 *   0x57, nn, ss: Write sector nn  }
 * 		512 bytes data      } x 9
 *
 *   Totalling 38 + 4635 = 4673 bytes
 *
 *   We have 41536 bytes in the buffer, allowing roughly 8 tracks to be
 *   buffered at once.
 */     

#define BUFFER_LEN 41536

unsigned char st_outbuf[BUFFER_LEN];
unsigned st_outpos;
unsigned char st_header[] = 
{
	0x13, 0x00,	/* 00-01 Length of header */
	0x00,		/* 02    Header block */
	0x44, '0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* 03-0D ID */
	0xff, 0xff,	/* 0E-0F Length of data */
	0xc0, 0x5d,	/* 10-11 Load address */
	0x00, 0x00,	/* 12-13 Unused */
	0x00		/* 14 xsum */
};

FILE *st_outfp;

dsk_err_t flush_outbuf()
{
	unsigned n;
	unsigned char xsum;
	unsigned char buf[3];

	st_header[0x0e] = st_outpos & 0xFF;
	st_header[0x0f] = (st_outpos >> 8) & 0xFF;
	/* Write output buffer, in .TAP format */
	for (xsum = 0, n = 2; n < 0x14; n++)
	{
		xsum ^= st_header[n];
	}	
	st_header[0x14] = xsum;
	if ((size_t)fwrite(st_header, 1, 0x15, st_outfp) < 0x15) return DSK_ERR_SYSERR;	

	buf[0] = (st_outpos + 2) & 0xFF;
	buf[1] = ((st_outpos + 2) >> 8) & 0xFF;
	buf[2] = 0xFF;

	for (xsum = 0xFF, n = 0; n < st_outpos; n++)
	{
		xsum ^= st_outbuf[n];
	}
	if ((size_t)fwrite(buf, 1, 3, st_outfp) < 3) return DSK_ERR_SYSERR;
	if ((size_t)fwrite(st_outbuf, 1, st_outpos, st_outfp) < st_outpos) 
		return DSK_ERR_SYSERR;
	if ((size_t)fwrite(&xsum, 1, 1, st_outfp) < 1) return DSK_ERR_SYSERR;

	st_outpos = 0;
	++st_header[4];
	return DSK_ERR_OK;
}

dsk_err_t append_command(unsigned len, unsigned char *block)
{
	dsk_err_t err;

	if (st_outpos + len >= BUFFER_LEN)
	{
		st_outbuf[st_outpos++] = 'L';	// End of block
		err = flush_outbuf();
	}
	memcpy(&st_outbuf[st_outpos], block, len);
	st_outpos += len;
	return DSK_ERR_OK;
}


/* Inlined bits from libdsk: utilopts.c and formname.c */

#undef strcmpi

#if HAVE_STRCMPI
        /* */
#else
 #if HAVE_STRICMP
  #define strcmpi stricmp
 #else
  #if HAVE_STRCASECMP
   #define strcmpi strcasecmp
  #endif
 #endif
#endif


void excise_arg(int na, int *argc, char **argv);
int find_arg(char *arg, int argc, char **argv);

int version(void)
{
	printf("libdsk version %s\n", LIBDSK_VERSION);
	return 0;
}


void valid_formats(void)
{
	dsk_format_t format;
	dsk_cchar_t fname, fdesc;

	fprintf(stderr, "\nValid formats are:\n");

	format = FMT_180K;
	while (dg_stdformat(NULL, format++, &fname, &fdesc) == DSK_ERR_OK)
	{
		fprintf(stderr, "   %-10.10s : %s\n", fname, fdesc);
	}
}

dsk_format_t check_format(char *arg, int *argc, char **argv)
{
	int fmt;
	int n = find_arg(arg, *argc, argv);
	dsk_cchar_t fname;
	char *argname;

	if (n < 0) return -1;
	excise_arg(n, argc, argv);
	if (n >= *argc) 
	{
		fprintf(stderr, "Syntax error: use '%s <format>'\n", arg);
		exit(1);
	}
	argname = argv[n];
	excise_arg(n, argc, argv);
	fmt = FMT_180K;
	while (dg_stdformat(NULL, fmt, &fname, NULL) == DSK_ERR_OK)
	{
		if (!strcmpi(argname, fname)) return fmt;
		++fmt;
	}
	fprintf(stderr, "Format name %s not recognised.\n", argname);
	exit(1);
	return FMT_180K;
}



/***/

void excise_arg(int na, int *argc, char **argv)
{	
	int n;

	--(*argc);
	for (n = na; n < *argc; n++)
	{
		argv[n] = argv[n+1];
	}
}

int find_arg(char *arg, int argc, char **argv)
{
	int n;
	
	for (n = 1; n < argc; n++)
	{
		if (!strcmpi(argv[n], "--")) return -1;
		if (!strcmpi(argv[n], arg)) return n;
	}
	return -1;
}


int check_forcehead(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	int fh;

	if (n < 0) return -1;	
	/* Remove the "-head" */
	excise_arg(n, argc, argv);
	if (n >= *argc || argv[n][0] < '0' || argv[n][0] > '1') 
	{
		fprintf(stderr, "Syntax error: use '%s 0' or '%s 1'\n", arg, arg);
		exit(1);
	}
	fh = argv[n][0] - '0';
	excise_arg(n, argc, argv);

	return fh;
}


unsigned check_retry(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	unsigned nr;

	if (n < 0) return 1;	
	/* Remove the "-retryy" */
	excise_arg(n, argc, argv);
	if (n >= *argc || atoi(argv[n]) == 0)
	{
		fprintf(stderr, "Syntax error: use '%s nnn' where nnn is nonzero\n", arg);
		exit(1);
	}
	nr = atoi(argv[n]);
	excise_arg(n, argc, argv);

	return nr;
}


char *check_type(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	char *v;

	if (n < 0) return NULL;
	/* Remove the "-type" */
	excise_arg(n, argc, argv);
	if (n >= *argc)
	{
		fprintf(stderr, "Syntax error: use '%s <type>'\n", arg);
		exit(1);
	}
	v = argv[n];
	/* Remove the argument */
	excise_arg(n, argc, argv);

	if (!strcmpi(v, "auto")) return NULL;
	return v;
}


int present_arg(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);

	if (n < 0) return 0;

	excise_arg(n, argc, argv);
	return 1;
}


int ignore_arg(char *arg, int count, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);

	if (n < 0) return 0;

	fprintf(stderr, "Warning: option '");
	while (count)
	{
		fprintf(stderr, "%s ", argv[n]);
		excise_arg(n, argc, argv);
		--count;
		if (n >= *argc) break;	
	}
	fprintf(stderr, "\b' ignored.\n");
	return 1;
}

void args_complete(int *argc, char **argv)
{
	int n;
	for (n = 1; n < *argc; n++)
	{
		if (!strcmpi(argv[n], "--")) 
		{
			excise_arg(n, argc, argv);
			return;
		}
		if (argv[n][0] == '-')
		{
			fprintf(stderr, "Unknown option: %s\n", argv[n]);
			exit(1);
		}
	}
}

/***/

static int stubborn = 0;
static int noformat = 0;
static dsk_format_t format = -1;
static char *intyp = NULL;
static char *incomp = NULL;
static int inside = -1;
static int idstep =  0;
static int retries = 1;

int do_copy(char *infile, char *outfile);

int check_numeric(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	unsigned nr;

	if (n < 0) return -1;	
	excise_arg(n, argc, argv);
	if (n >= *argc || atoi(argv[n]) == 0)
	{
		fprintf(stderr, "Syntax error: use '%s nnn' where nnn is nonzero\n", arg);
		exit(1);
	}
	nr = atoi(argv[n]);
	excise_arg(n, argc, argv);

	return nr;
}


static void report(const char *s)
{
        printf("%s\r", s);
        fflush(stdout);
}

static void report_end(void)
{
        printf("\r%-79.79s\r", "");
        fflush(stdout);
}


int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                       "      %s {options} dskfile tapfile\n",
			AV0);
	fprintf(stderr,"\nOptions are:\n"
		       "-type <type>    type of input disc image\n"
                       "-side <side>    Force side 0 or side 1 of input\n"
		       "-retry <count>  Set number of retries on error\n"
		       "-dstep          Double-step when reading\n"
                       "-stubborn       Ignore any read errors\n"
		       "-ftap           Output in TAP format (default)\n"
		       "-fpzx           Output in PZX format\n"
		       "-ftzx           Output in TZX format\n"
		       "-fzxt           Output in ZXT format\n"
		       "-format         Force a specified format name\n");
	fprintf(stderr,"\nDefault in-image type is autodetect.\n\n");
		
	valid_formats();
	return 1;
}



int main(int argc, char **argv)
{

	if (find_arg("--version", argc, argv) > 0) return version(); 
	if (argc < 3) return help(argc, argv);
	if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);

	intyp     = check_type("-type", &argc, argv); 
	incomp    = check_type("-comp", &argc, argv); 
        inside    = check_forcehead("-side", &argc, argv);
	retries   = check_retry("-retry", &argc, argv);
	if (present_arg("-dstep", &argc, argv)) idstep = 1;
	if (present_arg("-stubborn", &argc, argv)) stubborn = 1;
	if (present_arg("-noformat", &argc, argv)) noformat = 1;
        format    = check_format("-format", &argc, argv);
	args_complete(&argc, argv);
	return do_copy(argv[1], argv[2]);
}

static unsigned char format_buf[1026];

/* A standard PCW 180k boot spec */
static unsigned char pcw180[11] = 
{
	'X', 0, 0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52
};


static unsigned char basic_loader[] =
{
	0x13,0x00,	/* 19 byte block */
	0x00,		/* Header */
	0x00,		/* BASIC program */
	'd', 's', 'k', '2', 't', 'a', 'p', ' ', ' ', ' ',
			/* filename */
	0x20, 0x00,	/* Length */
	0x01, 0x00,	/* Start line */
	0x20, 0x00,	/* Length of program */
	0x0a,		/* Checksum */

	0x22, 0x00,	/* 34 byte block */
	0xff,		/* Data */
	0x00, 0x0a,	/* Line 10 */
	0x1C, 0x00,	/* Length 28 */
	0xfd, 0xb0, 0x22, '2', '3', '9', '9', '9', 0x22, 0x3a,
			/* CLEAR VAL "23999": */
	0xEF, 0x22, 0x22, 0xAF, 0x3a,	/* LOAD "" CODE: */
	0xf9, 0xc0, 0xb0, 0x22, '1','8','4','3','2',0x22, 0x3a,
			/* RANDOMIZE USR VAL "18432": */
	0xE2, 0x0D,	/* STOP */ 
	0xBC		/* Checksum */
};

static unsigned char code_header[] =
{
	0x13, 0x00,	/* +0: 19 byte block */
	0x00, 0x03,	/* +2: Header, type CODE */
	'd', 's', 'k', '2', 't', 'a', 'p', '.', 'm', 'c',
			/* +4: Filename */
	0x33, 0x05,	/* +14: Length */
	0x00, 0x48,	/* +16: Load address */
	0x00, 0x00,	/* +18: Unused */
	0x00		/* +20: Checksum */
};

/* The Z80 code that parses the remaining data blocks and plays them back. 
 * 
 * Source for this is in dskwrite.zsm.
 */
static unsigned char code_block[] = 
{
#include "dskwrite.inc"
};

int write_prelude(void)
{
	unsigned n;
	unsigned char buf[3], xsum;

	if ((size_t)fwrite(basic_loader, 1, sizeof(basic_loader), st_outfp) < sizeof(basic_loader))
		return DSK_ERR_SYSERR;

/* We could hardcode the size and checksum of the loader block, but for ease 
 * of future modification, do them dynamically */
	code_header[14] = sizeof(code_block) & 0xFF;
	code_header[15] = (sizeof(code_block) >> 8) & 0xFF;	

	code_header[20] = 0;
	for (n = 2; n < 20; n++) code_header[20] ^= code_header[n];
	if ((size_t)fwrite(code_header, 1, sizeof(code_header), st_outfp) < sizeof(code_header))
		return DSK_ERR_SYSERR;

	buf[0] = (sizeof(code_block) + 2) & 0xFF;
	buf[1] = ((sizeof(code_block) + 2) >> 8) & 0xFF;
	buf[2] = 0xFF;

	for (xsum = 0xFF, n = 0; n < sizeof(code_block); n++)
	{
		xsum ^= code_block[n];
	}
	if ((size_t)fwrite(buf, 1, sizeof(buf), st_outfp) < sizeof(buf))
		return DSK_ERR_SYSERR;
	if ((size_t)fwrite(code_block, 1, sizeof(code_block), st_outfp) < sizeof(code_block))
		return DSK_ERR_SYSERR;
	if ((size_t)fwrite(&xsum, 1, 1, st_outfp) < 1) 
		return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}



int do_copy(char *infile, char *outfile)
{
	DSK_PDRIVER indr = NULL;
	dsk_err_t e;
	dsk_lsect_t sec;
	dsk_ltrack_t track, tracks;
	unsigned char *buf = NULL;
	DSK_GEOMETRY dg;
	char *op = "Opening";
	int n;

	st_outpos = 0;
	if (!strcmp(LIBDSK_VERSION, "none"))
	{
		fprintf(stderr, "LibDsk was not present when this program was compiled.\nCannot continue.\n");
		exit(1);
	}

	st_outfp = fopen(outfile, "wb");
	if (st_outfp == NULL)
	{
		return DSK_ERR_SYSERR;
	}
	st_header[4] = '0';
	e = write_prelude();
	if (e) return e;

        dsk_reportfunc_set(report, report_end);

	        e = dsk_open (&indr,  infile,  intyp, incomp);
	if (!e) e = dsk_set_retry(indr, retries);
	if (!e && inside >= 0) e = dsk_set_option(indr, "HEAD", inside);
	if (!e && idstep) e = dsk_set_option(indr, "DOUBLESTEP", 1);
	if (format == -1)
	{
		op = "Identifying disc";
		if (!e) e = dsk_getgeom(indr, &dg);
	}
	else if (!e) e = dg_stdformat(&dg, format, NULL, NULL);
	if (!e)
	{	
		buf = dsk_malloc(dg.dg_secsize + 8);
		if (!buf) e = DSK_ERR_NOMEM;
	}
	if (!e)
	{
		int havegeom;

		tracks = dg.dg_cylinders * dg.dg_heads;

		/* Copy geometry */
		havegeom = 0;

/* If the format is being autoprobed and the disc is in PCW format, its 
 * boot block gives the format, so trust it. */
		if (format == -1 && 
		    dg.dg_secbase == 1 && 
		    !dsk_pread(indr, &dg, &buf[1], 0, 0, 1))
		{
			DSK_GEOMETRY spare;

			if (dg_pcwgeom(&spare, &buf[1]) == DSK_ERR_OK)
			{
				havegeom = 1;
				if (buf[1] == 0xE5)
				{
					append_command(11, pcw180);
				}
				else
				{	
					buf[0] = 'X';	
					append_command(11, buf);
				}
			}
		}
/* Else, build one from the DSK_GEOMETRY */
		if (!havegeom)
		{
			buf[0] = 'X';
			if (dg.dg_heads > 1 || dg.dg_cylinders > 42)
				buf[1] = 3;
			else	buf[1] = 0;
			buf[2] = 0;	/* Sidedness */
			if (dg.dg_heads > 1) switch (dg.dg_sidedness)
			{
				case SIDES_ALT:	buf[2] = 1; break;
				default:	buf[2] = 2; break;
			}
			/* Assume double-track if >42 cylinders */
			if (dg.dg_cylinders > 42) buf[2] |= 0x80;
			buf[3] = dg.dg_cylinders;
			buf[4] = dg.dg_sectors;
			buf[5] = dsk_get_psh(dg.dg_secsize);
			buf[6] = 1;	/* reserved tracks */
			buf[7] = 4;	/* CP/M block shift */
			buf[8] = 2;	/* CP/M directory blocks */
			buf[9] = dg.dg_rwgap;
			buf[10] = dg.dg_fmtgap;
			append_command(11, buf);
		}

/* Copy in logical order, since that's what +3DOS will use at the other 
  end. */
		for (track = 0; track < tracks; track++)
		{
			format_buf[0] = 'F';
			format_buf[1] = track;
			if (!e) for (sec = 0; sec < dg.dg_sectors; ++sec)
			{
				dsk_pcyl_t cyl;
				dsk_phead_t head;
				dsk_psect_t psec;

				dg_ls2ps(&dg,
					track * dg.dg_sectors + sec,
					&cyl, &head, &psec);

				format_buf[sec * 4 + 2] = cyl;
				format_buf[sec * 4 + 3] = head;
				format_buf[sec * 4 + 4] = psec;
				format_buf[sec * 4 + 5] = dsk_get_psh(dg.dg_secsize);
			}
			if (!e) e = append_command(2 + 4 * dg.dg_sectors,
					format_buf);
			
			if (!e) for (sec = 0; sec < dg.dg_sectors; ++sec)
			{
				op = "Reading";
				e = dsk_lread(indr, &dg, buf+3, sec + (dg.dg_sectors * track));
				if (stubborn && (e <= DSK_ERR_NOTRDY && e >= DSK_ERR_CTRLR))
				{
					fprintf(stderr, "\r%-79.79s\rIgnored read error: %s\n", "", dsk_strerror(e));
					e = DSK_ERR_OK;
				}
				if (e) break;

/* We do only the simplest compression:
 * 1. All-E5 sectors are skipped, because a newly formatted sector is all-E5.
 * 2. Other sectors that are all one byte get stored as that byte.
 *
 * More sophisticated compression would be possible, but requires changes to
 * the disk writer module.
 */
		
				op = "Writing";
				for (n = 1; n < dg.dg_secsize; n++)
				{
					if (buf[n + 3] != buf[3]) break;
				}
				if (n == dg.dg_secsize)	/* All bytes the same*/
				{
					if (buf[3] != 0xE5)
					{
						buf[0] = 'U';
						buf[1] = track;
						buf[2] = sec;
						e = append_command(4, buf);
					}
				}
				else
				{
					buf[0] = 'W';
					buf[1] = track;
					buf[2] = sec;
					e = append_command(3 + dg.dg_secsize, buf);
				}
			}
			if (e) break;
		}
	}
	append_command(1, (unsigned char *)"Q");	// Complete
	flush_outbuf();	
	printf("\r                                     \r");
	if (indr)  dsk_close(&indr);
	if (buf) dsk_free(buf);
	if (st_outfp) fclose(st_outfp);
	if (e)
	{
		fprintf(stderr, "\n%s: %s\n", op, dsk_strerror(e));
		return 1;
	}
	return 0;
}

