#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mz.h"
#include "ne.h"
#include "pe.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

static int count = 0;
static FILE *fp;

void font_dump(RESDIRENTRY *re)
{
	char fname[50];
	FILE *fpo;
	byte *fntfile;
	unsigned long len, actual;
	int ptSz, drvo, incr;

	len = (*re->get_size)(re);

	++count;
	fntfile = malloc(len);
	if (!fntfile) 
	{
		fprintf(stderr, "Out of memory!\n");
		return;
	}
	memset(fntfile, 0, len);
        fseek(fp, (*re->get_address)(re), SEEK_SET);
	actual = fread(fntfile, 1, len, fp);
	if (actual < len)
	{
		fprintf(stderr,"Warning: Failed to load all of FNT resource\n"
				"Resource size=%ld Bytes read=%ld\n", 
		      len, actual );
	}	
	ptSz = PEEK16(fntfile, 0x44);
	drvo = PEEK32(fntfile, 0x69);

	incr = 0;
/* Find a unique name for the font to be output under */
	do
	{
		if (!incr) sprintf(fname, "%s_%d.fnt", fntfile + drvo, ptSz);
		else sprintf(fname, "%s_%d_%d.fnt", fntfile + drvo, ptSz, incr);
		fpo = fopen(fname, "rb");
			
		if (fpo == NULL)
		{
			incr = 0;
		}
		else
		{
			fclose(fpo);
			++incr;
		}
	} while (incr);
	fpo = fopen(fname, "wb");
	if (!fpo) 
	{
		free(fntfile);
		perror(fname); 
		return; 
	}

	if (fwrite(fntfile, 1, len, fpo) < len) perror(fname);
	fclose(fpo);
	free(fntfile);
}


void rdir_dump(RESDIR *rdir)
{
	int idx = 0;
	char buf[30];
	RESDIRENTRY *re, *re2;	
	RESDIR *subdir;
	
	re = (*rdir->find_first)(rdir);
	while (re)
	{
		(*re->get_name)(re, buf, 30);
		subdir = (*re->open_subdir)(re);
		if (subdir)
		{
			rdir_dump(subdir);
		}
		else	
		{	
			if ((*re->get_type)(re) == RT_FONT) font_dump(re);
		}
		++idx;
		re2 = (*rdir->find_next)(rdir, re);
		free(re);
		re = re2;
	}
}

void rsc_dump(MZFILE *mz)
{
	RESDIR *rdir = (*mz->open_resdir)(mz);

	if (rdir) 
	{
		rdir_dump(rdir);
		free(rdir);
	}
	else printf("No resources found.\n"); 
}

int main(int argc, char **argv)
{
	int	na;
	MZFILE *mz;

	if (argc < 2) exit(1);

	for (na = 1; na < argc; na++)
	{
		count = 0;
		fp = fopen(argv[na], "rb");
		if (!fp) { perror(argv[na]); continue; }

		mz = get_mzfile(fp);
		if (!mz) 
		{
			printf("%s: Can't determine type\n", argv[na]);
		}
		else switch( mz_type(mz) )
		{
			case 1:	printf("%s: This file is in MZ (DOS) format "
				       "and has no resources.\n", argv[na]);
				break;

			case 2:	rsc_dump(mz);	
				break;

			case 3:	rsc_dump(mz);	
				break;

			case 4:	printf("%s: This file is in LE "
					"(Win16 driver) format.\n", argv[na]);
				break;

			default:
				printf("%s: Unknown type.\n", argv[na]);
				break;
	
		}
		fclose(fp);
		printf("%s: %d fonts extracted\n", argv[na], count);
	}
	return 0;
}








