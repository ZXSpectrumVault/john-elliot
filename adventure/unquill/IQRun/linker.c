
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This sort of behaviour is quick and nasty */

unsigned char zcodefile[256*1024];
unsigned char snapfile[65792];

int zlen;
int zxoff;
int lategame;
int cpcgame;
int c64game;

int main(int argc, char **argv)
{
	FILE *fp;

	lategame = 0;
	if (argc < 4)
	{
		fprintf(stderr,"Syntax: %s zcodefile snapfile outfile { L }\n",
			argv[0]);
		exit(1);
	}
	if (argc > 4 && argv[4][0] == 'L') lategame = 10;
	if (argc > 4 && argv[4][0] == 'C') { cpcgame  = 1; lategame = 10; }
	if (argc > 4 && argv[4][0] == '6') { c64game  = 2; lategame = 5;  }
	fp = fopen(argv[1], "rb");
	if (!fp) 
	{
		fprintf(stderr,"Can't open %s\n", argv[1]);
		exit(1);
	}
	zlen = fread(zcodefile, 1, 256*1024, fp);
	fclose(fp);

	for (zxoff = 0; zxoff < (zlen-4); zxoff++)
	{
		if (!memcmp(zcodefile + zxoff, "Achad7", 6)) break; 
	} 
	if (zxoff >= (zlen-4))
	{
		fprintf(stderr, "Can't find the magic word 'Achad7' in %s\n",
			argv[1]);
		exit(1);
	}

	fp = fopen(argv[2], "rb");
	if (fread(snapfile, 1, 65792, fp) < 49179)
	{
		fprintf(stderr,"Warning: %s is too short, and probably "
                               "not in .SNA format.", argv[2]); 
	}
	fclose(fp);
	if (cpcgame)
		memcpy(zcodefile + zxoff + 6, snapfile + 0x1C00, 0xA500);
	else if (c64game)
		memcpy(zcodefile + zxoff + 6, snapfile + 0x874, 0xA500);
	else	memcpy(zcodefile + zxoff + 0x106, snapfile + 0x1c1b, 0xa400);

	zcodefile[zxoff + 0xa506] = 1;		/* Game loaded */
	zcodefile[zxoff + 0xa507] = lategame;	/* "late" game? */
/*	zcodefile[zxoff + 0xa508] = 1;		mono */
 	zcodefile[zxoff + 0xa509] = 1;		/* no graphics */
	zcodefile[zxoff + 0xa50b] = (cpcgame | c64game);/* Architecture */

/* XXX Checksum the new zcode file! */

	fp = fopen(argv[3], "wb");
	if (!fp) 
	{
		fprintf(stderr, "Could not write %s\n", argv[3]);
		exit(1);
	}
	fwrite(zcodefile, 1, zlen, fp);
	fclose(fp);

	return 0;
}
