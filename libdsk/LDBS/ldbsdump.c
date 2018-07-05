/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright noticldbs2dske and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* LDBS example software: 
 *      Dump the contents of an LDBS-format disk image
 *
 * LDBS 0.1 as specified in ldbs.html
 *
 * This does not use any of the LDBS libraries, and can thus be used as 
 * an independent check of what they do.
 */

/* If -raw is passed, dumps all blocks in the file, in file order.
 * Otherwise, looks for the track directory and dumps those blocks it
 * contains, in directory order */
static int raw = 0;




unsigned short peek2(unsigned char *c)
{
	unsigned short v = c[1];

	v = (v << 8) | c[0];
	return v;
}


long peek4(unsigned char *c)
{
	long v;

	v = c[3];
	v = (v << 8) | c[2];
	v = (v << 8) | c[1];
	v = (v << 8) | c[0];
	return v;
}



unsigned char *load_block(FILE *fp, long pos, char *type, 
	size_t *blocklen)
{
	unsigned char blockhead[20];
	unsigned char *buf;

	if (fseek(fp, pos, SEEK_SET))
	{
		fprintf(stderr, "Seek error\n");
		return NULL;
	}
	if (fread(blockhead, 1, sizeof(blockhead), fp) < sizeof(blockhead) ||
	    memcmp(blockhead, "LDB\1", 4))
	{
		fprintf(stderr, "Block header not found at 0x%08lx\n", pos);
		return NULL;
	}
	*blocklen = peek4(blockhead + 12);	
	buf = malloc(*blocklen);
	if (!buf)
	{
		fprintf(stderr, "Out of memory loading block at 0x%08lx\n", pos);
		fseek(fp, pos + peek4(blockhead + 8), SEEK_SET);
		return NULL;
	}
	if (fread(buf, 1, *blocklen, fp) < *blocklen)
	{
		free(buf);
		fprintf(stderr, "Unexpected EOF loading block at 0x%08lx\n", pos);
		fseek(fp, pos + peek4(blockhead + 8), SEEK_SET);
		return NULL;
	}
	/* Seek to start of next block */
	if (fseek(fp, pos + 20 + peek4(blockhead + 8), SEEK_SET))
	{
		fprintf(stderr, "Seek error after block at 0x%08lx\n", pos);
	}
	memcpy(type, blockhead + 4, 4);
	return buf;
}

void show_blocktype(FILE *fp, const char *t)
{
	int n;

	if (t[0] == 0 && t[1] == 0 && t[2] == 0 && t[3] == 0)
	{
		fprintf(fp, "-- FREE --");
		return;
	}

	for (n = 0; n < 4; n++)
	{
		fprintf(fp, "%02x ", t[n]);
	}
	fprintf(fp, "(");
	for (n = 0; n < 4; n++)
	{
		if (isprint(t[n])) fputc(t[n], fp);
		else		fputc('.', fp);
	}
	fprintf(fp, ")");
}


void dump_block(FILE *fp, long pos)
{
	char type[4];
	size_t n, blocklen;
	unsigned char *data = load_block(fp, pos, type, &blocklen);
	char hex[75], txt[17];

	if (!data) return;
	printf("%08lx | Data block [length %u]: ", pos, (unsigned)blocklen);
	show_blocktype(stdout, type);	
	putchar('\n');

	for (n = 0; n < blocklen; n++)
	{
		if ((n % 16) == 0)
		{
			memset(hex, 0, sizeof(hex));
			memset(txt, 0, sizeof(txt));
		}
		sprintf(hex + strlen(hex), "%02x ", data[n]);
		txt[n % 16] = isprint(data[n]) ? data[n] : '.';	
		if ((n % 16) == 15)
		{
			printf("%-10.10s%-50.50s %s\n", "", hex, txt);
		}
	}	
	if ((n % 16) != 0)
	{
		printf("%-10.10s%-50.50s %s\n", "", hex, txt);
	}
	putchar('\n');
	free(data);
}





void dump_track(FILE *fp, long pos, int version)
{
	char type[4];
	size_t n, count, blocklen;
	unsigned char *trkhead = load_block(fp, pos, type, &blocklen);
	unsigned se_len, se_offset, th_offset;

	if (!trkhead) return;
	if (version < 2)
	{
		th_offset = 2;
		se_offset = 6;
		se_len    = 12;
		count = peek2(trkhead);
	}
	else
	{
		th_offset = 6;
		se_offset = peek2(trkhead);
		se_len    = peek2(trkhead + 2);
		count     = peek2(trkhead + 4);
	}
	printf("%08lx |     Track header (%u entries):\n", pos, (unsigned)count);
	printf("                   Data rate: %d\n", trkhead[th_offset]);
	printf("                   Rec mode:  %d\n", trkhead[th_offset+1]);
	printf("                   Fmt GAP3:  %d\n", trkhead[th_offset+2]);
	printf("                   Filler:    0x%02x\n", trkhead[th_offset+3]);
	if (se_offset >= 12)
		printf("                   Total len: 0x%04x\n", peek2(trkhead + th_offset + 4));
	for (n = 0; n < count; n++)
	{
		printf("                   Cyl 0x%02x Head 0x%02x Sec 0x%02x Size 0x%02x\n", 
			trkhead[se_offset + se_len*n],
			trkhead[se_offset + 1 + se_len*n],
			trkhead[se_offset + 2 + se_len*n],
			trkhead[se_offset + 3 + se_len*n]);
		printf("                   ST1 0x%02x ST2  0x%02x Copies %d Fill 0x%02x ", 
			trkhead[se_offset + 4 + se_len*n],
			trkhead[se_offset + 5 + se_len*n],
			trkhead[se_offset + 6 + se_len*n],
			trkhead[se_offset + 7 + se_len*n]);

		printf(" @ 0x%08lx\n", peek4(trkhead + se_offset + 8 + se_len*n));
		if (se_len >= 16)
		{
		    printf("                   Trail 0x%04x Offset 0x%04x\n",
			peek2(trkhead + se_offset + 12 + se_len * n),
			peek2(trkhead + se_offset + 14 + se_len * n));

		}
	}

	for (n = 0; n < count; n++)
	{
		if (trkhead[se_offset + 6 + se_len * n] &&
		    peek4(trkhead + se_offset + 8 + se_len * n))
		{
			dump_block(fp, peek4(trkhead + se_offset + 8 + se_len*n));
		}	
	}

	free(trkhead);
}



void dump_disk(FILE *fp, long pos, int version)
{
	char type[4];
	size_t n, count, blocklen;
	unsigned char *trackdir = load_block(fp, pos, type, &blocklen);

	if (!trackdir) return;
	if (memcmp(type, "DIR\1", 4))
	{
		fprintf(stderr, "Track directory not found: Block type is ");
		show_blocktype(stderr, type);
		fprintf(stderr, "\n");
		free(trackdir);
		return;
	}
	count = peek2(trackdir);
	printf("%08lx | Track directory (%u entries):\n", pos, (unsigned)count);
	for (n = 0; n < count; n++)
	{
		printf("               ");
		show_blocktype(stdout, (char *)(trackdir + 2 + 8 * n));
		printf(" @ 0x%08lx\n", peek4(trackdir + 6 + 8 * n));
	}

	for (n = 0; n < count; n++)
	{
		if (trackdir[2+8*n] == 'T')
		{
			dump_track(fp, peek4(trackdir + 6 + 8 * n), version);
		}
		else
		{
			dump_block(fp, peek4(trackdir + 6 + 8 * n));
		}	
	}

	free(trackdir);
}



void dump_file(const char *filename)
{
	FILE *fp;
	unsigned char header[20];
	unsigned char blockhead[20];
	long pos;

	/* Load the .DSK header */
	fp = fopen(filename, "rb");
	if (!fp) { perror(filename); return; }

	memset(header, 0, sizeof(header));
	if ((fread(header, 1, sizeof(header), fp) < sizeof(header)) ||
		memcmp(header, "LBS\1", 4))
	{
		fprintf(stderr, "%s: File is not in LDBS format\n", filename);
		return;
	}
	printf("%08lx | File header:\n", 0L);
	printf("           File type: "); 
	show_blocktype(stdout, (char *)(header + 4));
	printf("\n");
	printf("           First used block @ 0x%08lx\n", peek4(header + 8));
	printf("           First free block @ 0x%08lx\n", peek4(header + 12));
	printf("           Track directory  @ 0x%08lx\n", peek4(header + 16));
	putchar('\n');

	if (raw == 0 && peek4(header + 16) &&
		(!memcmp(header + 4, "DSK\1", 4) ||
		 !memcmp(header + 4, "DSK\2", 4)))
	{
		dump_disk(fp, peek4(header + 16), header[7]);
	}
	else
	{
		pos = ftell(fp);
		while (fread(blockhead, 1, sizeof(blockhead), fp) ==
				sizeof(blockhead))
		{
			dump_block(fp, pos);
			pos = ftell(fp);
		}
	}

}



int main(int argc, char **argv)
{
	int n;

	if (argc < 2)
	{
		fprintf(stderr, "%s: Syntax is %s <dskfile> <dskfile> ...\n",
				argv[0], argv[0]);
		exit(1);
	}
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--raw") || !strcmp(argv[n], "-raw"))
		{
			raw = 1;
			continue;
		}

		dump_file(argv[n]);
	}
	return 0;
}
