
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include  "tapeio.h"

#ifdef __PACIFIC__
#define ARG0 "TAPLS"
#else
#define ARG0 argv[0]
#endif

static int verbose = 0;
static int list_format = 0;

typedef enum
{
	NORMAL,
	IN_ZX,
	IN_IBM_BIN,
	IN_IBM_TXT
} LT_STATE;


void printable_filename(unsigned char *src, char *dest, int len)
{
	int n;
	int t;

	t = 0;
	for (n = 0; n < len; n++)
	{
		if (src[n] >= ' ' && src[n] < 0x7F)
		{
			dest[t++] = src[n];
			dest[t] = 0;
		}
		else
		{
			sprintf(dest + t, "\\0%o", src[n]);
			t = strlen(dest);
		}
	}
	n = strlen(dest) - 1;
	while (n > 0 && dest[n] == ' ')
	{
		dest[n] = 0;
		--n;
	}
}



void show_ibm_header(TIO_HEADER *hdr)
{
	char filename[41];
	char *filetype;
	char *intro;
	unsigned len = hdr->header[11] + 256 * hdr->header[12];
	unsigned seg = hdr->header[13] + 256 * hdr->header[14];
	unsigned off = hdr->header[15] + 256 * hdr->header[16];

	printable_filename(hdr->header + 2, filename, 8);

	if (hdr->header[10] & 0x20) 
	{
		strcat(filename, ".P");
		filetype = "IBM Protect";
		intro = "IBM Program";
	}
	else if (hdr->header[10] & 0x80) 
	{
		strcat(filename, ".B");
		filetype = "IBM BASIC  ";
		intro = "IBM Program";
	}
	else if (hdr->header[10] & 0x40) 
	{
		strcat(filename, ".A");
		filetype = "IBM ASCII  ";
		intro = "IBM Program";
	}
	else if (hdr->header[10] & 0x01) 
	{
		strcat(filename, ".M");
		filetype = "IBM memory ";
		intro = "IBM Bytes";
	}
	else 
	{
		strcat(filename, ".D");
		filetype = "IBM data   ";
		intro = "IBM Data";
	}

	switch (list_format)
	{
		case 0: printf("%s\n", filename); break;
		case 1: printf("-r--r--r-- %s %5u %s\n", filetype, len,
					filename); break;
		case 2: printf("%s: %s ", intro, filename);
			if (hdr->header[10] & 0xA0)
			{
				printf("Length=%04x", len);
				if (hdr->header[10] & 0x20)
					printf(" Protected");
			}
			else if (hdr->header[10] & 0x01)
			{
				printf("Length=%04x Address=%04x:%04x", len, seg, off);
			}
			putchar('\n');
	}
}


void show_sna_header(TIO_HEADER *hdr)
{
	switch (list_format)
	{
		case 0: printf("%s\n", "(Headerless .SNA snapshot)"); break;
		case 1: printf("-r--r--r-- %s %5u %s\n", "Snapshot   ", 
					hdr->length,
					"(Headerless .SNA snapshot)"); break;
		case 2: printf("%s\n", "Headerless block: .SNA snapshot"); break;
	}
}


void show_unknown(TIO_HEADER *hdr)
{
	switch (list_format)
	{
		case 0: printf("[Unknown type=0x%02x]\n",
				hdr->header[0]); break;
		case 1: printf("-r--r--r-- [0x%02x]      %5u %s\n", 
				hdr->header[0], hdr->length,
					"(Headerless block)"); break;
		case 2: printf("Headerless block: type=0x%02x length=%u\n", 
				hdr->header[0], hdr->length); break;
	}
}


void show_headerless(TIO_HEADER *hdr)
{
	switch (list_format)
	{
		case 0: printf("%s\n", "[Headerless]"); break;
		case 1: printf("-r--r--r-- %s %5u %s\n", 
		(hdr->header[0] == 0x16) ? "IBM hdrless" : "Headerless ", 
				hdr->length, "(Headerless block)"); break;
		case 2: printf("Headerless block: length=%u\n", 
					hdr->length); break;
	}
}

void show_tzxother(TIO_HEADER *hdr)
{
	if (!verbose) return;
	switch (list_format)
	{
		case 2:
		case 0: printf("[Ignoring TZX block type=0x%02x]\n",
				hdr->header[0]); break;
		case 1: printf("-r--r--r-- [0x%02x]      %5u %s\n", 
				hdr->header[0], hdr->length,
					"(Ignoring TZX block)"); break;
	}
}

void show_pzxother(TIO_HEADER *hdr)
{
	if (!verbose) return;
	switch (list_format)
	{
		case 2:
		case 0: printf("[Ignoring PZX block type=%-4.4s]\n",
				hdr->header); break;
		case 1: printf("-r--r--r-- [%-4.4s]      %5u %s\n", 
				hdr->header, hdr->length,
					"(Ignoring PZX block)"); break;

	}
}

void show_spectrum_header(TIO_HEADER *hdr)
{
	char filename[51];
	unsigned len  = hdr->header[12] + 256 * hdr->header[13];
	unsigned add  = hdr->header[14] + 256 * hdr->header[15];
	char filetype[12];
	char *intro;

	switch(hdr->header[1])
	{
		case 0:  strcpy(filetype, "BASIC      "); 
			 intro = "Program";
			 break;
		case 1:  strcpy(filetype, "Numeric    "); 
			 intro = "Number array";
			 break;
		case 2:  strcpy(filetype, "Characters "); 
			 intro = "Character array";
			 break;
		case 3:  strcpy(filetype, "Bytes      "); 
			 intro = "Bytes";
			 break;
		default: sprintf(filetype, "Unknown<%02x>", hdr->header[1]);
			 intro = filetype; 
			break;
	}

	printable_filename(hdr->header + 2, filename, 10);



	switch (list_format)
	{
		case 0: printf("%s\n", filename); break;
		case 1: printf("-r--r--r-- %s %5u %s\n", filetype, len,
					filename); break;
		case 2: printf("%s: %-10s", intro, filename);
			switch (hdr->header[1])
			{
				case 0: if (add < 0x8000) 
						printf(" LINE %d", add);
					break;
				case 1: printf(" DATA %c()", add); break;
				case 2: printf(" DATA %c$()", add); break;
				case 3: printf(" CODE %u,%u", add, len); break;
			}
			putchar('\n');
			break;
	}
}


LT_STATE show_header(TIO_HEADER *hdr, LT_STATE state, TIO_FORMAT fmt)
{
	if (state == IN_IBM_BIN && (hdr->status == TIO_IBM_HEADER ||
				    hdr->status == TIO_IBM_DATA))
	{	
		/* Data block following the header; expected, don't log */
		return NORMAL;
	}
	if (state == IN_IBM_TXT && (hdr->status == TIO_IBM_HEADER ||
				    hdr->status == TIO_IBM_DATA))
	{	
		/* Data block following the header; see if it's the last one */
		if (hdr->header[1] == 0) return IN_IBM_TXT;
		return NORMAL;
	}
	if (state == IN_ZX && hdr->status == TIO_ZX_DATA)
	{
		/* Data block following the header; expected, don't log */
		return NORMAL;
	}
	/* OK, not in a multi-block file. Show what we've got */
	switch (hdr->status)
	{
		case TIO_ZX_HEADER:
			show_spectrum_header(hdr);
			state = IN_ZX;
			break;
		case TIO_IBM_DATA:
		case TIO_ZX_DATA:
			show_headerless(hdr);
			break;
		case TIO_ZX_SNA:
			show_sna_header(hdr);
			break;
		case TIO_IBM_HEADER:
			show_ibm_header(hdr);
	  		if (hdr->header[10] == 0 || (hdr->header[10] & 0x40))
			{
				state = IN_IBM_TXT;
			}
			else
			{
				state = IN_IBM_BIN;
			}
			break;
		case TIO_UNKNOWN:
			show_unknown(hdr);
			break;
		case TIO_TZXOTHER:
			if (fmt == TIOF_TZX_IBM || fmt == TIOF_TZX_SPECTRUM)
				show_tzxother(hdr);
			else	show_pzxother(hdr);
			break;
		default:
			printf("!!Unknown tio_readblock() status=%d length=%d\n", hdr->status, hdr->length);
			break;
	}
	return state;
}



int list_tape(char *filename)
{
	TIO_PFILE tape;
	TIO_FORMAT fmt;
	TIO_HEADER hdr;
	const char *boo;
	LT_STATE state = NORMAL;

	boo = tio_open(&tape, filename, &fmt);
	if (boo)
	{
		fprintf(stderr, "%s\n", boo);
		return 1;
	}
	printf("%s: %s\n", filename, tio_format_desc(fmt));

	memset(&hdr, 0, sizeof(hdr));
	do
	{
		boo = tio_readraw(tape, &hdr, 1);
		if (boo)
		{
			fprintf(stderr, "%s\n", boo);
			tio_close(&tape);
			return 1;
		}
		if (hdr.status == TIO_EOF) break;

		state = show_header(&hdr, state, fmt);
	}
	while (hdr.status != TIO_EOF);
	tio_close(&tape);
	return 0;
}

void syntax (const char *arg0)
{
	fprintf(stderr, "Syntax: %s { options } tapefile { tapefile ... }\n\n"
			" Options:\n"
			"            -3  List in +3DOS CAT \"T:\" style\n"
			"            -l  List in UNIX ls -l style\n"
			"            -v  Show all TZX/PZX blocks\n", 
				arg0);
	exit(0);
}

int main(int argc, char **argv)
{
	int n;
	int errs = 0;
	int nfiles = 0;
	int endargs = 0;

	for (n = 1; n < argc; n++)
	{
		if (argv[n][0] == '-' && !endargs)
		{
			switch (argv[n][1])
			{
				case 'v': case 'V': verbose = 1; break;
				case '3': list_format = 2; break;
				case 'l': case 'L': list_format = 1; break;
				case '-': endargs = 1; break;
				case 'h': case 'H': case '?':
					syntax(ARG0);
					return 0;
			
				default:
					fprintf(stderr, "Unrecognised option: '%s'\n", argv[n]);	
					return 1;
			}
		}
		else 
		{
			errs += list_tape(argv[n]);
			++nfiles;
		}
	}


	if (!nfiles)
	{
		syntax(ARG0);
		errs = 1;
	}
	return errs;
}
