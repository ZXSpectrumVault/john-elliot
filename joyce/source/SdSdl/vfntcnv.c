/*************************************************************
 * vfntobj.c
 * create a ".c" file that can be compiled into a binary 
 * image of a GEM font
 *
 * 31-12-2000 John Elliott
 * 	Rewritten for UNIX and C
 *
 * 11-16-87 jn
 *	MOdified to always include the 8 point font file.
 *	Also all the details are now taken care of.
 *
 *	USAGE: vfntcnv <ascii data file> <header file> <outfile>
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>
#include <stdarg.h>

#define	CHRBM_SIZE 32

void error(char *s, ...) ;
void emit_cmt() ;
void emit_hdr() ;
void emit_wtbl() ;
void emit_fnt() ;
void init_hdr() ;
void open_files() ;
void open_8pt(int argc, char **argv, FILE **out_file, FILE **font_file, FILE **hdr_file);
void open_10pt(int argc, char **argv, FILE **out_file, FILE **font_file, FILE **hdr_file);
void mkchrbm(void) ;
void mkfntbm(Uint32 font_size, int bytes_pl);

int	font_height,
	form_width,
	font_count,
	char_width ;

FILE	*font_file, *out_file, *hdr_file ;

Uint8	*font_buff ;
Uint8	chrbm[CHRBM_SIZE] ;
Sint32	chrbmw ;

Uint8	pfont_name[15];
char	hdr_name[15];
char	fname[33] ;
char	pfont_line[128];
char	pbuffer[128];

Uint8	bitmsk[] = { 0x80, 0x40, 0x20, 0x10,
		     0x08, 0x04, 0x02, 0x01 } ;

Uint32		offsets[257];

const char *hdrlbs[] = {
	"Font ID",
	"Font Point size",
	"Font Name, pad",
	"Lowest ADE",
	"Highest ADE",
	"Top line distance",
	"Ascent line distance",
	"Half line distance",
	"Desent line distance",
	"Bottom line distance",
	"Width of widest char in font",
	"Width of widest char cell in face",
	"Left offset",
	"Right offset",
	"Thickening",
	"Underline size",
	"Lightening mask",
	"Skewing mask",
	"Flags",
	"Long ptr to horz offset table",
	"Long ptr to char offset table",
	"Long ptr to font data",
	"Form width",
	"Form height",
	"Long ptr, next font header",
	"Long ptr, next section of font",
	"Least frequently used counter low word",
	"Least frequently used counter high word",
	"LONG word, offset in file to offset table",
	"Data Size, length of horz offset table",
	"7 reserved words",
	"Device Flags",
	"32 byte escape sequence buffer",
	"" } ;

const char *restbl[] = {
	"0",
	"0",
	"0",
	"0",
	"0",
	"0, 0, 0, 0, 0, 0, 0 ",
	"0",
	"0,0,0,0,0,0,0,0",
	"" } ;

Uint32 hdrval[] = {
	1,	/* Font ID */
	6,	/* Font Point size */
	32,	/* Font Name, number of bytes to reserver */
	32,	/* Lowest ADE */
	127,	/* Highest ADE */
	5,	/* Top line distance */
	5,	/* Ascent line distance */
	4,	/* Half line distance */
	1,	/* Desent line distance */
	1,	/* Bottom line distance */
	5,	/* Width of widest char in font */
	6,	/* Width of widest char cell in face */
	1,	/* Left offset */
	0,	/* Right offset */
	1,	/* Thickening */
	1,	/* Underline size */
	0x5555,	/* Lightening mask */
	0x5555,	/* Skewing mask */
	0x08,	/* Flags */
	0,	/* Long ptr to horz offset table */
	0,	/* Long ptr to char offset table */
	0,	/* Long ptr to font data */
	72,	/* Form width */
	6,	/* Form height */
	0,	/* Long ptr to next font header */
	 } ;






/*--------------
 * main ....
 *------------*/
int main(int argc, char **argv)
{
Uint32	i, j, c ;
Uint32	nitems;
char	*pline;
Uint32	bytes_pl,
	words_pl,
	font_size ;
Uint32	hdr_8pt, wtbl_8pt, font_8pt, end_8pt,
	hdr_10pt, wtbl_10pt, font_10pt, end_10pt ;
FILE	*in8pt_file, *in10pt_file ;


	printf("Font Conversion Utility\n") ;
	printf("Date: 18-04-2005\n\n") ;

	printf("Processing 8 point system font\n") ;

	open_8pt(argc, argv, &out_file, &font_file, &hdr_file) ;

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%s", pfont_name);

	/* get count of items in font */

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &font_count);

	/* get height of font	*/

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &font_height);

	/* get width of entire strike form */

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &form_width);

	char_width = form_width / font_count ;

	bytes_pl = form_width / 8 ;
	words_pl = (bytes_pl + 1) / 2 ;
	bytes_pl = words_pl * 2 ;
	font_size = bytes_pl * (font_height) ;

	mkfntbm(font_size, bytes_pl) ;

	init_hdr() ;
	rewind(out_file) ;
	hdr_8pt = ftell(out_file) ;
	emit_cmt() ;
/*	emit_hdr(1, argv[1]) ; */
	wtbl_8pt = ftell(out_file) ;
	emit_wtbl() ;
	font_8pt = ftell(out_file) ;
	emit_fnt(font_size, bytes_pl) ;
	emit_hdr(1, argv[1]) ; 
	end_8pt =  ftell(out_file) ;

	/* close source file */

	fclose(out_file) ;
	fclose(font_file) ;
	fclose(hdr_file) ;

	printf("Processing 10 point system font\n") ;

	open_10pt(argc, argv, &out_file, &font_file, &hdr_file) ;

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%s", pfont_name);
	
	/* get count of items in font */

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &font_count);

	/* get height of font	*/

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &font_height);

	/* get width of entire strike form */

	pline = fgets(pbuffer, 82, font_file);
	nitems = sscanf(pline, "%d", &form_width);

	char_width = form_width / font_count ;

	bytes_pl = form_width / 8 ;
	words_pl = (bytes_pl + 1) / 2 ;
	bytes_pl = words_pl * 2 ;
	font_size = bytes_pl * (font_height) ;

	mkfntbm(font_size, bytes_pl) ;
	init_hdr() ;
	rewind(out_file) ;
	hdr_10pt = ftell(out_file) ;
	emit_cmt() ;
/*	emit_hdr(0, "") ; */
	wtbl_10pt = ftell(out_file) ;
	emit_wtbl() ;
	font_10pt = ftell(out_file) ;
	emit_fnt(font_size, bytes_pl) ;
	emit_hdr(0, "") ; 
	end_10pt =  ftell(out_file) ;

	/* close source file */

	fprintf(out_file,"\032");

	fclose(out_file) ;
	fclose(font_file) ;
	fclose(hdr_file) ;

	/* open both temp files as input,
	   and the output file */

	printf("Merging 8 and 10 point files\n") ;

	in8pt_file = fopen("8point.tmp", "rb") ;
	in10pt_file = fopen("10point.tmp", "rb") ;

	rewind(in8pt_file) ;
	rewind(in10pt_file) ;

	out_file = fopen(argv[5], "w") ;
	if (out_file == NULL)
	  error("Output file open error: %s\n", argv[5]) ;

	fprintf(out_file, "// THIS IS A GENERATED FILE! DO NOT EDIT!\n\n"
			  "#include \"sdsdl.hxx\"\n\n") ;

	j = wtbl_8pt - hdr_8pt ;
	fseek(in8pt_file, hdr_8pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in8pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	j = wtbl_10pt - hdr_10pt ;
	fseek(in10pt_file, hdr_10pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in10pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	j = font_8pt - wtbl_8pt ;
	fseek(in8pt_file, wtbl_8pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in8pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	j = font_10pt - wtbl_10pt ;
	fseek(in10pt_file, wtbl_10pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in10pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	j = end_8pt - font_8pt ;
	fseek(in8pt_file, font_8pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in8pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	j = end_10pt - font_10pt ;
	fseek(in10pt_file, font_10pt, SEEK_SET) ;
	for ( i = 0 ; i < j ; i++) {
	  c = fgetc(in10pt_file) ;
	  c &= 0xff ;
	  fputc(c, out_file) ;
	}

	fclose(in8pt_file) ;
	fclose(in10pt_file) ;
	fclose(out_file) ;

	exit(0) ;


}



/*----------------------------
 * emit the comment header....
 *---------------------------*/
void emit_cmt()
{

	fprintf(out_file, "/*----------------------------------------*\n") ;
	fprintf(out_file,
	  " *                    Font Name: %s\n", pfont_name) ;
	fprintf(out_file,
	  " * Number of characters in font: %d\n", font_count) ;
	fprintf(out_file,
	  " * Height of characters in font: %d\n", font_height) ;
	fprintf(out_file,
	  " *   Width of character in font: %d\n", char_width) ;
	fprintf(out_file, " *----------------------------------------*/\n\n") ;

  #if 0
	rfp = fopen("rules.dat", "r") ;
	if (rfp == NULL) {
	  fprintf(stderr, "Rules file not found\n") ;
	  return ;
	}

	while(!feof(rfp)) {
	  fgets(pfont_line, 82, rfp) ;
	  if (!feof(rfp))
	    fprintf(out_file, "\t; %s", pfont_line) ;
	}

	fclose(rfp) ;
#endif
}



/*---------------------
 * emit the font header
 *-------------------*/
void emit_hdr(pt8, pt10_name)
Sint32	pt8 ;
char	*pt10_name ;
{
Sint32	i, j, li ;
char	name[20] ;
char *c;

/*        fprintf(out_file, "static Uint32 %s_wtbl[];\n", pfont_name) ;

        fprintf(out_file, "static Uint8 %s_data[];\n", pfont_name); */
        if (pt8 == 1)
        {
	  c = strchr(pt10_name, '/');
	  if (c)
	  {
		pt10_name = c + 1;	
	  }
          j = strlen(pt10_name) ;
          for (i = 0 ; i < j ; i++ )
          {
            if (pt10_name[i] == '.') { name[i] = 0 ; break ; }
            else name[i] = pt10_name[i] ;
          }
          fprintf(out_file, "extern FontHead fnt%s_hdr;\n", name) ;
          li++ ;
        }


	fprintf(out_file, "/*----------------------------------------*\n") ;
	fprintf(out_file, " * Ventura Style Font header              *\n") ;
	if (pt8 == 1) 
	  fprintf(out_file, " * 8 point system font header             *\n") ;
	
	else {
	  fprintf(out_file, " * 10 point system font header            *\n") ;
	}
	fprintf(out_file, " *----------------------------------------*/\n\n") ;

	/* At this time just emit a default header,
	   At a latter date use the font header file
	   to construct specific header */

	if (pt8 == 1) 
	     fprintf(out_file, "FontHead fntFirst = {\n") ;
	else fprintf(out_file, "FontHead %s_hdr = {\n", pfont_name) ;

	for ( li = 0 ; li < 2 ; li++) {
	  fprintf(out_file, "\t%d,\t// %s\n", hdrval[li], hdrlbs[li]) ;
	}

	i = strlen(fname) ;
	fprintf(out_file, "\tL\"%s\", \n", fname ) ;
	li++;

	for ( ; li <  16 ; li++) {
	  fprintf(out_file, "\t%d,\t// %s\n", hdrval[li], hdrlbs[li]) ;
	}

	for ( ; li <  19 ; li++) {
	  fprintf(out_file, "\t0x%04x, \t// %s\n", hdrval[li], hdrlbs[li]) ;
	}

	fprintf(out_file, "\tNULL, \t// %s\n", 
	  hdrlbs[li++]) ;

	fprintf(out_file, "\t%s_wtbl,\t// %s\n", pfont_name, hdrlbs[li++]) ;

	fprintf(out_file, "\t%s_data,\t// %s\n", pfont_name, hdrlbs[li++]) ;

	for ( ; li <  24 ; li++) {
	  fprintf(out_file, "\t%d,\t// %s\n", hdrval[li], hdrlbs[li]) ;
	}

	if (pt8 == 1) 
        { 
	  j = strlen(pt10_name) ;
	  for (i = 0 ; i < j ; i++ ) 
          {
	    if (pt10_name[i] == '.') { name[i] = 0 ; break ; }
	    else name[i] = pt10_name[i] ;
	  }
	  fprintf(out_file, "\t&fnt%s_hdr,\t// %s\n", name, hdrlbs[li] ) ;
	  li++ ;
	}
	else {
	  fprintf(out_file, "\tNULL,\t// %s\n", hdrlbs[li]) ;
	  li++ ;
	}

	fprintf(out_file, "};\n\n") ;

}



/*---------------------
 * emit the width table
 *---------------------*/
void emit_wtbl()
{
Sint32	i ;


	fprintf(out_file, "\n\n") ;

	fprintf(out_file, "/*----------------------------------------*\n") ;
	fprintf(out_file, " * Font width table                       *\n") ;
	fprintf(out_file, " *----------------------------------------*/\n\n") ;

	fprintf(out_file, "static Uint32 %s_wtbl[] = {\n\n", pfont_name) ;

	for ( i = 0 ; i < font_count + 1 ; i++ ) {
	  if (i % 8 == 0) {
	    fprintf(out_file, "\n\t%d, ", i * char_width) ;
	  }
	  else {
	    fprintf(out_file, "%d, ", i * char_width) ;
	  }
	}
	fprintf(out_file, "};\n\n") ;

#if 0
	for ( i = 0 ; i < font_count + 1 ; i++ ) {
	  fprintf(out_file, "\tdw\t%d\t\t; ADE %03xh, ascii: %c\n",
	    i * char_width, i + hdrval[3], (isalnum(i)) ? i : '.') ;
	}
#endif

}




/*------------------------------------------
 * emit the .A86 data structure for the font
 * put a comment out for each line.
 *-----------------------------------------*/
void emit_fnt(font_size, bytes_pl)
Sint32	font_size, bytes_pl ;
{
Sint32	i ;

	fprintf(out_file, "/*----------------------------------------*\n") ;
	fprintf(out_file, " * Font data                              *\n") ;
	fprintf(out_file, " *----------------------------------------*/\n\n") ;

	fprintf(out_file, "static Uint8 %s_data[] = {\n\n", pfont_name) ;

	for (i = 0 ; i < font_size ; i++ ) {
	  if (i % bytes_pl == 0) {
	    fprintf(out_file,"\t// Line %d\n", i / bytes_pl) ;
	  }
	  if (i % 8 == 0) {
	    fprintf(out_file,"\t") ;
	  }
	  if (i % 8 != 7) {
	    fprintf(out_file, "0x%03x, ", font_buff[i]) ;
	  }
	  else {
	    fprintf(out_file, "0x%03x, \n", font_buff[i]) ;
	  }
	}
	fprintf(out_file, "};\n");
}



/*--------------------------
 * init the font header info
 *--------------------------*/
void init_hdr()
{
Sint32	i, ret ;

	for (i = 0 ; i < 2 ; i++) {
	  ret = fscanf(hdr_file, "%d", &hdrval[i]) ;
	  if (ret == -1) {
	    printf("Error on item: %d of header file\n", i) ;
	  }
	}

	ret = fscanf(hdr_file, "%s", fname) ;
	if (ret == -1) {
	  printf("Error on name in header file\n") ;
	}
	i = strlen(fname) ;

	hdrval[2] = 32 - i ;

	for ( i = 3 ; i < 16 ; i++ ) {
	  ret = fscanf(hdr_file, "%d", &hdrval[i]) ;
	  if (ret <= 0) {
	    printf("Error on item: %d of header file\n", i) ;
	  }
	}

	for ( i = 16 ; i < 19 ; i++ ) {
	  ret = fscanf(hdr_file, "%x", &hdrval[i]) ;
	  if (ret <= 0) {
	    printf("Error on item: %d of header file\n", i) ;
	  }
	}

	for ( i = 22 ; i < 24 ; i++ ) {
	  ret = fscanf(hdr_file, "%d", &hdrval[i]) ;
	  if (ret <= 0) {
	    printf("Error on item: %d of header file\n", i) ;
	  }
	}

}




/*------------------
 * make font bit map
 *-----------------*/
void mkfntbm(Uint32 font_size, int bytes_pl) 
{
Uint32	srcx,
	destx,
	y,
	cur_char ;


	/* allocate memory for the font BM */

	font_buff = (Uint8 *) calloc(font_size + 16, sizeof(char)) ;
	if (font_buff == NULL) {
	  error("Memory Allocation error: %d bytes\n", font_size) ;
	}
	
	/* Initialize the dest X.
	   This will be common over all the chars */

	destx = 0 ;

	for (cur_char = 0 ; cur_char < font_count ; cur_char++) {
	  mkchrbm() ;
	  for ( srcx = 0 ; srcx < char_width ; srcx++, destx++ ) {
	    for ( y = 0 ; y < font_height ; y++ ) {
	      if (chrbm[(y * chrbmw) + (srcx >> 3)] & bitmsk[srcx & 0x07]) {
	        font_buff[(y*bytes_pl)+(destx >> 3)] |= bitmsk[destx & 0x07];
	      }
	    }
	  }
	}
}



/*-------------------------------------
 * make a character bit map
 * return a left justified char bit map
 * in chrbm
 *-----------------------------------*/
void mkchrbm()
{
	Uint32	i, x, y ;

	/* the char bm is assumed to 16 bits wide.
	   It's max height is 16 pixels.
	   Clear the char bitmap. */
	
	chrbmw = 2 ;

	for ( i = 0 ; i < CHRBM_SIZE ; i++ )
	  chrbm[i] = 0 ;

	/* read the first ID.
	   This will go into the bit bucket */

	if (!fgets(pfont_line, 82, font_file))
	{
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	}

	for ( y = 0 ; y < font_height ; y++ ) 
	{
	  if (!fgets(pfont_line, 82, font_file))
	  {
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	  }
	  for ( x = 0 ; x < char_width ; x++) {
	    if (pfont_line[x] == '1') {
	      chrbm[(y * 2) + (x / 8)] |= bitmsk[x % 8] ;
	    }
	  }
	}

	


}


/*--------------------------------------------
 * open the 10 point font and the output files
 *------------------------------------------*/
void open_10pt(int argc, char **argv, FILE **out_file, 
		FILE **font_file, FILE **hdr_file)
{

	/* open font file */

	*font_file = fopen(argv[1],"r");
	if (*font_file == NULL)
	  error("Font input file open error: %s\n", argv[1]) ;

	/* open header file */

	strcpy(hdr_name, argv[2]) ;
	*hdr_file = fopen(argv[2],"r");
	if (*hdr_file == NULL)
	  error("Header input file open error: %s\n", argv[2]) ;

	/* open output file */

	*out_file = fopen("10point.tmp", "w") ;
	if (*out_file == NULL)
	  error("Output file open error: %s\n", "10point.tmp") ;

}

/*-------------------------------------------
 * open the 8 point font and the output files
 *------------------------------------------*/
void open_8pt(int argc, char **argv,
		FILE **out_file, FILE **font_file, FILE **hdr_file)
{

	/* open font file */

	*font_file = fopen(argv[3], "r");
	if (*font_file == NULL)
	  error("Font input file open error: %s\n", argv[3]);

	/* open header file */

	strcpy(hdr_name, argv[4]);
	*hdr_file = fopen(argv[4],"r");
	if (*hdr_file == NULL)
	  error("Header input file open error: %s\n", argv[4]) ;

	/* open output file */

	*out_file = fopen("8point.tmp", "w") ;
	if (*out_file == NULL)
	  error("Output file open error: %s\n", "8point.tmp") ;

}

/*----------------
 * error handler.
 *---------------*/
void error(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	
	vfprintf(stderr, s, ap) ;
	exit(1) ;
	va_end(ap);
}
