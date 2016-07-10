/*************************************************************************
**	 Copyright 2005, John Elliott                                   **
**       Copyright 1999, Caldera Thin Clients, Inc.                     **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
**                  Historical Copyright                                **
**                                                                      **
**                                                                      **
**                                                                      **
**  Copyright (c) 1987, Digital Research, Inc. All Rights Reserved.     **
**  The Software Code contained in this listing is proprietary to       **
**  Digital Research, Inc., Monterey, California and is covered by U.S. **
**  and other copyright protection.  Unauthorized copying, adaptation,  **
**  distribution, use or display is prohibited and may be subject to    **
**  civil and criminal penalties.  Disclosure to others is prohibited.  **
**  For the terms and conditions of software code use refer to the      **
**  appropriate Digital Research License Agreement.                     **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"

void    SdSdl::graphicText (const GPoint *pt, const wchar_t *str)
{
	graphicText(pt, str, false, NULL);	
}

void    SdSdl::graphicText (const GPoint *pt, const wchar_t *str,
		bool justified, Uint32 *just_w)
{
	bool need_extent;
	Sint32 delh, delv, rdel1, rdel2;
	GPoint ext[4];

	if (str == NULL || str[0] == 0) return;

	/* Take horizontal alignment into account. */

	need_extent = !justified;
	if (m_h_align == 0) delh = 0;
	else
	{
	     if (need_extent) queryTextExtent(str, ext);
             need_extent = false;
             delh = m_h_align == 1 ? m_width >> 1 : m_width;
             // XXX CONTRL[2] = 0;

	}	
	m_X1 = 1;
	switch(m_v_align)
	{
		case 0:	delv = 0;
			delh += m_L_OFF;
			break;

		case 1: delv = m_FONT_INF.half;
			if (m_FONT_INF.top) m_X1 = m_FONT_INF.top;
			delh += m_L_OFF + (m_FONT_INF.half * m_R_OFF) / m_X1;
			break;
		case 2: delv = m_FONT_INF.ascent;
			if( m_FONT_INF.top ) m_X1 = m_FONT_INF.top;
			delh += m_L_OFF + (m_FONT_INF.ascent * m_R_OFF) / m_X1;
			break;
	        case 3: delv = -m_FONT_INF.bottom;
	 		if( m_FONT_INF.bottom ) m_X1 = m_FONT_INF.bottom;
			delh -= ((m_FONT_INF.bottom - m_FONT_INF.descent) * m_L_OFF) / m_X1;
			break;
		case 4: delv = -m_FONT_INF.descent; 
			break;
	        case 5: delv = m_FONT_INF.top;
			delh += m_L_OFF + m_R_OFF;
			break;
        }
        rdel1 = m_FONT_INF.top - delv;
        rdel2 = (m_ACTDELY - m_FONT_INF.top - 1) + delv;
        switch (m_rot_case)
        {
		case 0: m_DEST.x = pt->x - delh;
			m_DEST.y = pt->y - rdel1;
			break;
                case 1: m_DEST.x = pt->x - rdel1;
                        m_DEST.y = pt->y + delh;
                        break;
                case 2: m_DEST.x = pt->x + delh;
                        m_DEST.y = pt->y - rdel2;
                        break;
                case 3: m_DEST.x = pt->x - rdel2;
                        m_DEST.y = pt->y - delh;
                        break;

	}
	if (m_SPECIAL != 0 || !m_MONO_STATUS || !MONO8XHT(str, justified))
	{
	    if ( (m_SPECIAL & SKEW) && (m_WRT_MODE == 0 || m_WRT_MODE == 3) )
	    {
		if (need_extent) queryTextExtent(str, ext);
	        need_extent = false;
		switch (m_rot_case)
		{
		case 0:
		    m_X1 = m_DEST.x;
		    m_X2 = m_X1 + m_width;
		    m_Y2 = m_Y1 = m_DEST.y + m_ACTDELY - 1;
		    break;
		case 1:
		    m_Y2 = m_DEST.y;
		    m_Y1 = m_Y2 - m_width;
		    m_X1 = m_X2 = m_DEST.x + m_ACTDELY - 1;
		    break;
		case 2:
		    m_X2 = m_DEST.x;
		    m_X1 = m_X2 - m_width;
		    m_Y2 = m_Y1 = m_DEST.y;
		    break;
		case 3:
		    m_Y1 = m_DEST.y;
		    m_Y2 = m_Y1 + m_width;
		    m_X1 = m_X2 = m_DEST.x;
		    break;
	        }

		clr_skew();
	    }  /* End if:  skewed and replace or inverse transparent mode. */
	    if (m_SPECIAL & UNDER)
	    {
                if (need_extent) queryTextExtent(str, ext);
                need_extent = false;

		switch (m_rot_case)
		{
		case 0:
		    m_X1 = m_DEST.x + m_L_OFF;
		    m_X2 = m_X1 + m_width;
		    m_Y1 = m_DEST.y + m_FONT_INF.top + 1;
		    m_Y2 = m_Y1 + m_FONT_INF.ul_size - 1;
		    break;
		case 1:
		    m_X1 = m_DEST.x + m_FONT_INF.top + 1;
		    m_X2 = m_X1 + m_FONT_INF.ul_size - 1;
		    m_Y2 = m_DEST.y - m_L_OFF;
		    m_Y1 = m_Y2 - m_width;
		    break;
		case 2:
		    m_X2 = m_DEST.x - m_L_OFF;
		    m_X1 = m_X2 - m_width;
		    m_Y2 = m_DEST.y + m_ACTDELY - m_FONT_INF.top - 1;
		    m_Y1 = m_Y2 - m_FONT_INF.ul_size + 1;
		    break;
		case 3:
		    m_X2 = m_DEST.x + m_ACTDELY - m_FONT_INF.top - 1;
		    m_X1 = m_X2 - m_FONT_INF.ul_size + 1;
		    m_Y1 = m_DEST.y + m_L_OFF;
		    m_Y2 = m_Y1 + m_width;
		    break;
	        }
	    }  /* End if:  underlined. */
	    m_XACC_DDA = 0.5; /* init the horizontal dda */
	    m_sav_pixel = m_text_pixel;
	    TEXT_BLT(str, justified);
	    m_text_pixel = m_sav_pixel;
	}
}



void    SdSdl::queryTextExtent(const wchar_t *str, GPoint *pt)
{	
	long i, j, horsts, ix;
	wchar_t ch;

	if (!str || !*str) return;	
	m_width = 0;
	
	for (i = 0; i < (long)wcslen(str); i++)
	{
		ch = str[i];
		if ((j = chk_ade(ch)) == 1) 
		{
			chk_fnt();
		}
		else if (j == 2)
		{
			ch = ' ';
			if (chk_ade(ch) == 1) 
			{
				chk_fnt();
			}
		}
		horsts = m_FONT_INF.flags & HORZ_OFF;
		ix = ch - m_FONT_INF.first_ade;
		m_width += m_FONT_INF.off_table[ix+1] - m_FONT_INF.off_table[ix];
		if (horsts)
		{
			ix <<= 1;
			m_width -= m_FONT_INF.hor_table[ix] + 	
				       m_FONT_INF.hor_table[ix+1];
		}
	}
	if (m_SPECIAL & SCALE) m_width = ACT_SIZ(m_width);

	if ((m_SPECIAL & THICKEN) && !(m_FONT_INF.flags & MONOSPACE))
		m_width += wcslen(str) * m_WEIGHT;

	m_height = m_CHR_HT;
	for (i = 0; i < 4; i++) pt[i].x = pt[i].y = 0;

	switch(m_rot_case)
	{
		case 0: pt[1].x = pt[2].x = m_width;
			pt[2].y = pt[3].y = m_height;
			break;
		case 1: pt[0].x = pt[1].x = m_height;
			pt[1].y = pt[2].y = m_width;
			break;
		case 2: pt[0].x = pt[3].x = m_width;
			pt[0].y = pt[1].y = m_height;
			break;
		case 3: pt[2].x = pt[3].x = m_height;
			pt[0].y = pt[3].y = m_width;
			break;

	}
	m_FLIP_Y = 1;
}


void SdSdl::queryText  (long *styles, GPoint *size)
{
	styles[0] = m_FONT_INF.font_id;
	styles[1] = m_text_color;
	styles[2] = 900*( (m_SPECIAL & ROTATE) >> 6 );
	styles[3] = m_h_align;
	styles[4] = m_v_align;
	styles[5] = m_WRT_MODE;

	size[0].x = m_FONT_INF.maxCharWidth;
	size[0].y = m_FONT_INF.top + 1;
	size[1].x = m_FONT_INF.maxCellWidth;
	size[1].y = size[0].y + m_FONT_INF.bottom;

	m_FLIP_Y = 1;
}


long SdSdl::stColor(long index)
{
	if (index < 0 || index >= MAX_COLOR) index = 1;
	m_text_color = index;
	m_text_pixel = m_globals->MAP_COL(index);
	return index;
}


#if 0
EXTERN WORD ftmgroff;
EXTERN WORD ftmgrseg;

EXTERN WORD FLIP_Y;
EXTERN WORD WRT_MODE;
EXTERN WORD SIZ_TAB[];
EXTERN WORD DEV_TAB[];
EXTERN WORD X1, X2, Y1, Y2;
EXTERN WORD FG_BP_1;
EXTERN WORD buff_seg, def_buf, sav_buf1, sav_buf2, txbuf1, txbuf2;

EXTERN WORD DELY;		/* width and height of character unscaled */
EXTERN UWORD *FOFF;
EXTERN WORD FWIDTH;		/* offset,segment and form with of font	*/
EXTERN UWORD *OFF_TBL;
EXTERN WORD FIR_CHR;

WORD	T_SCLSTS;
WORD	MONO_STATUS;
WORD	DESTX, DESTY;		/* upper left of destination on screen  */
WORD	ACTDELY;		/* actual height of character scaled*/
WORD	CHR_HT;			/* character top + bottom + 1*/
WORD	SPECIAL;		/* special effects flag word		*/
WORD	WEIGHT;			/* amount to thicken character		*/
WORD	R_OFF, L_OFF;		/* skew above and below baseline	*/
WORD	CHAR_DEL;		/* additional char width		*/
WORD    TEXT_BP;		/* character color			*/
UWORD	XACC_DDA;		/* accumulator for X DDA		*/
UWORD	DDA_INC;		/* the fraction to be added to the DDA  */
WORD	text_color;		/* requested color			*/
WORD	rot_case;		/* rotation case (0 - 3)		*/
WORD	h_align, v_align;	/* alignment				*/
WORD	width,height;		/* extent of string set in dqt_extent   */
GLOBAL BOOLEAN	rq_type;
GLOBAL BYTE	rq_font;
GLOBAL BYTE	rq_attr;
GLOBAL WORD	rq_size;
GLOBAL WORD	dbl;
	WORD	loaded;
static WORD ini_font_count;	/* default number of fonts		*/
#define	FHEAD struct font_head
struct font_head *cur_font,*act_font;	/* font head pointer */
struct font_head *cur_head;		/* font header copy pointer */
EXTERN struct font_head FONT_INF;	/* header for current font */
EXTERN struct font_head first;		/* font head pointer */
struct font_head *font_top = &first;

d_gtext()
{		 	   
	BOOLEAN	need_extent;
	WORD	delh, delv, rdel1, rdel2;
	WORD	sav_bp;

#if EGA
	EGA_KLUG();
#endif

	if ( CONTRL[3] == 0 )
	    return;
    
	/* Take horizontal alignment into account. */

        need_extent = (CONTRL[0] != 11);
        if (h_align == 0)
	    delh = 0;
	else
	{
	    if (need_extent)
	        dqt_extent();
	    need_extent = FALSE;
	    delh = h_align == 1 ? width >> 1 : width;
	    CONTRL[2] = 0;
	}  /* End else:  center or right alignment. */
	X1 = 1;
	switch (v_align)
	{
	case 0:
            delv = 0;
            delh += L_OFF;
	    break;
	case 1:
            delv = FONT_INF.half;
	    if ( FONT_INF.top )
		X1 = FONT_INF.top;
	    delh += L_OFF + (FONT_INF.half * R_OFF) / X1;
	    break;
	case 2:
            delv = FONT_INF.ascent;
	    if( FONT_INF.top )
		X1 = FONT_INF.top;
	    delh += L_OFF + (FONT_INF.ascent * R_OFF) / X1;
	    break;
	case 3:
            delv = -FONT_INF.bottom;
	    if( FONT_INF.bottom )
		X1 = FONT_INF.bottom;
	    delh -= ((FONT_INF.bottom - FONT_INF.descent) * L_OFF) / X1;
	    break;
	case 4:
            delv = -FONT_INF.descent;
	    break;
	case 5:
            delv = FONT_INF.top;
            delh += L_OFF + R_OFF;
	    break;
	}

	rdel1 = FONT_INF.top - delv;
	rdel2 = (ACTDELY - FONT_INF.top - 1) + delv;
	switch (rot_case)
	{
	case 0:
	    DESTX = PTSIN[0] - delh;
	    DESTY = PTSIN[1] - rdel1;
	    break;
	case 1:
	    DESTX = PTSIN[0] - rdel1;
	    DESTY = PTSIN[1] + delh;
	    break;
	case 2:
	    DESTX = PTSIN[0] + delh;
	    DESTY = PTSIN[1] - rdel2;
	    break;
	case 3:
	    DESTX = PTSIN[0] - rdel2;
	    DESTY = PTSIN[1] - delh;
	    break;
	}
 
#endif

void SdSdl::text_init(const GWSParams *wsp)
{
    //Sint32 new_width;
    Uint32 id_save;
    Sint32 temp_max, temp_top;
    FontHead *def_save;		/* font head pointer */
    
    Uint8 *scratch;

    m_cur_head = &m_FONT_INF;

    /* Set the default text scratch buffer addresses. */
    scratch = m_def_buf;
    m_txbuf1 = m_sav_buf1 = m_def_buf;
    m_txbuf2 = m_sav_buf2 = m_def_buf + cell_size;

    m_globals->m_SIZ_TAB[0].x = m_globals->m_SIZ_TAB[0].y = 32767;
    m_globals->m_SIZ_TAB[1].x = m_globals->m_SIZ_TAB[1].y = 0;
	
    m_cur_font = m_font_top;
    id_save = m_cur_font->font_id;

    for ( m_globals->m_DEV_TAB[5] = 0, m_ini_font_count = 1 ; TRUE ;
          m_cur_font = m_cur_font->next_font )
    {
        if ( m_cur_font->flags & DEFAULT ) def_save = m_cur_font;
	if ( m_cur_font->font_id != id_save )
        {   
	    m_ini_font_count++;
	    id_save = m_cur_font->font_id;
	}
	if (m_cur_font->font_id == 1)
	{
	    temp_max = m_cur_font->maxCharWidth;
	    temp_top = m_cur_font->top + 1;
	    if ( m_globals->m_SIZ_TAB[0].x > temp_max ) m_globals->m_SIZ_TAB[0].x = temp_max;
	    if ( m_globals->m_SIZ_TAB[0].y > temp_top ) m_globals->m_SIZ_TAB[0].y = temp_top;
	    if ( m_globals->m_SIZ_TAB[1].x < temp_max ) m_globals->m_SIZ_TAB[1].x = temp_max; 
	    if ( m_globals->m_SIZ_TAB[1].y < temp_top ) m_globals->m_SIZ_TAB[1].y = temp_top;
	    m_globals->m_DEV_TAB[5]++;    		/* number of sizes */
 	}

	if (m_cur_font->next_font == 0) break;
    }

    m_globals->m_DEV_TAB[10] = m_ini_font_count;	/* number of faces */

    m_act_font = m_cur_font = def_save;
    cpy_head();

    stColor(wsp->defaults[5]); 
    m_SPECIAL = m_rot_case = m_h_align = m_v_align = 0;
    in_rot();
    in_doub();	

    /* rq_size was set to 1 in the original source from Ventura.
       GEM WRITE chose the 8 point system font.  
       Sel_Size searched for a system font 1 pixel high as it's
       default.
       rq_size is now set for a 10 point system font as the default
       jn 9-15-87 */

    m_rq_font = 1;				/* jn 9-15-87 */
    m_rq_size = 10 ;				/* jn 9-15-87 */
    m_rq_attr = 0;
    m_dbl = 0;
    m_rq_type = TRUE;
    m_loaded = FALSE;
    m_CHAR_DEL = m_WEIGHT = m_R_OFF = m_L_OFF = m_MONO_STATUS = 0;	

    m_CHR_HT  = m_FONT_INF.top + m_FONT_INF.bottom + 1;
    m_ACTDELY = m_FONT_INF.form_height;
}


void SdSdl::stHeight(const GPoint *height, GPoint *result)
{
	GPoint h = *height;

	if (m_xfm_mode == 0) h.y = (m_globals->m_DEV_TAB[1] + 1) - h.y;

    	m_rq_size = h.y;
    	if( m_rq_size == 0 ) m_rq_size = 1;	/* it must be at least one pel high */
	m_rq_type = FALSE;	
	sel_font();	

	result[0].x = m_FONT_INF.maxCharWidth;
	result[0].y = m_FONT_INF.top+1;
	result[1].x = m_FONT_INF.maxCellWidth;
	result[1].y = m_CHR_HT;	
	inc_lfu();
	m_FLIP_Y = 1;
} 

long SdSdl::stPoint(long point, GPoint *sizes)
{
	m_rq_size = point;
	m_rq_type = TRUE;
	sel_font();
	
	sizes[0].x = m_FONT_INF.maxCharWidth;
	sizes[0].y = m_FONT_INF.top+1;
	sizes[1].x = m_FONT_INF.maxCellWidth;
	sizes[1].y = m_CHR_HT;
	inc_lfu();
	return m_FONT_INF.point;
} 


void SdSdl::make_header()
{
    Uint32 i;

    cpy_head();
    m_FONT_INF.point <<= 1;
    i = ACT_SIZ( m_FONT_INF.top );
    if( m_DDA_INC == 1.0 ) i++;
    else
    {
	if( i == ACT_SIZ( m_FONT_INF.top + 1 ) ) i--;	
    }		
    m_FONT_INF.top = i;
    m_FONT_INF.ascent = ACT_SIZ( m_FONT_INF.ascent ) + 1;
    m_FONT_INF.half = ACT_SIZ( m_FONT_INF.half ) + 1;
    m_FONT_INF.descent = ACT_SIZ(m_FONT_INF.descent );
    m_FONT_INF.bottom = ACT_SIZ( m_FONT_INF.bottom );
    m_FONT_INF.maxCharWidth = ACT_SIZ( m_FONT_INF.maxCharWidth );
    m_FONT_INF.maxCellWidth = ACT_SIZ( m_FONT_INF.maxCellWidth );
    m_FONT_INF.leftOffset = ACT_SIZ( m_FONT_INF.leftOffset );
    m_FONT_INF.rightOffset = ACT_SIZ( m_FONT_INF.rightOffset );
    m_FONT_INF.thicken = ACT_SIZ( m_FONT_INF.thicken );
    m_FONT_INF.ul_size = ACT_SIZ( m_FONT_INF.ul_size );
    m_ACTDELY = ACT_SIZ( m_FONT_INF.form_height );

    m_SPECIAL |= SCALE;
    m_cur_font = m_cur_head;
}

long SdSdl::stStyle(long style)
{
    m_rq_attr = style & 0x000f;	
    sel_font();	
    return m_rq_attr;
} 

void SdSdl::stAlign(long h, long v, long *ah, long *av)
{
	m_h_align = *ah = h;
	m_v_align = *av = v;
}


long SdSdl::stRotation(long angle)
{
	while (angle < 0)     angle += 3600;
	while (angle >= 3600) angle -= 3600;

	m_rot_case = ((angle + 450) / 900) & 3;
    	m_SPECIAL = (m_SPECIAL & ~ROTATE) | ((m_rot_case << 6) & ROTATE);
    	return m_rot_case * 900;
}


long SdSdl::stFont(long font)
{
	m_rq_font = font;
	sel_font();
	return m_FONT_INF.font_id & 0xFF;
}


FontHead *SdSdl::sel_effect(FontHead *f_ptr)
{
	Sint32		size;
	Uint8		eff;
	Uint8		find;
	Uint8		font;
	FontHead	*bold;
	FontHead	*italic;
	FontHead	*normal;
	FontHead	*ptr;

	/* Determine what is being searched for. */
	font = f_ptr->font_id;
	size = f_ptr->point;
	find = m_rq_attr & (SKEW | THICKEN);
	normal = italic = bold = NULL;
	m_SPECIAL = m_SPECIAL & 0xfff0;
	m_SPECIAL = m_SPECIAL | m_rq_attr;
	/* Scan for a font matching the effect exactly. */
	for (ptr = f_ptr; ptr && ((ptr->font_id & 0xff) == font) &&
			(ptr->point == size); ptr = ptr->next_font) 
	{
		eff = ((ptr->font_id >> 8) & 0x00ff);
		if (eff == find) 
		{
			m_SPECIAL &= (~find);
			return(ptr);
		}  /* End if:  effect found. */

		if (eff == 0x0) normal = ptr;
		else if (eff == THICKEN) bold = ptr;
		else if (eff == SKEW) italic = ptr;
	}  /* End for:  over fonts. */

	/* No exact match was found.  If the effect is bold-italic, */
	/* return italic (if possible) or bold (if possible).       */
	if (find == (SKEW | THICKEN)) 
	{
		if (italic) 
		{
			m_SPECIAL &= (~SKEW);
			return(italic);
		}  /* End if:  returning italic. */
		else if (bold) 
		{
			m_SPECIAL &= (~THICKEN);
			return(bold);
		}  /* End elseif:  returning bold. */
	}  /* End if:  bold-italic. */

	/* Running out of alternatives.  Set the style and try to */
	/* return normal.                                         */
	if (normal) return(normal);
	else return(f_ptr);
}  /* End "sel_effect". */


FontHead *SdSdl::sel_size(FontHead *f_ptr)
{
	Sint32		close_size;
	Sint32		this_size;
	Uint8		font;
	Sint32		ptsize;
	Sint32		curptsz;
	FontHead	*ptszptr;
	FontHead	*close_ptr;
	FontHead	*ptr;

	/* Look for a font whose size matches exactly. */
	font = f_ptr->font_id;
	ptsize = f_ptr->point;
	ptszptr = f_ptr;
	for (ptr = f_ptr; ptr && ((ptr->font_id & 0xff) == font);
			ptr = ptr->next_font) 
	{
		curptsz = ptr->point;
		if (m_rq_type) this_size = curptsz;
		else 	       this_size = ptr->top + 1;
		if( ptsize != curptsz )	/* return head of size chain */
		{
		    ptsize = curptsz;
		    ptszptr = ptr;
		}
		if (this_size == m_rq_size) return(ptszptr);
		else if (this_size > m_rq_size) break;
	}  /* End for:  over fonts. */

	/* No exact match.  Look for a doubling match. */
	ptsize = f_ptr->point;
	ptszptr = f_ptr;
	for (ptr = f_ptr; ptr && ((ptr->font_id & 0xff) == font);
			ptr = ptr->next_font) 
	{
		curptsz = ptr->point;
		if (m_rq_type) this_size = curptsz;
		else this_size = ptr->top + 1;
		if( ptsize != curptsz )	/* return head of size chain */
		{
		    ptsize = curptsz;
		    ptszptr = ptr;
		}
		this_size <<= 1;
		if (this_size == m_rq_size) 
		{
			m_dbl = TRUE;
			return(ptszptr);
		}  /* End if:  exact doubling match. */
		else if (this_size > m_rq_size) break;
	}  /* End for:  over fonts. */

	/* No doubling match.  Return the next size lower (this */
	if (m_rq_type)  close_size = f_ptr->point;
	else 		close_size = f_ptr->top + 1;
	close_ptr = f_ptr;
	ptsize = f_ptr->point;
	ptszptr = f_ptr;
	for (ptr = f_ptr; ptr && ((ptr->font_id & 0xff) == font);
			ptr = ptr->next_font) 
	{
		curptsz = ptr->point;
		if (m_rq_type)	this_size = curptsz;
		else		this_size = ptr->top + 1;
		if( ptsize != curptsz )	/* return head of size chain */
		{
		    ptsize = curptsz;
		    ptszptr = ptr;
		}
		if (this_size < m_rq_size) 
		{
/* DJH 3/23/87 */
/* fixed potential doubling instead of scaling bug < to <= */
			if (m_rq_size - this_size <= m_rq_size - close_size) 
/* DJH */		{
				close_ptr = ptszptr;
				close_size = this_size;
				m_dbl = FALSE;
			}  /* End if:  smaller found. */
		}  /* End if:  normal candidate. */

/*----------------------------------------------------------------
* jn 11-10-87
* Put doubling back in
* the following comment was Dons.
* DJH 3/23/87 removed doubling to make sure largest size is used 
* He used his initials to close the comment 
*-----------------------------------------------------------------*/
//#if 1

		if ((this_size <<= 1) < m_rq_size) 
		{
			if (m_rq_size - this_size < m_rq_size - close_size) 
			{
				close_ptr = ptr;
				close_size = this_size;
				m_dbl = TRUE;
			}  /* End if:  smaller found. */
			m_dbl = TRUE;
		}  /* End if:  double candidate. */

//#endif
	}  /* End for:  over fonts. */
	if( (this_size << 1) < m_rq_size ) m_dbl = TRUE;
	return(close_ptr);
}  /* End "sel_size". */


void SdSdl::sel_font()
{
	FontHead *ptr;

	/* Find a matching face identifier.  If there is no matching */
	/* face, use the first non-dummy face.                       */
	for (ptr = m_font_top; ptr && ((ptr->font_id & 0xff) != m_rq_font);
		ptr = ptr->next_font)
		;
	if (!ptr) ptr = m_font_top;

	/* Find the first font in the face which is the closest in size */
	/* (note:  it may be a doubled font -- dbl will be set).        */
	m_dbl = FALSE;
	ptr = sel_size(ptr);

	/* Find the closest matching attribute. */
	m_act_font = m_cur_font = sel_effect(ptr);

	/* If the font needs to be doubled, build a header. */
        m_SPECIAL &= ~SCALE;
        cpy_head();
        m_ACTDELY = m_FONT_INF.form_height;
	if ( (m_dbl) || (m_FONT_INF.top + 1 != (Uint32)m_rq_size ) )
	{
	    if( m_dbl )
	    {
	        m_DDA_INC = 1.0;
	        m_T_SCLSTS = 1;
		make_header();
	    }
	    else
            {
		if( !m_rq_type )
		{
	            m_DDA_INC = CLC_DDA(m_FONT_INF.top + 1, m_rq_size);
	            make_header();
		}
            }
	}
	m_CHR_HT = m_FONT_INF.top + 1 + m_FONT_INF.bottom;
    	m_CHAR_DEL = m_WEIGHT = m_R_OFF = m_L_OFF = 0;	
    	if (m_SPECIAL & THICKEN)
	{
		m_CHAR_DEL = m_WEIGHT = m_FONT_INF.thicken;
	}
    	if (m_SPECIAL & SKEW)
    	{
	    m_L_OFF = m_FONT_INF.leftOffset;
   	    m_R_OFF = m_FONT_INF.rightOffset;
	    m_CHAR_DEL += (m_L_OFF + m_R_OFF);
        }
        m_MONO_STATUS = (m_FONT_INF.maxCellWidth == 8) ? 
				(MONOSPACE & m_FONT_INF.flags) : FALSE;

}  /* End "sel_font". */
#if 0

dqt_width()
{
    WORD  k;
	
    k = INTIN[0];
    
    if (chk_ade(k) == 1) {	/* jn 1-5-88, make sure font is in ram */
        chk_fnt();		
    }

    if (k < FONT_INF.first_ade || k > FONT_INF.last_ade)
	INTOUT[0] = -1;
    else
    {
    	INTOUT[0] = k;
	k -= FONT_INF.first_ade;
 	PTSOUT[0] = FONT_INF.off_table[k+1] - FONT_INF.off_table[k];
 	if ( (SPECIAL & SCALE) != 0 )
	    PTSOUT[0] = ACT_SIZ(PTSOUT[0]);

        if ( FONT_INF.flags & HORZ_OFF )
        {
	    PTSOUT[2] = FONT_INF.hor_table[k << 1];
	    PTSOUT[4] = FONT_INF.hor_table[(k << 1) + 1];
        }
        else
	    PTSOUT[2] = PTSOUT[4] = 0;
    }
    CONTRL[2] = 3;
    CONTRL[4] = 1;
    FLIP_Y = 1;	
}

dqt_name()
{
    WORD i;
    BYTE *name;
    struct font_head *tmp_font;

    tmp_font = font_top;
    for (i = 1 ; i < INTIN[0] ; i++, tmp_font = tmp_font->next_font)
      while ((tmp_font->font_id & 0xff)==(tmp_font->next_font->font_id & 0xff))
	    tmp_font = tmp_font->next_font;
    INTOUT[0] = (tmp_font->font_id & 0xff) ;
    for (i = 1, name = tmp_font->name ; INTOUT[i++] = *name++ ;)
	;
    while(i < (CONTRL[4] = 33))
	INTOUT[i++] = 0;
} 

dqt_fontinfo()
{
#if 0
    INTOUT[0] = FIR_CHR;
    INTOUT[1] = FONT_INF.last_ade;
#else
    INTOUT[0] = 32;
    INTOUT[1] = 255 ;
#endif
    PTSOUT[0] = FONT_INF.max_cell_width;
    PTSOUT[1] = FONT_INF.bottom;
    PTSOUT[3] = FONT_INF.descent;
    PTSOUT[5] = FONT_INF.half;
    PTSOUT[7] = FONT_INF.ascent;
    PTSOUT[9] = FONT_INF.top;
    if ( !(FONT_INF.flags & MONOSPACE) )
        PTSOUT[2] = WEIGHT;
    else
	PTSOUT[2] = 0;
    PTSOUT[6] = R_OFF;
    PTSOUT[4] = L_OFF;

    CONTRL[2] = 5;
    CONTRL[4] = 2;
    FLIP_Y = 1;	
} 

dt_loadfont()
{
  WORD id, i;
  union
  {
    WORD w_ptr[2];
    struct font_head *f_ptr;
  } fontptr;
  struct font_head *this_font;

  /* The scratch text buffer segment is passed in CONTRL[8].  The size of */
  /* the buffer is passed in CONTRL[9].  Use them to set the text scratch */
  /* buffer addresses.                                                    */
  buff_seg = CONTRL[8];
  txbuf1 = 0;
  txbuf2 = CONTRL[9] >> 1;
  ftmgroff = CONTRL[10];
  ftmgrseg = CONTRL[5];

  /* The font segment is passed in CONTRL[7].  Plug it into a pointer. */
  fontptr.w_ptr[0] = 0;
  fontptr.w_ptr[1] = CONTRL[7];
  v_nop();  /* Don't remove this -- Lattice optimization may bite you! */

  /* If the font offset and segment are not already linked into the */
  /* font chain, link them in.                                      */
  for (this_font = font_top;
    (this_font->next_font != 0) && (this_font->next_font != fontptr.f_ptr);
    this_font = this_font->next_font);

  CONTRL[4] = 1;
  INTOUT[0] = 0;

  if (this_font->next_font == 0)
  {
    /* Patch the last link to point to the first new font. */
    this_font->next_font = fontptr.f_ptr;
    id = this_font->font_id & 0xff ;

    /* Find out how many distinct font id numbers have just been linked in. */
    while (this_font = this_font->next_font)
    {
      /* Update the count of font id numbers, if necessary. */
      if ((this_font->font_id & 0xff) != id)
      {
        id = this_font->font_id & 0xff ;
        INTOUT[0]++;
      }  /* End if:  new font id found. */
    }  /* End while:  over fonts. */
  }  /* End if:  must link in. */
  /* Update the device table count of faces. */
  DEV_TAB[10] += INTOUT[0];
  loaded = TRUE;
}  /* End "dt_loadfont". */

dt_unloadfont()
{
  union
  {
    WORD w_ptr[2];
    struct font_head *f_ptr;
  } fontptr;
  union
  {
    WORD wd[2];
    WORD *pt;
  } scratch;
  struct font_head *this_font;

  /* The font segment chain to be unloaded is passed in CONTRL[7].  Plug */
  /* it into a pointer.  If it is zero, bail out.                        */
  if (!CONTRL[7])
    return;
  fontptr.w_ptr[0] = 0;
  fontptr.w_ptr[1] = CONTRL[7];
  v_nop();  /* Don't remove this -- Lattice optimization may bite you! */

  /* Find a pointer to the font segment and offset in the chain and */
  /* break the link.                                                */
  for (this_font = font_top;
    (this_font->next_font != 0) && (this_font->next_font != fontptr.f_ptr);
    this_font = this_font->next_font);
  if (this_font->next_font)
    this_font->next_font = 0;

  /* Reset the default text font and scratch buffer addresses. */
  scratch.pt = &def_buf;
  v_nop();  /* Don't remove this -- Lattice optimization may bite you! */
  buff_seg = scratch.wd[1];
  txbuf1 = sav_buf1;
  txbuf2 = sav_buf2;
  cur_font = font_top;
  cpy_head();

  /* Re-initialize the number of fonts indicated in the device table. */
  DEV_TAB[10] = ini_font_count;
  loaded = FALSE;
}  /* End "dt_unloadfont". */

#endif // 0

