/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003  John Elliott

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
*/

/* Attempt the black magic of converting one or more .FNT files to a .FON 
 * file. This trick is normally done using various tools from the Windows 
 * 3.1 DDK (such as MASM, RC and LINK) and writing customised .RC and .DEF 
 * scripts. 
 *
 * See <http://support.microsoft.com/support/kb/articles/Q76/5/35.ASP>
 */

#include "cnvshell.h"
#include "psflib.h"
#include "mswfnt.h"

/* Systems with short command lines don't tend to be able to escape spaces 
 * in their argument lists.  */
#ifdef CPM
#define SHORT_CMDLINE 1
#endif

#ifdef __MSDOS__
#define SHORT_CMDLINE 1
#endif

#define MAXFCOUNT 20        /* Max. 20 fonts in a file. At the moment we only
                         * support doing one. */

static char helpbuf[2048];
static char msgbuf[100];
/* Message printed if someone tries to run the .FON file under DOS */
static char *mz_message = "This is a Windows font file.\r\n$";

static int align = 16;                       /* 16-byte alignment */
static int linkver  = 0x0400;                /* Linker 4.00 */
static int winver   = 0x030A;                /* Windows 3.10 */
static int fnt_count = 0;                    /* Number of FNTs */
static MSW_FONTINFO *msw;                    /* Font headers */
static msw_byte **msw_font;                  /* Font bodies */
static unsigned long ne_checksum = 0;        /* NE header checksum */
static unsigned short mz_checksum = 0;       /* MZ header checksum */
static unsigned short mz_extra = 0;          /* Length of last page in MZ stub */
static int stubtype = 0;                     /* Use alternative stub */

static char mn_str[20];
static char *modname = "FONTLIB";       /* Module name */
static char fres_str[200];              /* Module description */
static int resid_first = 101;		/* First resource ID */
static int resid_step  = 1;		/* Resource ID increment */ 
static int sysfont     = -1;		/* Font is system font? */
/* Calculate the lengths of various bits of the NE file we're generating */
static psf_dword mz_header_len(void);
static psf_dword mz_body_len(void);
static psf_dword ne_header_len(void); 
static psf_dword ne_res_table_len(void);
static psf_dword ne_entry_table_len(void);
static psf_dword ne_nonres_table_len(void);
static psf_dword ne_rname_table_len(void);

/* Calculate the offsets of various bits of the NE file */
/* mz_header_offset = 0 */
static psf_dword mz_body_offset(void);
static psf_dword ne_header_offset(void);
static psf_dword ne_res_table_offset(void);
static psf_dword ne_entry_table_offset(void);
static psf_dword ne_rname_table_offset(void);
static psf_dword ne_nonres_table_offset(void);
static psf_dword ne_fontdir_offset(void);




/* Align an address to the nearest "n" bytes, or to the global alignment 
 * if that's coarser-grained */
static psf_dword realign(psf_dword d, int w)
{
        psf_dword o;
        if (w < align ) w = align;

        o = ((d + (w-1)) & (~(w-1)));

        return o;
}

/* Write out various bits */

static int mz_write_header(FILE *fp)
{
        long mhl = mz_header_len();
        long mdl = mz_body_len() + mz_header_len() + mz_extra;
        psf_word sp = mz_body_len() - 4;
	psf_word ss = 0;
        int n;

	if (winver < 0x200) sp = 0;	/* In Windows 1.0, SP = 0 */
	if (winver >= 0x200 && winver < 0x300) 
	{
		ss = 4;	/* win2.0, SS = 4 */
		sp = 0x100;
	}

        if (write_byte(fp, 'M'))                return -1; /* MZ magic */
        if (write_byte(fp, 'Z'))                return -1;
        /* File size: Low word = size mod 512 */
        if (write_word(fp, mdl & 0x1FF))        return -1; /* MZ file size */
        /* High word = ceiling(size / 512) */
        if (write_word(fp, (mdl + 0x1FF) >> 9)) return -1;
        if (write_word(fp, 0))                  return -1; /* No relocation */
        if (write_word(fp, (mhl + 15)/16))      return -1; /* Header length */
        if (write_word(fp, 0))                  return -1; /* Min memory */
        if (write_word(fp, 0xFFFF))             return -1; /* Max memory */
        if (write_word(fp, ss))                 return -1; /* SS */
        if (write_word(fp, sp))                 return -1; /* SP */        
        if (write_word(fp, mz_checksum))        return -1; /* Checksum */
        if (write_dword(fp,0))                  return -1; /* CS:IP */
        if (write_dword(fp, 0x40))           return -1;        /* NE magic check */

        /* Padding */
        for (n = 0x1C; n < 0x3C; n+=2)
        {
                if (write_word(fp, 0))        return -1;        
        }
        if (write_dword(fp, ne_header_offset())) return -1;
        for (n = 0x40; n < mz_body_offset(); n++) 
        {
                if (write_byte(fp, 0)) return -1;
        }
        return 0;
}

static psf_dword mz_header_len(void)
{
	return realign(0x40, 0x40);
}


static psf_dword mz_body_offset(void)
{
        return mz_header_len();
}

static int mz_write_body(FILE *fp)
{
        int n;

	switch(stubtype)
	{
	    case 1: 
            if (write_byte(fp, 0xE8))   return -1;  
            if (write_word(fp, strlen(mz_message)))   return -1;  /* CALL 002E */

            for (n = 0; n < strlen(mz_message); n++)
            {
                if(write_byte(fp, mz_message[n])) return -1;
            }
            if (write_byte(fp, 0x5A))   return -1;  /* POP DX */
            if (write_byte(fp, 0x0E))   return -1;  /* PUSH CS */
            if (write_byte(fp, 0x1F))   return -1;  /* POP DS */
            if (write_byte(fp, 0xB4))   return -1;  /* MOV AH, */
            if (write_byte(fp, 0x09))   return -1;  /* 9 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
            if (write_byte(fp, 0xB8))   return -1;  /* MOV AX, */
            if (write_word(fp, 0x4C01)) return -1;  /* 0x4C01 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
            n += 0x0F;
            break;
        
            case 2:
            if (write_byte(fp, 0xE8))   return -1;  
            if (write_word(fp, 0x0B + strlen(mz_message)))   return -1;  /* CALL 002E */

            for (n = 0; n < strlen(mz_message); n++)
            {
                if(write_byte(fp, mz_message[n])) return -1;
            }
            if (write_byte(fp, 0x0E))   return -1;  /* PUSH CS */
            if (write_byte(fp, 0x1F))   return -1;  /* POP DS */
            if (write_byte(fp, 0xB4))   return -1;  /* MOV AH, */
            if (write_byte(fp, 0x09))   return -1;  /* 9 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
            if (write_byte(fp, 0xB8))   return -1;  /* MOV AX, */
            if (write_word(fp, 0x4C01)) return -1;  /* 0x4C01 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
            if (write_byte(fp, 0x5A))   return -1;  /* POP DX */
            if (write_word(fp, 0xF2EB)) return -1;  /* JMPS justaftermessage */
            n += 0x11;
            break;
        


            default:
            if (write_byte(fp, 0x0E))   return -1;  /* PUSH CS */
            if (write_byte(fp, 0x1F))   return -1;  /* POP DS */
            if (write_byte(fp, 0xBA))   return -1;  /* MOV DX, */
            if (write_word(fp, 0x0E))   return -1;  /* OFFSET MESSAGE */
            if (write_byte(fp, 0xB4))   return -1;  /* MOV AH, */
            if (write_byte(fp, 0x09))   return -1;  /* 9 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
            if (write_byte(fp, 0xB8))   return -1;  /* MOV AX, */
            if (write_word(fp, 0x4C01)) return -1;  /* 0x4C01 */
            if (write_word(fp, 0x21CD)) return -1;  /* INT 0x21 */
        
            for (n = 0; n < strlen(mz_message); n++)
            {
                if(write_byte(fp, mz_message[n])) return -1;
            }
            n += 0x0E;
            break;
        }

        n += mz_header_len();
        for (; n < ne_header_offset(); n++)
        {
                if (write_byte(fp, 0)) return -1;
        }
        return 0;
}

static psf_dword mz_body_len(void)
{
        psf_dword l;

	switch(stubtype)
	{
		case 1:  l = 0x0F + strlen(mz_message); break;
		case 2:  l = 0x11 + strlen(mz_message); break;
		default: l = 0x0E + strlen(mz_message); break;
	}
        return realign(l, 0x40);
}

static psf_dword ne_header_offset(void)
{
        return mz_body_offset() + mz_body_len();
}

static psf_dword ne_header_len(void)
{
        return 0x40;        /* Constant */
}


static int ne_write_ne_header(FILE *fp)
{
        psf_dword ne_base = ne_header_offset();
        int lalign;
        
        lalign = 1;
        while ((1 << lalign) < align) ++lalign;        

        if (write_byte(fp, 'N')) return -1;            /* 00 */
        if (write_byte(fp, 'E')) return -1;            /* 01 */
        if (write_byte(fp, linkver >> 8)) return -1;   /* 02 linker major */
        if (write_byte(fp, linkver & 0xFF)) return -1; /* 03 linker minor */
        if (write_word(fp, ne_entry_table_offset() - ne_base)) return -1;/*04*/
        if (write_word(fp, ne_entry_table_len())) return -1; /* 06 */
        if (write_dword(fp, ne_checksum))  return -1;            /* 08 CRC */
	if (winver < 0x200)	/* Windows 1.01 exe flags */
        {
            if (write_word(fp, 0x8004)) return -1;     /* 0C DLL | global init */
        }
        else 
        {
            if (write_word(fp, 0x8300)) return -1;     /* 0C DLL | requires GUI */
        }
        if (write_word(fp, 0))   return -1;            /* 0E Data segment no. */
        if (write_word(fp, 0))   return -1;            /* 10 Heap size */
        if (write_word(fp, 0))   return -1;            /* 12 Stack size */
        if (write_dword(fp, 0))  return -1;            /* 14 CS:IP */
        if (write_dword(fp, 0))  return -1;            /* 18 SS:SP */
        if (write_word(fp, 0))   return -1;            /* 1C Segments */
        if (write_word(fp, 0))   return -1;            /* 1E Module references */
        if (write_word(fp, ne_nonres_table_len())) return -1; /* 20 */
              /* 22 Segment table address: vacuous */ 
        if (write_word(fp, ne_res_table_offset()   - ne_base)) return -1;
        /* 24 Resource table */
        if (write_word(fp, ne_res_table_offset()   - ne_base)) return -1;
        /* 26 Resident-name table */
        if (write_word(fp, ne_rname_table_offset() - ne_base)) return -1;
        /* 28 Module-reference table */
        if (write_word(fp, ne_entry_table_offset() - ne_base)) return -1;
        /* 2A Imported-name table */
        if (write_word(fp, ne_entry_table_offset() - ne_base)) return -1;
        /* 2C Nonresident-name table */
        if (write_dword(fp, ne_nonres_table_offset() )) return -1;
        if (write_word(fp, 0)) return -1;            /* 30 No entry points */
        if (write_word(fp, lalign)) return -1;       /* 32 Alignment */
        if (winver < 0x200)
        {
            if (write_dword(fp, 0)) return -1;       /* 34 Unused? */
            if (write_dword(fp, 0)) return -1;       /* 38 Unused? */
            if (write_dword(fp, 0)) return -1;       /* 3C Unused? */
        }
        else
        {
            if (write_word(fp, 0)) return -1;        /* 34 Resource segments */
            if (write_byte(fp, 2)) return -1;        /* 36 Windows OS */
            if (write_byte(fp, 8)) return -1;        /* 37 Contains fast-load area */
            /* XXX Work what this refers to */
            if (write_word(fp, 0x16)) return -1;     /* 38 Fast-load area address */
            if (write_word(fp, 0x0E)) return -1;     /* 3A Length of fast-load area*/
            if (write_word(fp, 0)) return -1;        /* 3C Reserved */
            if (write_word(fp, winver)) return -1;   /* 3E Expected Windows ver. */
        }
        return 0;        
}

static psf_dword ne_res_table_offset(void)
{
        return ne_header_offset() + ne_header_len();
}


static psf_dword ne_res_table_len(void)
{
        /* Size is:  2 bytes header
                  + 20 bytes for FONTDIR data
                  +  8 bytes for FONT typeinfo
                  + 12 bytes/font
                        + 20 bytes if there's a VS_VERSION_INFO; currently
                      this is not supported. 
                  +  2 bytes trailer
                  +  8 bytes "\007FONTDIR"
         */
        return 40 + (12 * fnt_count);
}

static psf_dword ne_rname_table_offset(void)
{
        return ne_res_table_offset() + ne_res_table_len();
}

static int ne_write_rname_table(FILE *fp)
{
        int fpos;

        if (write_byte(fp, strlen(modname))) return -1;
        if (fputs(modname, fp) == EOF) return -1;        
        fpos = 1 + strlen(modname);
        while (fpos < ne_rname_table_len())
        {
                if (write_byte(fp, 0)) return -1;
                ++fpos;
        }
        return 0;
}



static psf_dword ne_rname_table_len(void)
{
	/* DB string length
	 * DB string
	 * DW 0	;Ordinal
	 * DB 0 ;End of table */
        psf_dword len = 4 + strlen(modname);        

        return len;
}

static psf_dword ne_entry_table_offset(void)
{
        return ne_rname_table_offset() + ne_rname_table_len();
}

static psf_dword ne_entry_table_len(void)
{
        return 2;
}

static int ne_write_entry_table(FILE *fp)
{
        return write_word(fp, 0);
}

static psf_dword ne_nonres_table_offset(void)
{
        return ne_entry_table_offset() + ne_entry_table_len();
}

static psf_dword ne_nonres_table_len(void)
{
        psf_dword len = 3 + strlen(fres_str);        
        if (winver < 0x300 || (len & 1)) len++;
        return len;
}

static int ne_write_nonres_table(FILE *fp)
{
        int fpos;
        int fnext;

        fnext = ne_fontdir_offset() - ne_nonres_table_offset();
                        /* Fill space up to first FONTDIR */

        if (write_byte(fp, strlen(fres_str))) return -1;
        if (fputs(fres_str, fp) == EOF) return -1;
        fpos = 1 + strlen(fres_str);
        while (fpos < /*ne_nonres_table_len()*/ fnext)
        {
                if (write_byte(fp, 0)) return -1;
                ++fpos;
        }
        return 0;
}




/* Find the length of a fontdir entry for a given font */
static int ne_fontdir_len(int font)
{
        int dnamelen = 1, fnamelen = 1;
        int len;

	/* Add 1 to dnamelen/fnamelen, for terminating zeroes */
        if (msw[font].dfFace)   fnamelen += msw[font].dfSize - msw[font].dfFace;
        if (msw[font].dfDevice) dnamelen += msw[font].dfFace - msw[font].dfDevice;

        len = 0x72 + dnamelen + fnamelen;

        return len;
}

/* Find the address of the first fontdir */
static psf_dword ne_fontdir_offset(void)
{
        psf_dword prev_end = ne_nonres_table_offset() + ne_nonres_table_len();
      
	/* Strange alignment biz */ 
	if (winver < 0x300) return (prev_end + 15) & (~15);
        return (prev_end + 127) & (~127);
}

static psf_dword ne_fontdir_addr(int font)
{
        long base = ne_fontdir_offset();
        int n;
	
	base += 2;	/* Add 2 for "count" word */
        for (n = 0; n < font; n++) 
	{
		base += ne_fontdir_len(n);
	}

        return base;
}

/* Find the address of the first fontdir */
static psf_dword ne_font_offset(void)
{
        psf_dword prev_end = ne_fontdir_addr(fnt_count);
      
	return (prev_end + 15) & (~15);
}


static psf_dword ne_font_addr(int font)
{
        int n;
        long base = ne_font_offset();

        /* XXX In my model, the Windows 3.x CGA80WOA.FON, the FONTDIR 
         *     resource is at 0180h and length 80h. The next resource in 
         *     the file is the FONT resource... at 0240h! */
        if (winver >= 0x300) base += 64;

        for (n = 0; n < font; n++) 
        {
                base += ((msw[n].dfSize + 15) & ~15); 
        }
        return base;
}



static int ne_write_res_table(FILE *fp)
{
        /* NE segment table omitted (blank) */
        int n;
	unsigned flags  = 0x50;	/* PRELOAD | MOVEABLE */
	unsigned fflags = 0, sflags = 0;
        unsigned resid  = resid_first | 0x8000;

	if (winver < 0x200) 
        {
                sflags = 0x60;	        /* Flags for SYSTEM font */
		fflags = 0xf030;	/* Flags for other fonts */
        }
        else
        {
		flags  |= 0x0C00;	/* ? */
                fflags = 0x1C30;	/* MOVEABLE | ?? */
		sflags = 0x1C30;
        }
	

        if (write_word(fp,  4)) return -1;           /* Aligned to 16 bytes */

        if (write_word(fp,  0x8007)) return -1;  /* RT_FONTDIR */
        if (write_word(fp,  1)) return -1;       /* One fontdir */
        if (write_dword(fp, 0)) return -1;       /* Reserved */
        if (write_word(fp, ne_fontdir_offset() / 16)) return -1;
                                /* (File offset of fontdir) / 16 */
        if (write_word(fp, (ne_fontdir_addr(fnt_count) - 
                                ne_fontdir_offset() + 15)/16)) return -1;
                                /* Fontdir length / 16 */
        if (write_word(fp, flags)) return -1; 
        if (write_word(fp, ne_res_table_len() - 8)) return -1;   /* ID at res_table + 40h */
        if (write_word(fp, 0x00)) return -1;   /* Handle */
        if (write_word(fp, 0x00)) return -1;   /* Usage  */

        if (write_word(fp, 0x8008)) return -1;        /* RT_FONT */
        if (write_word(fp, fnt_count)) return -1;
        if (write_dword(fp, 0)) return -1;
        for (n = 0; n < fnt_count; n++)
        {
                        /* (File offset of font) / 16 */        
            if (write_word(fp, ne_font_addr(n) / 16)) return -1;   
                        /* Resource length / 16 */
            if (write_word(fp, (msw[n].dfSize + 15) / 16)) return -1;
            if (write_word(fp, (n == sysfont) ? sflags : fflags)) return -1; 
            if (write_word(fp, resid)) return -1; /* ID */
            if (write_word(fp, 0x00)) return -1;   /* Handle */
            if (write_word(fp, 0x00)) return -1;   /* Usage */
            resid += resid_step;
        }
        /* dummy version info
        write_word(fp, 0x8010);
        write_word(fp, 1);
        write_dword(fp, 0);
        write_word(fp, 0xEC);
        write_word(fp, 0x21);
        write_word(fp, 0x0C30);
        write_word(fp, 0x8001);
        write_dword(fp, 0);
        */

        if (write_word(fp, 0)) return -1;        /* End of resource names */
        /* "FONTDIR" */
        if (fputs("\007FONTDIR", fp) == EOF) return -1;

        /* Resource table ends */
        return 0;
}


static int ne_write_fontdirs(FILE *fp)
{
        int n;
	int ordinal;
        long fontdirend, fontbeg;

        ordinal = resid_first;
        if (write_word (fp, fnt_count)) return -1;        /* Count of fonts */
        for (n = 0; n < fnt_count; n++)
        {
                MSW_FONTINFO *f = &msw[n];

                if (write_word (fp, ordinal)) return -1;  /* Ordinal */
                if (write_word (fp, f->dfVersion)) return -1; /* Version */
                if (write_dword(fp, f->dfSize)) return -1;
                if (fwrite(f->dfCopyright, 1, 60, fp) < 60) return -1;
                if (write_word (fp, f->dfType))            return -1;
                if (write_word (fp, f->dfPoints))          return -1;
                if (write_word (fp, f->dfVertRes))         return -1;
                if (write_word (fp, f->dfHorizRes))        return -1;
                if (write_word (fp, f->dfAscent))          return -1;
                if (write_word (fp, f->dfInternalLeading)) return -1;
                if (write_word (fp, f->dfExternalLeading)) return -1;
                if (write_byte (fp, f->dfItalic))          return -1;
                if (write_byte (fp, f->dfUnderline))       return -1;
                if (write_byte (fp, f->dfStrikeOut))       return -1;
                if (write_word (fp, f->dfWeight))          return -1;
                if (write_byte (fp, f->dfCharSet))         return -1;
                if (write_word (fp, f->dfPixWidth))        return -1;
                if (write_word (fp, f->dfPixHeight))       return -1;
                if (write_byte (fp, f->dfPitchAndFamily))  return -1;
                if (write_word (fp, f->dfAvgWidth))        return -1;
                if (write_word (fp, f->dfMaxWidth))        return -1;
                if (write_byte (fp, f->dfFirstChar))       return -1;
                if (write_byte (fp, f->dfLastChar))        return -1;
                if (write_byte (fp, f->dfDefaultChar))     return -1;
                if (write_byte (fp, f->dfBreakChar))       return -1;
                if (write_word (fp, f->dfWidthBytes))      return -1;
                if (write_dword(fp, f->dfDevice))          return -1;
                if (write_dword(fp, f->dfFace))            return -1;
                if (write_dword(fp, f->dfReserved))        return -1;
/* Write device name & face name */
                if (fputs((char *)msw_font[n] + f->dfDevice, fp) == EOF) 
                        return -1;
                if (write_byte(fp, 0)) return -1;
                if (fputs((char *)msw_font[n] + f->dfFace,   fp) == EOF) 
                        return -1;
                if (write_byte(fp, 0)) return -1;
                ordinal += resid_step;
        }
/* Pack from end of fontdir to start of fonts */
        fontdirend = ne_fontdir_addr(fnt_count);
        fontbeg    = ne_font_addr(0);
        while (fontdirend < fontbeg)
        {
                if (write_byte(fp, 0)) return -1;
                ++fontdirend;
        }        
        return 0;
}



static int ne_write_fonts(FILE *fp)
{
        int rv;
        int n, m, max;

	/* Packing between the last FONTDIR and the first FONT */
        for (n = 0; n < fnt_count; n++)
        {
                rv = msw_fontinfo_write(&msw[n], fp);
                if (rv) return rv;
                rv = msw_font_body_write(&msw[n], fp, msw_font[n]);
                if (rv) return rv;
		max = ne_font_addr(n+1) - ne_font_addr(n);
                for (m = msw[n].dfSize; m < max; m++)
                {
                        if (write_byte(fp, 0)) return -1;
                }
        }        
        return 0;
}




static int msw_load(int font, FILE *fp)
{
        int rv;

        rv = msw_fontinfo_read(&msw[font], fp);
        if (rv) return rv;
        rv = msw_font_body_read(&msw[font], fp, &msw_font[font]);

	if (msw[font].dfVersion > winver)
	{
		fprintf(stderr, 
"Warning: Font %d is version %d.%d, but output file version is %d.%d\n",
	font, (msw[font].dfVersion >> 8) & 0xFF, (msw[font].dfVersion & 0xFF),
		(winver >> 8) & 0xFF, winver & 0xff);
	}
        return rv;
}


/* Parse a version number as two integers separated by a dot. Note that 
 * this is not the same thing as a decimal number; 3.1 and 3.10 are different. 
 */
msw_word parse_version(char *v)
{
        int minor = 0, major = atoi(v);
        char *dot = strchr(v, '.');

        if (dot) minor=atoi(dot+1);
        return 256*major+minor;
}

/* Program name */
char *cnv_progname = "FNT2FON";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "dosmsg")) 
	{
		strncpy(msgbuf, value, sizeof(msgbuf) - 4);
		msgbuf[sizeof(msgbuf) - 4] = 0;
		strcat(msgbuf, "\r\n$");
		mz_message = msgbuf;
		return NULL;
	}
	if (!stricmp(variable, "sysfont")) 
	{
		sysfont = atoi(value);
		return NULL;
	}
	if (!stricmp(variable, "fontid")) 
	{
		strncpy(fres_str, value, sizeof(fres_str)-1);
		fres_str[sizeof(fres_str)-1] = 0;
#ifdef SHORT_CMDLINE
/* On systems that don't allow spaces in command-line arguments, replaces
 * underlines with spaces */
		{
			char *p;
			for (p = fres_str; *p; ++p)
			{
				if (*p == '_') *p = ' ';
			}
		}
#endif
		return NULL;
	}
	if (!stricmp(variable, "modname")) 
	{
		strncpy(mn_str, value, sizeof(mn_str)-1);
		mn_str[sizeof(mn_str)-1] = 0;
		modname = mn_str;
		return NULL;
	}
        if (!stricmp(variable, "winver"))
        {
                winver = parse_version(value);

/* Windows 1.x/2.x fonts are aligned at 512 bytes */
		if (winver < 0x300) align = 0x200;
                return NULL;
        }
/* These options are provided so that fon2fnts can produce output that is 
 * bytewise identical to genuine Windows fonts. It's pointless trying to 
 * calculate the checksums using sane methods because the values present
 * in the Windows 1.0 font files (for example) look more like timestamps
 * rather than checksums (though the timestamps aren't based on the UNIX
 * epoch) */
	if (!stricmp(variable, "checksum16")) 	/* Checksum in MZ header */
	{
		mz_checksum = atoi(value);
		return NULL;
	}
	if (!stricmp(variable, "checksum32")) 	/* Checksum in NE header */
	{
		ne_checksum = atol(value);
		return NULL;
	}
        if (!stricmp(variable, "linkver"))
        {
                linkver = parse_version(value);
                return NULL;
        }
	if (!stricmp(variable, "mzextra")) 	/* Byte 2 of MZ header */
	{
		mz_extra = atol(value);
		return NULL;
	}
	if (!stricmp(variable, "stub")) 	/* Which stub to use? */
	{
		stubtype = atol(value);
		if (stubtype > 2 || stubtype < 0)
		{
			sprintf(helpbuf, "Unrecognised --stub value: %d\n", stubtype);
			return helpbuf;
		}
		return NULL;
	}
	if (!stricmp(variable, "idbase")) 	/* First resource ID */
	{
		resid_first = atoi(value);
		return NULL;
	}
	if (!stricmp(variable, "idstep")) 	/* Resource ID increment */
	{
		resid_step = atoi(value);
		return NULL;
	}
        if (strlen(variable) > 2000) variable[2000] = 0;
        sprintf(helpbuf, "Unknown option: %s\n", variable);
        return helpbuf;
}


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s source.fnt source.fnt ... target.fon \n\n", cnv_progname);
    strcat(helpbuf,  "Options are: --winver=<windows version>\n");
    strcat(helpbuf,  "             --linkver=<linker version>\n");
    strcat(helpbuf,  "             --modname=<module name>\n");
    strcat(helpbuf,  "             --sysfont=<font index> : Mark font as system font\n");
    strcat(helpbuf,  "             --idbase=<first resource ID>\n");
    strcat(helpbuf,  "             --idstep=<resource ID increment>\n");
    strcat(helpbuf,  "             --fontid =<font descriptor>, \n");
#ifdef SHORT_CMDLINE
    strcat(helpbuf,  "                eg: --fontid=FONTRES_200,96,48_:_Cursive\n");
#else
    strcat(helpbuf,  "                eg: --fontid=\"FONTRES 200,96,48 : Cursive\"\n");
#endif
    strcat(helpbuf,  "             --dosmsg=<message for DOS stub to display>\n");
    strcat(helpbuf,  "             --stub=0, 1 or 2: Use alternative DOS stub\n");
    strcat(helpbuf,  "             --checksum16=<checksum>, --checksum32=<checksum>\n");
    strcat(helpbuf,  "             --mzextra=<word 2 of MZ header>\n");
    return helpbuf;
    }


static char *write_outfile(FILE *outfile)
{

        if (mz_write_header(outfile)       ||
            mz_write_body  (outfile)) return "Write error in DOS stub";
        if (ne_write_ne_header(outfile)    ||
            ne_write_res_table(outfile)    ||
            ne_write_rname_table(outfile)  ||
            ne_write_entry_table(outfile)  ||
            ne_write_nonres_table(outfile) ||
            ne_write_fontdirs(outfile)     ||
            ne_write_fonts(outfile) )                  
                return "Write error";
	return NULL;
}


/* Do the conversion */
char *cnv_multi(int nfiles, char **infiles, FILE *outfile)
{        
        int n, rv, nf;
        char *rvs = NULL;

        msw_font = malloc(nfiles * sizeof(msw_byte *));
        if (!msw_font) return "Out of memory.";
        msw      = malloc(nfiles * sizeof(MSW_FONTINFO));
        if (!msw) { free(msw_font); return "Out of memory."; }

        for (nf = 0; nf < nfiles; nf++)
        { 
            FILE *fp;

            if (!strcmp(infiles[nf], "-")) fp = stdin;
            else  fp = fopen(infiles[nf], "rb");
          
            if (!fp) { perror(infiles[nf]); return "Cannot open font file."; }
            rv = msw_load(nf, fp);
            if (fp != stdin) fclose(fp);

            if (rv < 0)
            {
                free(msw);
                free(msw_font);
                if (rv == -1) return "Read error on input file.";
                if (rv == -2) return ".FNT file is not in a known Windows format.";
                if (rv == -3) return "No memory to load .FNT files.";
                return "Unknown error loading font.";
            }
            ++fnt_count;
        }
        /* Generate the font-resource string */

	if (!fres_str[0])
	{
        	sprintf(fres_str, "FONTRES 200,96,48 : %-100s", 
                                msw_font[0] + msw[0].dfFace);
	}
	
	rvs = write_outfile(outfile);


        for(n = 0; n < nfiles; n++) if (msw_font[n]) free(msw_font[n]);

        free(msw_font);
        free(msw);
        return rvs;
}

