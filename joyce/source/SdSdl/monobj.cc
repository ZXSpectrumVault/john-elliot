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

#if 0

/* CLEAR_WORKSTATION: */
v_clrwk()
{
	CLEARMEM();
}

/* UPDATE_WORKSTATION: */
v_updwk()
{
}
#endif
/* SET_COLOR_REP: */
void SdSdl::setColor(long index, short r, short g, short b)
{
	m_globals->setColor(index, r, g, b);

// Rebuild pixel values for mapped colours
	if (m_line_color == index) m_line_pixel = m_globals->MAP_COL(m_line_color);
	if (m_mark_color == index) m_mark_pixel = m_globals->MAP_COL(m_mark_color);
	if (m_fill_color == index) m_fill_pixel = m_globals->MAP_COL(m_fill_color);
	if (0            == index) m_bg_pixel   = m_globals->MAP_COL(0);
	if (m_text_color == index) m_text_pixel = m_globals->MAP_COL(m_text_color);
}

/* INQUIRE_COLOR_REP: */
long SdSdl::qColor(long index, short mode, short *r, short *g, short *b)
{
	return m_globals->qColor(index, mode, r, g, b);
}


/* S_LINE_TYPE: */
long SdSdl::slType(long type)
{
	m_line_index = type - 1;
	if ((m_line_index >= MAX_LINE_STYLE) || (m_line_index < 0))
	  m_line_index = 0; 
	return (m_line_qi = (m_line_index + 1));
} 



/*-------------
 * S_LINE_WIDTH
 *-------------*/
void SdSdl::slWidth(const GPoint *pwidth, GPoint *result)
{
	int i, j, x, y, d, low, high;
	long width = pwidth->x;

	/* Limit the requested line width to a reasonable value. */

	if (width < 1) width = 1;
	else if (width > MAX_L_WIDTH) width = MAX_L_WIDTH;

	/* Make the line width an odd number (one less, if even). */

	width = ((width - 1)/2)*2 + 1;

	/* Set the line width internals and the return parameters.  
	   Return if the line width is being set to one. */

	result->y = 0;
	if ( (result->x = m_line_qw = m_line_width = width) == 1)
	  return;

	/* Initialize the circle DDA.  "y" is set to the radius. */

	x = 0;
	y = (m_line_width + 1)/2;
	d = 3 - 2*y;

	for ( i = 0 ; i < MAX_L_WIDTH ; i++ ) {
	  m_q_circle[i] = 0 ;
	}

	/* Do an octant, starting at north.  
	   The values for the next octant (clockwise) will
	   be filled by transposing x and y. */

	 while (x < y) {
	   m_q_circle[y] = x;
	   m_q_circle[x] = y;

	   if (d < 0) {
	     d = d + (4 * x) + 6;
	   }
	   else {
	     d = d + (4 * (x - y)) + 10;
	     y--;
	   }
	   x++;
	 }  /* End while loop. */

	 if (x == y)
	   m_q_circle[x] = x;

	 /* Calculate the number of vertical pixels required. */

	 m_num_qc_lines = (m_line_width * xsize / ysize) / 2 + 1;

	 /* Fake a pixel averaging when converting to 
	    non-1:1 aspect ratio. */

	low = 0;
	for (i = 0; i < m_num_qc_lines; i++) {
	  high = ((2 * i + 1) * ysize / xsize) / 2;
	  d = 0;

	  for (j = low; j <= high; j++) {
	    d += m_q_circle[j];
	  }

	  m_q_circle[i] = d/(high - low + 1);
	  low = high + 1;
	} 
}



/* S_END_STYLE: */
void SdSdl::slEnds(const long *intin, long *intout)
{
  m_line_beg = intin[0];
  if ( (m_line_beg < 0) || (m_line_beg > 2) )
    m_line_beg = 0;
  m_line_end = intin[1];
  if ( (m_line_end < 0) || (m_line_end > 2) )
    m_line_end = 0;
  intout[0] = m_line_beg;
  intout[1] = m_line_end;
}  /* End "vsl_ends". */


/* S_LINE_COLOR: */
long SdSdl::slColor(long index)
{
	m_line_color = index;
	if ((m_line_color >= MAX_COLOR) || (m_line_color < 0))
	  m_line_color = 1; 
	m_line_qc = m_line_color;
	m_line_pixel = m_globals->MAP_COL(  m_line_color );
	return m_line_color;
}

/* S_MARK_TYPE: */
long SdSdl::smType(long type)
{
	m_mark_qi = type;
	if ((m_mark_qi > MAX_MARK_INDEX) || (m_mark_qi < 1))
	  m_mark_qi = 3; 
	m_mark_index = (m_mark_qi) - 1;
	return m_mark_qi;
}

/* S_MARK_SCALE: */
void SdSdl::smHeight(const GPoint *height, GPoint *result)
{
  long y = height->y;

  /* Limit the requested marker height to a reasonable value. */
  if      (y < DEF_MKHT) y = DEF_MKHT;
  else if (y > MAX_MKHT) y = MAX_MKHT;

  /* Set the marker height internals and the return parameters. */
  m_mark_height = y;
  m_mark_scale = (m_mark_height + DEF_MKHT/2)/DEF_MKHT;

  result->x = m_mark_scale*DEF_MKWD;
  result->y = m_mark_scale*DEF_MKHT;
  m_FLIP_Y = 1; 
}  /* End "vsm_height". */

/* S_MARK_COLOR: */
long SdSdl::smColor(long colour)
{
	m_mark_color = colour;
	if ((m_mark_color >= MAX_COLOR) || (m_mark_color < 0))
	  m_mark_color = 1; 
	m_mark_qc = m_mark_color;
	m_mark_pixel = m_globals->MAP_COL(m_mark_color );
	return m_mark_qc;
}

 
/* S_FILL_STYLE: */
long SdSdl::sfStyle(long style)
{
	m_fill_style = style;
	if ((m_fill_style > MX_FIL_STYLE) || (m_fill_style < 0))
	  m_fill_style = 0; 
	m_NEXT_PAT = 0;
	if ( m_fill_style == 4 )
	   m_NEXT_PAT = m_udpt_np;
	st_fl_ptr();
	return m_fill_style;
}
	
/* S_FILL_INDEX: */
long SdSdl::sfIndex(long index)
{
	m_fill_qi = index;
	if (m_fill_style == 2 )
	{
	    if ((m_fill_qi > MX_FIL_PAT_INDEX) || (m_fill_qi < 1))
	  	m_fill_qi = 1; 
	}
	else 
	{
	    if ((m_fill_qi > MX_FIL_HAT_INDEX) || (m_fill_qi < 1))
	  	m_fill_qi = 1; 
	}
	m_fill_index = m_fill_qi - 1;
	st_fl_ptr();
	return m_fill_qi;
}

/* S_FILL_COLOR: */
long SdSdl::sfColor(long colour)
{
	m_fill_color = colour;
	if ((m_fill_color >= MAX_COLOR) || (m_fill_color < 0))
	  m_fill_color = 1; 
	m_fill_qc = m_fill_color;
	m_fill_pixel = m_globals->MAP_COL( m_fill_color );
	return m_fill_qc;
}

/* LOCATOR_INPUT: */
short SdSdl::qLocator(short device, const GPoint *ptin, GPoint *ptout, 
		short *termch)
{
	short i;

        /* Set the initial locator position. */
	if ( m_loc_mode == 0 )
	{
		m_HIDE_CNT = 1;
		DIS_CUR();
		while ( (i = GLOC_KEY(true)) != 1 ); /* loop till some event */
		ptout->x = m_X1;
		ptout->y = m_Y1;
		HIDE_CUR();
		*termch = m_TERM_CH & 0xFF;
	}
	else
	{
		i = GLOC_KEY(false);
		switch(i)
		{
			case 0: break;
			case 1: *termch = (m_TERM_CH & 0xFF); break;
			case 3: *termch = (m_TERM_CH & 0xFF); 
				/* FALL THROUGH */
			case 2: ptout->x = m_X1;
				ptout->y = m_Y1;
				break;
		}
	}
	return i;
}
#if 0
/* SHOW CURSOR */
v_show_c()
{
  if ( !INTIN[0] )
    HIDE_CNT = 1;
  DIS_CUR();
}

/* HIDE CURSOR */
v_hide_c()
{
  HIDE_CUR();
}

/* RETURN MOUSE BUTTON STATUS */
vq_mouse_status() 
{
	INTOUT[0] = MOUSE_BT;
	CONTRL[4] = 1;
	CONTRL[2] = 1;
	PTSOUT[0] = GCURX;
	PTSOUT[1] = GCURY;
}

/* VALUATOR_INPUT: */
v_valuator()
{
}
#if ibmvdi
/* CHOICE_INPUT: */
v_choice()
{
	WORD	i;
	if ( chc_mode == 0 )
	{
	  CONTRL[4]=1;
	  while ( GCHC_KEY() != 1 );
	    INTOUT[0]=TERM_CH &0x00ff;
	}
	else
	{
	  i = GCHC_KEY();
	  CONTRL[4]=i;
	  if (i == 1)
	    INTOUT[0]=TERM_CH &0x00ff;
	  if (i == 2)
	    INTOUT[1]=TERM_CH & 0x00ff;
	}	
}
#else
v_choice()
{
}
#endif

/* STRING_INPUT: */
v_string()
{
	WORD	i,j,k,mask;
	mask = 0x00ff;
	j = INTIN[0];
	if ( j < 0 )
	{
	    j = -j;
	    mask = 0xffff;
	}
	if ( !str_mode )  /* Request mode */
	{
	  TERM_CH = 0;
	  for ( i = 0;( i < j) && ((TERM_CH & 0x00ff) != 0x000d); i++)
	  {
	    while ( GCHR_KEY() == 0 );
	    INTOUT[i] = TERM_CH = TERM_CH & mask;	
	  }
	  if ( ( TERM_CH & 0x00ff )== 0x000d )
	    --i;
	  CONTRL[4] = i; 
	}
	else  /* Sample mode */
	{
	  i = 0;
	  while ( (i < j) && (GCHR_KEY() != 0) )
	    INTOUT[i++] = TERM_CH & mask;
	  CONTRL[4] = i;
	}
}
/* Return Shift, Control, Alt State */
vq_key_s()
{
	CONTRL[4] = 1;
	INTOUT[0] = GSHIFT_S();
}

#endif

/* SET_WRITING_MODE: */
short SdSdl::swrMode(short mode)
{
	m_WRT_MODE = (mode - 1);
	if ((m_WRT_MODE > MAX_MODE) | (m_WRT_MODE < 0))
	  m_WRT_MODE = 0;
	
	return (m_write_qm = (m_WRT_MODE + 1));
}
/* SET_INPUT_MODE: */
short SdSdl::sinMode(short device, short mode)
{
	short	i = mode;

	i--; 
	switch ( device )
	{	
	  case 0:
	    break;
	  
	  case 1:	/* locator */
	    m_loc_mode = i;
	    break;

	  case 2:	/* valuator */
	    m_val_mode = i;
	    break;

	  case 3: /* choice */
	    m_chc_mode = i;
	    break;

	  case 4: /* string */
	    m_str_mode = i;
	    break;
	}
	return mode;
}

/* INQUIRE INPUT MODE: */
short SdSdl::qinMode(short device)
{
	switch ( device )
	{	
	  case 0:
	    break;
	  
	  case 1:	/* locator */
	    return m_loc_mode;

	  case 2:	/* valuator */
	    return m_val_mode;

	  case 3: /* choice */
	    return m_chc_mode;

	  case 4: /* string */
	    return m_str_mode;
	}
	return 0;
}
	    
/* ST_FILLPERIMETER: */
long SdSdl::sfPerimeter(long value)
{
	if ( !value) m_fill_per = FALSE;
	else	     m_fill_per = TRUE;
	return m_fill_per;
}


/* ST_UD_LINE_STYLE: */
void SdSdl::udLinestyle(long pattern)
{
	m_line_sty[6] = pattern;
}

/* Set Clip Region */
void SdSdl::setClip(short do_clip, const GPoint *pt)
{
    GPoint rect[2];

    m_CLIP = do_clip;
    if ( do_clip)
    {		
        arb_corner(pt, rect, ULLR);
    	m_XMN_CLIP = rect[0].x;
    	m_YMN_CLIP = rect[0].y;
    	m_XMX_CLIP = rect[1].x;
    	m_YMX_CLIP = rect[1].y;
    	if ( m_XMN_CLIP < 0 ) m_XMN_CLIP = 0;
    	if ( m_XMX_CLIP > m_globals->m_DEV_TAB[0] ) 
		m_XMX_CLIP = m_globals->m_DEV_TAB[0];
    	if ( m_YMN_CLIP < 0 ) m_YMN_CLIP = 0;
    	if ( m_YMX_CLIP > m_globals->m_DEV_TAB[1] )
		m_YMX_CLIP = m_globals->m_DEV_TAB[1];
    }	
    else
    {
        m_XMN_CLIP = m_YMN_CLIP = 0;
        m_XMX_CLIP = xres;
        m_YMX_CLIP = yres;
   }  /* End else:  clipping turned off. */
}

void SdSdl::arb_corner(const GPoint *ptsin, GPoint *xy, int type)
{
  /* Local declarations. */
  long temp;

  xy[0] = ptsin[0];
  xy[1] = ptsin[1];

  /* Fix the x coordinate values, if necessary. */
  if (xy[0].x > xy[1].x)
  {
    temp    = xy[0].x;
    xy[0].x = xy[1].x;
    xy[1].x = temp;
  }  /* End if:  "x" values need to be swapped. */

  /* Fix y values based on whether traditional (ll, ur) or raster-op */
  /* (ul, lr) format is desired.                                     */
  if ( ( (type == LLUR) && (xy[0].y < xy[1].y) ) ||
       ( (type == ULLR) && (xy[0].y > xy[1].y) ) )
    {
      temp    = xy[0].y;
      xy[0].y = xy[1].y;
      xy[1].y = temp;
    }  /* End if:  "y" values need to be swapped. */
}  /* End "arb_corner". */

#if 0
dro_cpyfm()
{
  arb_corner(PTSIN, ULLR);
  arb_corner(&PTSIN[4], ULLR);
  COPYTRAN = 0; 	
  COPY_RFM();
}  /* End "dr_cpyfm". */

drt_cpyfm()
{
  arb_corner(PTSIN, ULLR);
  arb_corner(&PTSIN[4], ULLR);
  COPYTRAN = 0xffff; 	
  COPY_RFM();
}  /* End "dr_cpyfm". */

#endif
void SdSdl::fillRect(const GPoint ptin[2])
{
	GPoint pt[2];

  /* Perform arbitrary corner fix-ups and invoke the rectangle fill routine. */
	arb_corner(ptin, pt, LLUR);
	m_FG_BP_1 = m_fill_pixel;
	m_LSTLIN = false;
	m_X1 = pt[0].x;
	m_X2 = pt[1].x;
	m_Y2 = pt[0].y;
	m_Y1 = pt[1].y;
	RECTFILL();
	update_rect(m_X1, m_Y1, m_X2, m_Y2);
}  /* End "dr_recfl". */

#if 0
/* transform form entry code */
r_trnfm()
{
  TRAN_FM();
} /* end of trnsform form */
/* exchange timer vector */
dex_timv()
{
  EX_TIMV();
} /* end of timer vector */

#endif


int SdSdl::sfUdPat(long count, unsigned long *pattern)
{
	int planes = count / 16;
	int n;

	if (count % 16) return -1;	// Not an exact number of planes

	if (planes == 1)
	{
		m_udpt_np = 0;
		for (n = 0; n < 16; n++)
		{
			m_UD_PATRN[n] = pattern[n];
		}
	}
	else
	{
		// XXX colour user-defined pattern not supported
		return -1;
	}	
	return 0;	
}
