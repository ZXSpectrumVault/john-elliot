
#ifndef ZXFLIB_H_INCLUDED

#define ZXFLIB_H_INCLUDED

#define ZXF_MAGIC "PLUS3DOS\032\001"	/* +3DOS magic number */

typedef unsigned char  zxf_byte;
typedef unsigned short zxf_word;

typedef int zxf_errno_t;


typedef enum
{
	ZXF_NAKED,
	ZXF_P3DOS,
	ZXF_TAP,
	ZXF_SNA,
	ZXF_ROM
} ZXF_FORMAT;


typedef struct
{
	ZXF_FORMAT zxf_format;
	zxf_byte zxf_header[128];
	zxf_byte zxf_data[768];
	zxf_byte zxf_udgs[168];
} ZXF_FILE;

/* Initialise the structure as empty */
void zxf_file_new(ZXF_FILE *f);
/* Initialise the structure for a new font*/
zxf_errno_t zxf_file_create(ZXF_FILE *f);
/* Load a font from a stream 
 * nb: If the file pointer is important to you, you have to save it. */
zxf_errno_t zxf_file_read  (ZXF_FILE *f, FILE *fp, ZXF_FORMAT fmt);
/* Write font to stream. If format is ROM or SNA, auxname must be set. */
zxf_errno_t zxf_file_write (ZXF_FILE *f, FILE *fp, ZXF_FORMAT fmt, char *auxname);
/* Free any memory associated with a Spectrum font file (dummy) */
void zxf_file_delete (ZXF_FILE *f);
/* Expand error number returned by other routines, to string */
char *zxf_error_string(zxf_errno_t err);

/* errors */
#define ZXF_E_OK        ( 0)	/* OK */
#define ZXF_E_NOTP3FNT	(-3)	/* Attempt to load incorrect file */
#define ZXF_E_ERRNO	(-4)	/* Error in fread() or fwrite() */
#define ZXF_E_EMPTY	(-5)	/* Attempt to save an empty file */
#endif /* ZXFLIB_H_INCLUDED */

