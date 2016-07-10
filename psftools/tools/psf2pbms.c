
#include "cnvshell.h"
#include "psflib.h"

char *cnv_progname = "PSF2PNMS";


char *cnv_set_option(int ddash, char *variable, char *value)
    {
    return "This program does not have options.";
    }

char *cnv_help(void)
    {
    static char buf[1250];

    sprintf(buf, "Syntax: %s psf_file\n\n"
		 "This program will convert the PSF file into a separate\n"
		 "Portable BitMap (.pbm) file for each character.\n", 
		cnv_progname);
    return buf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	PSF_FILE psf;
	int x,y, m;
	psf_dword count, max;
	psf_byte pix;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);	

	if (rv != PSF_E_OK) return psf_error_string(rv);

	max = psf.psf_length;

	for (count = 0; count < max; count++)
	{
		FILE *fp;
		char buf[50];

		sprintf(buf, "psf%ld.pbm", count);
		fp = fopen(buf, "w");
		
		fprintf(fp, "P1\n%ld\n%ld\n", psf.psf_width,psf.psf_height);	
		for (y = 0; y < psf.psf_height; y++)
		{
			for (x = 0; x < psf.psf_width; x++)
			{
				psf_get_pixel(&psf, count, x, y, &pix);
				if (pix) fputc('1', fp);
				else fputc('0', fp);
				m = m >> 1;
			}
			fputc('\n', fp);
		}	
		fclose(fp);
	}
	psf_file_delete(&psf);
	return NULL;
}

