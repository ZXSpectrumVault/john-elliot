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

/*History
Fix #	Name	Date	Fix description
1	DH	5/29/85	Polyline had edge shorten flag inverted (LSTLIN)
*/
#include "sdsdl.hxx"


/* POLYLINE: */
void SdSdl::polyLine(long count, const GPoint *pt)
{
	Sint32 pln_sts;
	if ( m_globals->m_HIDE_CNT == 0 )
	{
	    pln_sts = 1;
	    HIDE_CUR();
	}
	else pln_sts = 0;	

        m_LN_RMASK = 0x80000000L;
	m_LN_MASK  = m_line_sty[m_line_index];
	m_FG_BP_1  = m_line_pixel;

	if (m_line_width == 1)
	{
		pline(count, pt);
		if ( (m_line_beg | m_line_end ) & ARROWED )
			do_arrow(count, pt);
	}  /* End if:  normal polyline. */
	else	wline(count, pt);

	if ( pln_sts == 1 ) DIS_CUR();
}

void SdSdl::polyMarker(long count, const GPoint *pt)
{
#define MARKSEGMAX 5

  static Sint32 *markhead[] =
  {
    m_dot, m_plus, m_star, m_square, m_cross, m_dmnd
  };

  Sint32 i, j, k, num_lines, num_vert;
  Sint32 sav_index, sav_color, sav_width, sav_beg, sav_end, sav_clip;
  Sint32 *mark_ptr;
  GPoint markpt[MARKSEGMAX], centre; 
  Uint32 sav_pixel;

  /* Save the current polyline attributes which will be used. */
  sav_index = m_line_index;
  sav_color = m_line_color;
  sav_pixel = m_line_pixel;
  sav_width = m_line_width;
  sav_beg = m_line_beg;
  sav_end = m_line_end;
  sav_clip = m_CLIP;

  /* Set the appropriate polyline attributes. */
  m_line_index = 0;
  m_line_color = m_mark_color;
  m_line_pixel = m_mark_pixel;
  m_line_width = 1;
  m_line_beg = 0;
  m_line_end = 0;
  m_CLIP = 1;

  /* Copy the PTSIN values which will be overwritten by the polyline arrays. */
  num_vert = count;

  /* Loop over the number of points. */
  for (i = 0; i < num_vert; i++)
  {
    /* Get the (x, y) position for the marker. */
    centre.x = pt[i].x;
    centre.y = pt[i].y;

    /* Get the pointer to the appropriate marker type definition. */
    mark_ptr = markhead[m_mark_index];
    num_lines = *mark_ptr++;

    /* Loop over the number of polylines which define the marker. */
    for (j = 0; j < num_lines; j++)
    {
      /* How many points?  Get them. */
      count = *mark_ptr++;
      for (k = 0; k < count; k++)
      {
        markpt[k].x = centre.x + m_mark_scale*(*mark_ptr++);
        markpt[k].y = centre.y + m_mark_scale*(*mark_ptr++);
      }  /* End for:  extract points. */

      /* Output the polyline. */
      polyLine(count, markpt);
    }  /* End for:  over the number of polylines defining the marker. */
  update_rect(centre.x - 5 * m_mark_scale, centre.y - 5 * m_mark_scale,
	    centre.x + 5 * m_mark_scale, centre.y + 5 * m_mark_scale);
  }  /* End for:  over marker points. */

  /* Restore the current polyline attributes. */
  m_line_index = sav_index;
  m_line_color = sav_color;
  m_line_pixel = sav_pixel;
  m_line_width = sav_width;
  m_line_beg = sav_beg;
  m_line_end = sav_end;
  m_CLIP = sav_clip;
}  /* End "v_pmarker". */

/* FILLED_AREA: */
void SdSdl::fillArea(long count, const GPoint *pt)
{
	plygn(count, pt);
}

/*  GDP: */
void SdSdl::gdp(long i, long pcount, const GPoint *pt, long icount,
		                        const long *ival, wchar_t *text)
{
// Ignore obviously bogus input 
    if (icount > 0 && ival == NULL) return;
    if (pcount > 0 && pt   == NULL) return;

    Uint32 ltmp_end,rtmp_end;
    if (( i > 0 ) && ( i < 11 )) 
    {  
	i--;
	switch ( i )
	{
	    case 0:	/* GDP BAR */
	      if (pcount < 2) return;
	      fillRect(pt);
	      if ( m_fill_per == TRUE )
	      {
			GPoint lpt[5];
			m_LN_MASK = 0xffffffff;
			lpt[0].x = lpt[3].x = lpt[4].x = pt[0].x;
			lpt[1].x = lpt[2].x = pt[1].x;
			lpt[0].y = lpt[1].y = lpt[4].y = pt[0].y;
			lpt[2].y = lpt[3].y = pt[1].y; 
			pline(5, lpt);
	      }
	      break;	
	    case 1: /* GDP ARC */
	      if (pcount < 4 || icount < 2) return;
	      gdp_arc(1+i, pt, ival);
	      break;

	    case 2: /* GDP PIE */
	      if (pcount < 4 || icount < 2) return;
	      gdp_arc(1+i, pt, ival);
	      break;
	 	
	    case 3: /* GDP CIRCLE */
	      if (pcount < 2) return;
	      m_xc = pt[0].x; 
	      m_yc = pt[0].y;
	      m_xrad = pt[2].x;
	      m_yrad = SMUL_DIV (m_xrad, xsize, ysize ); 
	      m_del_ang = 3600;
	      m_beg_ang = 0;
	      m_end_ang = 3600;
	      clc_nsteps();
	      clc_arc(1+i, pt);
	      break;

	    case 4: /* GDP ELLIPSE */
	      if (pcount < 2) return;
	      m_xc   = pt[0].x; m_yc = pt[0].y;
	      m_xrad = pt[1].x;
	      m_yrad = pt[1].y;
	      if ( m_xfm_mode < 2 ) /* if xform != raster then flip */
	        m_yrad = m_globals->m_yres - m_yrad;	
	      m_del_ang = 3600;
	      m_beg_ang = 0;
	      m_end_ang = 0;
	      clc_nsteps();
	      clc_arc(1+i, pt);
	      break;
		
	    case 5: /* GDP ELLIPTICAL ARC */
	      if (pcount < 4 || icount < 2) return;
	      gdp_ell(1+i, pt, ival);
	      break;
		
	    case 6: /* GDP ELLIPTICAL PIE */
	      if (pcount < 4 || icount < 2) return;
	      gdp_ell(1+i, pt, ival);
	      break;

	    case 7: /* GDP Rounded Box */
	      if (pcount < 2) return;
	      ltmp_end = m_line_beg;
	      m_line_beg = SQUARED;
	      rtmp_end = m_line_end;
	      m_line_end = SQUARED;
	      gdp_rbox(1+i, pt);
	      m_line_beg = ltmp_end;
	      m_line_end = rtmp_end; 	
	      break;

	    case 8: /* GDP Rounded Filled Box */
	      if (pcount < 2) return;
	      gdp_rbox(1+i, pt);
              break;

	    case 9: /* GDP Justified Text */
	      if (pcount < 2 || icount < 2 || text == NULL) return;
	      d_justified(pt, text, ival[0], ival[1]);
              break;

	    }
	}
}    	


/* INQUIRE CURRENT POLYLINE ATTRIBUTES */
void SdSdl::queryLine(long *styles, GPoint *width)
{
	styles[0] = m_line_qi;
	styles[1] = m_line_qc;
	styles[2] = m_write_qm;
	styles[3] = m_line_beg;
	styles[4] = m_line_end;
	width->x  = m_line_qw;
	width->y  = 0;
}

/* INQUIRE CURRENT Polymarker ATTRIBUTES */
void SdSdl::queryMarker(long *styles, GPoint *height)
{
	styles[0] = m_mark_qi;
	styles[1] = m_mark_qc;
	styles[2] = m_write_qm;
  	height->x = m_mark_scale*DEF_MKWD;
  	height->y = m_mark_scale*DEF_MKHT;
	m_FLIP_Y = 1;
}

/* INQUIRE CURRENT Fill Area ATTRIBUTES */
void SdSdl::queryFill(long *styles)
{
	styles[0] = m_fill_style;
	styles[1] = m_fill_qc;
	styles[2] = m_fill_qi;
	styles[3] = m_write_qm;
	styles[4] = m_fill_per;
}


void SdSdl::pline(long count, const GPoint *pt)
{
	Sint32	i,j;
	Sint32	xmin, xmax, ymin, ymax;

	xmin = xmax = pt[0].x;
	ymin = ymax = pt[0].y;

	if (lock_surface()) return;

	j = 0;
	m_LSTLIN = FALSE;
	for (i = (count - 1);i > 0;i--)
	{
		if (i == 1)	m_LSTLIN = TRUE;
		m_X1 = pt[j].x;
		m_Y1 = pt[j++].y;
		m_X2 = pt[j].x;
		m_Y2 = pt[j].y;

		if (m_X2 < xmin) xmin = m_X2;
		if (m_X2 > xmax) xmax = m_X2;
		if (m_Y2 < ymin) ymin = m_Y2;
		if (m_Y2 > ymax) ymax = m_Y2;

		if ( m_CLIP )
		{   
	        	if ( clip_line() ) ABLINE();
	    	} 
		else ABLINE();
	}
	unlock_surface();
	xmin -= MAX_L_WIDTH; if (xmin < 0) xmin = 0;
	xmax += MAX_L_WIDTH; if (xmax >= xres) xmax = xres - 1;
  
	ymin -= MAX_L_WIDTH; if (ymin < 0) ymin = 0;
	ymax += MAX_L_WIDTH; if (ymax >= yres) ymax = yres - 1;

	update_rect(xmin, ymin, xmax, ymax);
}

int SdSdl::clip_line()
{
	Sint32	deltax, deltay;
	int	x1y1_clip_flag, x2y2_clip_flag, line_clip_flag;
	Sint32	*x,*y;

	while (( x1y1_clip_flag = code(m_X1, m_Y1 )) | 
		( x2y2_clip_flag = code( m_X2, m_Y2)))
	{
	    if ( ( x1y1_clip_flag & x2y2_clip_flag )) return( FALSE );

	    if ( x1y1_clip_flag )
	    {
		line_clip_flag = x1y1_clip_flag;
		x = &m_X1; y = &m_Y1;
	    }
  	    else
	    {
		line_clip_flag = x2y2_clip_flag;
		x = &m_X2; y = &m_Y2;
	    }
	    deltax = m_X2 - m_X1; deltay = m_Y2 - m_Y1;
	    if ( line_clip_flag & 1 )	/* left ? */
	    {
		*y = m_Y1 + SMUL_DIV( deltay, 
					(m_XMN_CLIP - m_X1), deltax );
		*x = m_XMN_CLIP;
	    }
	    else if ( line_clip_flag & 2 )	/* right ? */
	    {
		*y = m_Y1 + SMUL_DIV( deltay, 
					(m_XMX_CLIP - m_X1), deltax );
		*x = m_XMX_CLIP;
	    }
	    else if ( line_clip_flag & 4 )	/* top ? */
	    {
		*x = m_X1 + SMUL_DIV( deltax, 
					(m_YMN_CLIP - m_Y1), deltay );
		*y = m_YMN_CLIP;
	    }
	    else if ( line_clip_flag & 8 )	/* bottom ? */
	    {
		*x = m_X1 + SMUL_DIV( deltax, 
					(m_YMX_CLIP - m_Y1), deltay );
		*y = m_YMX_CLIP;
	    }
	}
	return( TRUE );		/* segment now clipped  */
}

int SdSdl::code( Sint32 x, Sint32 y )
{
	int	clip_flag = 0;

	if      ( x < m_XMN_CLIP ) clip_flag  = 1;
	else if ( x > m_XMX_CLIP ) clip_flag  = 2;
	if      ( y < m_YMN_CLIP ) clip_flag |= 4;
	else if ( y > m_YMX_CLIP ) clip_flag |= 8;
	return ( clip_flag );
}


void SdSdl::plygn(long count, const GPoint *x_pt)	
{
	Sint32 	i,k;
	GPoint *pt = new GPoint[(count + 1)];

	if (!pt) return;	// Out of memory
	memcpy(pt, x_pt, count * sizeof(GPoint));
	pt[count].x = pt[0].x;
	pt[count].y = pt[0].y;

	m_FG_BP_1 = m_fill_pixel;
	m_LSTLIN  = FALSE;

	m_fill_minx = m_fill_maxx = pt[0].x;
	m_fill_miny = m_fill_maxy = pt[0].y;
	for (i = 1; i < count; i++)
	{
          k = pt[i].x;
	  if      ( k < m_fill_minx ) m_fill_minx = k;
	  else if ( k > m_fill_maxx ) m_fill_maxx = k;
	  k = pt[i].y;
	  if      ( k < m_fill_miny ) m_fill_miny = k;
	  else if ( k > m_fill_maxy ) m_fill_maxy = k;
	}
	k = m_fill_miny; 
	if ( m_CLIP )
	{
	    if ( m_fill_miny < m_YMN_CLIP )		
	    {
	  	if ( m_fill_maxy >= m_YMN_CLIP )
				    	/* plygon starts before clip */
		    m_fill_miny = m_YMN_CLIP;	
					/* plygon partial overlap */
		else { delete pt; return; } /* plygon entirely before clip */
	    }
	    if ( m_fill_maxy > m_YMX_CLIP )
	    {
		if ( m_fill_miny <= m_YMX_CLIP )
						/* plygon ends after clip */
		    m_fill_maxy = m_YMX_CLIP;
						/* plygon partial overlap */
		else { delete pt; return; }	/* plygon entirely after clip */ 
	    }
	    if ( m_fill_miny != k )		/* plygon fills n -1 scans */
		--m_fill_miny;
        }
	for (m_Y1 = m_fill_maxy; m_Y1 > m_fill_miny; m_Y1--)
	{
	    m_fill_intersect = 0;
	    CLC_FLIT(count, pt);
	}
	if ( m_fill_per == TRUE )
	{
	    m_LN_MASK = 0xffffffff;
	    pline(1 + count, pt);
	}
	else update_rect(m_fill_minx, m_fill_miny,
			 m_fill_maxx, m_fill_maxy);
	delete pt;
}


void SdSdl::gdp_rbox(int type, const GPoint *ptin)
{
	GPoint pt[30];
	Sint32	i,j, rdeltax, rdeltay;

        arb_corner(ptin, pt, LLUR);
	m_X1 = pt[0].x; m_Y1 = pt[0].y;
	m_X2 = pt[1].x; m_Y2 = pt[1].y;
	rdeltax = (m_X2 - m_X1)/2; 
	rdeltay = (m_Y1 - m_Y2)/2;
	m_xrad = m_globals->m_xres >> 6;
	if ( m_xrad > rdeltax )
	    m_xrad = rdeltax;
	m_yrad = SMUL_DIV( m_xrad, xsize, ysize );
	if ( m_yrad > rdeltay )
	    m_yrad = rdeltay;
	pt[0].x =0; pt[0].y = m_yrad;
	pt[1].x = SMUL_DIV ( Icos(675), m_xrad, 32767 ) ;
	pt[1].y = SMUL_DIV ( Isin(675), m_yrad, 32767 ) ;
	pt[2].x = SMUL_DIV ( Icos(450), m_xrad, 32767 ) ;
	pt[2].y = SMUL_DIV ( Isin(450), m_yrad, 32767 ) ;
	pt[3].x = SMUL_DIV ( Icos(225), m_xrad, 32767 ) ;
	pt[3].y = SMUL_DIV ( Isin(225), m_yrad, 32767 ) ;
	pt[4].x = m_xrad;
	pt[4].y = 0;
	m_xc = m_X2 - m_xrad; 
	m_yc = m_Y1 - m_yrad;
	j = 5;
	for ( i = 4; i >= 0 ; i-- )
	{ 
		pt[j].y = m_yc + pt[i].y;
		pt[j].x = m_xc + pt[i].x;
		++j;
	}
	m_xc = m_X1 + m_xrad; 
	j = 10;
	for ( i = 0; i < 5; i++ )
	{ 
		pt[j].x = m_xc - pt[i].x;
		pt[j].y = m_yc + pt[i].y;
		++j;
	}
	m_yc = m_Y2 + m_yrad;
	j = 15;
	for ( i = 4; i >= 0; i-- )
	{ 
		pt[j].y = m_yc - pt[i].y;
		pt[j].x = m_xc - pt[i].x;
	  	++j; 
	}
	m_xc = m_X2 - m_xrad;
	j = 0;
	for ( i = 0; i < 5; i++ )
	{ 
		pt[j].x = m_xc + pt[i].x;
		pt[j].y = m_yc - pt[i].y;
		++j;
	}
	pt[20] = pt[0];
    	if (type == 8) 
	{
	  m_LN_MASK = m_line_sty[m_line_index];
	  m_FG_BP_1 = m_line_pixel;

	  if (m_line_width == 1) pline(21, pt);
	  else			 wline(21, pt);
	}
    	else plygn(20, pt);

	return;
}


void SdSdl::gdp_arc(int filled, const GPoint *pt, const long *angles)
{
	m_beg_ang = angles[0];
	m_end_ang = angles[1];
	m_del_ang = m_end_ang - m_beg_ang;
	if ( m_del_ang < 0 )
	    m_del_ang += 3600; 
	m_xrad = pt[3].x;
	m_yrad = SMUL_DIV ( m_xrad, xsize, ysize );
	clc_nsteps();
	m_n_steps = SMUL_DIV ( m_del_ang, m_n_steps, 3600 );
	if ( m_n_steps == 0 )
	  return;
	m_xc = pt[0].x; m_yc = pt[0].y;
	clc_arc(filled, pt);
	return;
}


void SdSdl::clc_nsteps()
{
	if ( m_xrad > m_yrad )
	    m_n_steps = m_xrad;
	else
	    m_n_steps = m_yrad;
	m_n_steps = m_n_steps >> 2;
	if ( m_n_steps < 16 )
	    m_n_steps = 16;
	else
	{
	    if ( m_n_steps > MAX_ARC_CT )
	        m_n_steps = MAX_ARC_CT;
	}
}

void SdSdl::gdp_ell(int filled, const GPoint *pt, const long *angles)
{
	m_beg_ang = angles[0];
	m_end_ang = angles[1];
	m_del_ang = m_end_ang - m_beg_ang;
	if ( m_del_ang < 0 )
	    m_del_ang += 3600;
	m_xrad = pt[1].x;
	m_yrad = pt[1].y;
	if ( m_xfm_mode < 2 ) /* if xform != raster then flip */
	  m_yrad = m_globals->m_yres - m_yrad;	
	clc_nsteps();	
	m_n_steps = SMUL_DIV ( m_del_ang, m_n_steps, 3600 );
	if ( m_n_steps == 0 )
	  return;
	m_xc = pt[0].x;
	m_yc = pt[0].y;
	clc_arc(filled, pt);
}


void SdSdl::clc_arc(int type, const GPoint *pt)
{
	int i,j;

	if ( m_CLIP )
	{
		if ( (( m_xc + m_xrad ) < m_XMN_CLIP) || 
		     (( m_xc - m_xrad ) > m_XMX_CLIP) ||
		     (( m_yc + m_yrad ) < m_YMN_CLIP) || 
		     (( m_yc -  m_yrad) >  m_YMX_CLIP))
		return;
	}
	m_start = m_angle = m_beg_ang ;	
    	i = j = 0 ;
	GPoint *pts = new GPoint[m_n_steps + 3];
    	Calc_pts(j, pts);
	for ( i = 1; i < m_n_steps; i++ )
    	{
	  j++;
	  m_angle = SMUL_DIV( m_del_ang, i , m_n_steps ) + m_start;
          Calc_pts(j, pts);
        }
	j++;
    	i = m_n_steps ;
    	m_angle = m_end_ang ;
    	Calc_pts(j, pts);

/*----------------------------------------------------------------------*/
/* If pie wedge	draw to center and then close. If arc or circle, do 	*/
/* nothing because loop should close circle.				*/

        int iptscnt = m_n_steps + 1 ;	/* since loop in Clc_arc starts at 0 */
	if ((type == 3)||(type == 7)) /* pie wedge */
	{
	  m_n_steps++;
	  j ++;
	  pts[j].x = m_xc;
	  pts[j].y = m_yc;
	  iptscnt = m_n_steps+1;
	}
    	if ((type == 2) || ( type == 6 )) /* open arc */
          polyLine(iptscnt, pts);
    	else
	  fillArea(iptscnt, pts);
	delete pts;
}

void SdSdl::Calc_pts(int j, GPoint *pt) 
{
	Sint32	k;
	k = SMUL_DIV ( Icos(m_angle), m_xrad, 32767 ) + m_xc ;
    	pt[j].x = k;
/*	if ( xfm_mode < 2 )
*/	    k = m_yc - SMUL_DIV ( Isin( m_angle), m_yrad, 32767 ) ;
/*	else
	    k = yc + SMUL_DIV ( Isin( angle), yrad, 32767 ) ;
*/	pt[j].y = k;	
}

void SdSdl::st_fl_ptr()
{
	m_patmsk = 0;
	switch ( m_fill_style )
	{
		case 0: m_patptr = HOLLOW; break;

		case 1: m_patptr = SOLID; break;

		case 2: if ( m_fill_index < 8 )
			{
			    m_patmsk = DITHRMSK;
			    m_patptr = &DITHER[ m_fill_index * 
                                                   (m_patmsk+1) ];
			}
			else
			{
			    m_patmsk = OEMMSKPAT;
			    m_patptr = &OEMPAT[ (m_fill_index - 8) * 
                                                    (m_patmsk+1) ]; 
			}
			break;
		case 3:
			if ( m_fill_index < 6 )
			{
			    m_patmsk = HAT_0_MSK;
			    m_patptr = &HATCH0[ m_fill_index * 
                                                   (m_patmsk+1) ];
			}
			else
			{
			    m_patmsk = HAT_1_MSK;
			    m_patptr = &HATCH1[ (m_fill_index - 6) * 
						    (m_patmsk+1) ];
			}
			break;
		case 4: m_patmsk = 0x000f;	// XXX should be 1F?
			m_patptr = m_UD_PATRN;
			break;
	}
}


#ifndef ABS
# define ABS(x) ((x) >= 0 ? (x) : -(x))
#endif

void SdSdl::wline(long count, const GPoint *pt)
{
  Sint32 i, j, k;
  Sint32 numpts;
  GPoint wp1, wp2, vp;
  GPoint ppts[4];
  Sint32 xmin, xmax, ymin, ymax;

  /* Don't attempt wide lining on a degenerate polyline. */
  if ( (numpts = count) < 2) return;

  /* If the ends are arrowed, output them. */
  if ( (m_line_beg | m_line_end) & ARROWED)
    do_arrow(count, pt);

  /* Set up the attribute environment.  Save attribute globals which need */
  /* to be saved.                                                         */
  s_fa_attr();

  /* Initialize the starting point for the loop. */
  j = 0;
  xmin = xmax = wp1.x = pt[j  ].x;
  ymin = ymax = wp1.y = pt[j++].y;

  /* If the end style for the first point is not squared, output a circle. */
  if (m_s_begsty != SQUARED)
    do_circ(&wp1);

  /* Loop over the number of points passed in. */
  for (i = 1; i < numpts; i++)
  {
    /* Get the ending point for the line segment and the vector from the */
    /* start to the end of the segment.                                  */
    wp2.x = pt[j  ].x;
    wp2.y = pt[j++].y;

    if (wp2.x < xmin) xmin = wp2.x;
    if (wp2.x > xmax) xmax = wp2.x;
    if (wp2.y < ymin) ymin = wp2.y;
    if (wp2.y > ymax) ymax = wp2.y;

    vp.x = wp2.x - wp1.x;
    vp.y = wp2.y - wp1.y;

    /* Ignore lines of zero length. */
    if ( (vp.x == 0) && (vp.y == 0) ) continue;

    /* Calculate offsets to fatten the line.  If the line segment is */
    /* horizontal or vertical, do it the simple way.                 */
    if (vp.x == 0)
    {
      vp.x = m_q_circle[0];
      vp.y = 0;
    }  /* End if:  vertical. */

    else if (vp.y == 0)
    {
      vp.x = 0;
      vp.y = m_num_qc_lines - 1;
    }  /* End else if:  horizontal. */

    else
    {
      /* Find the offsets in x and y for a point perpendicular to the line */
      /* segment at the appropriate distance.                              */
      k = SMUL_DIV(-vp.y, ysize, xsize);
      vp.y = SMUL_DIV(vp.x, xsize, ysize);
      vp.x = k;
      perp_off(&vp);
    }  /* End else:  neither horizontal nor vertical. */

    /* Prepare the control and points parameters for the polygon call. */
    ppts[0].x = wp1.x + vp.x;
    ppts[0].y = wp1.y + vp.y;
    ppts[1].x = wp1.x - vp.x;
    ppts[1].y = wp1.y - vp.y;
    ppts[2].x = wp2.x - vp.x;
    ppts[2].y = wp2.y - vp.y;
    ppts[3].x = wp2.x + vp.x;
    ppts[3].y = wp2.y + vp.y;
    plygn(4, ppts);

    /* If the terminal point of the line segment is an internal joint, */
    /* or the end style is not squared, output a filled circle.        */
    if ( (m_s_endsty != SQUARED) || (i < numpts - 1) )
      do_circ(&wp2);

    /* The line segment end point becomes the starting point for the next */
    /* line segment.                                                      */
    wp1.x = wp2.x;
    wp1.y = wp2.y;
  }  /* End for:  over number of points. */

  xmin -= MAX_L_WIDTH; if (xmin < 0) xmin = 0;
  xmax += MAX_L_WIDTH; if (xmax >= xres) xmax = xres - 1;

  ymin -= MAX_L_WIDTH; if (ymin < 0) ymin = 0;
  ymax += MAX_L_WIDTH; if (ymax >= yres) ymax = yres - 1;

  update_rect(xmin, ymin, xmax, ymax);


  /* Restore the attribute environment. */
  r_fa_attr();
}  /* End "wline". */



void SdSdl::perp_off(GPoint *vxy)
{
  Sint32 u, v, quad, magnitude, min_val;
  GPoint xy, val;

  /* Mirror transform the vector so that it is in the first quadrant. */
  if (vxy->x >= 0) quad = (vxy->y >= 0) ? 1 : 4;
  else	 	   quad = (vxy->y >= 0) ? 2 : 3;

  quad_xform(quad, vxy, &xy);

  /* Traverse the circle in a dda-like manner and find the coordinate pair   */
  /* (u, v) such that the magnitude of (u*y - v*x) is minimized.  In case of */
  /* a tie, choose the value which causes (u - v) to be minimized.  If not   */
  /* possible, do something.                                                 */
  min_val = 0x7FFFFFFFL;
  u = m_q_circle[0];
  v = 0;
  while(1)
  {
    /* Check for new minimum, same minimum, or finished. */
    if ( ( (magnitude = ABS(u*xy.y - v*xy.x)) < min_val ) ||
         ( (magnitude == min_val) && (ABS(val.x - val.y) > ABS(u - v) ) ) )
    {
      min_val = magnitude;
      val.x = u;
      val.y = v;
    }  /* End if:  new minimum. */

    else break;

    /* Step to the next pixel. */
    if (v == m_num_qc_lines - 1)
    {
      if (u == 1) break;
      else u--;
    }  /* End if:  doing top row. */

    else
    {
      if (m_q_circle[v + 1] >= u - 1)
      {
        v++;
        u = m_q_circle[v];
      }  /* End if:  do next row up. */
      else
      {
        u--;
      }  /* End else:  continue on row. */
    }  /* End else:  other than top row. */
  }  /* End FOREVER loop. */

  /* Transform the solution according to the quadrant. */
  quad_xform(quad, &val, vxy);
}  /* End "perp_off". */


void SdSdl::quad_xform(int quad, GPoint *xy, GPoint *txy)
{
	switch (quad)
	{
		case 1: case 4: txy->x =   xy->x;  break;
		case 2: case 3: txy->x = (-xy->x); break;
	}
	switch(quad)
	{
		case 1: case 2: txy->y =   xy->y;  break;
		case 3: case 4: txy->y = (-xy->y); break;
	}
}



void SdSdl::do_circ(GPoint *pt)
{
  Sint32 k;

  /* Only perform the act if the circle has radius. */
  if (m_num_qc_lines > 0)
  {
    /* Do the horizontal line through the center of the circle. */
    m_X1 = pt->x - m_q_circle[0];
    m_X2 = pt->x + m_q_circle[0];
    m_Y1 = m_Y2 = pt->y;
    if (clip_line() ) ABLINE();

    /* Do the upper and lower semi-circles. */
    for (k = 1; k < m_num_qc_lines; k++)
    {
      /* Upper semi-circle. */
      m_X1 = pt->x - m_q_circle[k];
      m_X2 = pt->x + m_q_circle[k];
      m_Y1 = m_Y2 = pt->y - k;
      if (clip_line() ) ABLINE();

      /* Lower semi-circle. */
      m_X1 = pt->x - m_q_circle[k];
      m_X2 = pt->x + m_q_circle[k];
      m_Y1 = m_Y2 = pt->y + k;
      if (clip_line() ) ABLINE();
    }  /* End for. */
  }  /* End if:  circle has positive radius. */
}  /* End "do_circ". */


void SdSdl::s_fa_attr()
{
  /* Set up the fill area attribute environment. */
  m_LN_RMASK = 0x80000000L;
  m_LN_MASK = m_line_sty[0];
  m_fill_color = m_line_color;
  m_fill_pixel = m_line_pixel;
  m_s_fill_per = m_fill_per;
  m_fill_per = TRUE;
  m_s_patptr = m_patptr;
  m_s_nxtpat = m_NEXT_PAT;
  m_NEXT_PAT = 0;	
  m_patptr = SOLID;
  m_s_patmsk = m_patmsk;
  m_patmsk = 0;
  m_s_begsty = m_line_beg;
  m_s_endsty = m_line_end;
  m_line_beg = m_line_end = SQUARED;
}  /* End "s_fa_attr". */


void SdSdl::r_fa_attr()
{
  /* Restore the fill area attribute environment. */
  m_NEXT_PAT = m_s_nxtpat;
  m_fill_color = m_fill_qc;
  m_fill_pixel = m_globals->MAP_COL(m_fill_qc);
  m_fill_per = m_s_fill_per;
  m_patptr = m_s_patptr;
  m_patmsk = m_s_patmsk;
  m_line_beg = m_s_begsty;
  m_line_end = m_s_endsty;
}  /* End "r_fa_attr". */



void SdSdl::do_arrow(long count, const GPoint *pt)
{
  Sint32 x_start, y_start, new_x_start, new_y_start;

  // Create a working copy to avoid problems with const 
  GPoint *pt2 = new GPoint[count];
  if (!pt2) return;

  memcpy(pt2, pt, count * sizeof(GPoint));

  /* Set up the attribute environment. */
  s_fa_attr();

  /* Function "arrow" will alter the end of the line segment.  Save the */
  /* starting point of the polyline in case two calls to "arrow" are    */
  /* necessary.                                                         */
  new_x_start = x_start = pt2[0].x;
  new_y_start = y_start = pt2[0].y;

  if (m_s_begsty & ARROWED)
  {
    arrow(&pt2[0], 1, count, pt2);
    new_x_start = pt2[0].x;
    new_y_start = pt2[0].y;
  }  /* End if:  beginning point is arrowed. */

  if (m_s_endsty & ARROWED)
  {
    pt2[0].x = x_start;
    pt2[0].y = y_start;
    arrow(&pt2[count-1], -1, count, pt2);
    pt2[0].x = new_x_start;
    pt2[1].x = new_y_start;
  }  /* End if:  ending point is arrowed. */

  /* Restore the attribute environment. */
  r_fa_attr();
  delete pt2;
  // XXX Are the side-effects on pt[] actually needed anywhere? If so,
  //    we need to move pt2[] up to the routines that call this one.
}  /* End "do_arrow". */


void SdSdl::arrow(GPoint *xy, Sint32 inc, long count, GPoint *pt)
{
  Sint32 i, arrow_len, arrow_wid, line_len;
  Sint32 dx, dy;
  Sint32 base_x, base_y, ht_x, ht_y;
  GPoint *xybeg;
  GPoint triangle[3];

  /* Set up the arrow-head length and width as a function of line width. */
  arrow_wid = (arrow_len = (m_line_width == 1) ? 8 : 3*m_line_width - 1)/2;

  /* Initialize the beginning pointer. */
  xybeg = xy;

  /* Find the first point which is not so close to the end point that it */
  /* will be obscured by the arrowhead.                                  */
  for (i = 1; i < count; i++)
  {
    /* Find the deltas between the next point and the end point.  Transform */
    /* to a space such that the aspect ratio is uniform and the x axis      */
    /* distance is preserved.                                               */
    xybeg += inc;
    dx = xy->x - xybeg->x;
    dy = SMUL_DIV(xy->y - xybeg->y, ysize, xsize);

    /* Get the length of the vector connecting the point with the end point. */
    /* If the vector is of sufficient length, the search is over.            */
    if ( (line_len = vec_len(ABS(dx), ABS(dy))) >= arrow_len)
      break;
  }  /* End for:  over i. */

  /* If the longest vector is insufficiently long, don't draw an arrow. */
  if (line_len < arrow_len)
    return;

  /* Rotate the arrow-head height and base vectors.  Perform calculations */
  /* in 1000x space.                                                      */
  ht_x = SMUL_DIV(arrow_len, SMUL_DIV(dx, 1000, line_len), 1000);
  ht_y = SMUL_DIV(arrow_len, SMUL_DIV(dy, 1000, line_len), 1000);
  base_x = SMUL_DIV(arrow_wid, SMUL_DIV(dy, -1000, line_len), 1000);
  base_y = SMUL_DIV(arrow_wid, SMUL_DIV(dx, 1000, line_len), 1000);

  /* Transform the y offsets back to the correct aspect ratio space. */
  ht_y = SMUL_DIV(ht_y, xsize, ysize);
  base_y = SMUL_DIV(base_y, xsize, ysize);

  /* Build a polygon to send to plygn.  Build into a local array first since */
  /* xy will probably be pointing to the PTSIN array.                        */

  triangle[0].x = xy->x + base_x - ht_x;
  triangle[0].y = xy->y + base_y - ht_y;
  triangle[1].x = xy->x - ht_x;
  triangle[1].y = xy->y - base_y - ht_y;
  triangle[2].x = xy->x;
  triangle[2].y = xy->y;
  plygn(3, triangle);

  /* Adjust the end point and all points skipped. */
  xy->x -= ht_x;
  xy->y -= ht_y;
  while ( (xybeg -= inc) != xy)
  {
    xybeg->x = xy->x;
    xybeg->y = xybeg->y;
  }  /* End while. */
}  /* End "arrow". */



void SdSdl::init_wk(const GWSParams *wsp, GWSResult *wsr)
{
	Sint32	i;

	m_line_qi = wsp->defaults[0];
        if ((m_line_qi > MAX_LINE_STYLE) || (m_line_qi < 1))
                m_line_qi = 1;
	
	m_line_index = ( m_line_qi - 1);

	m_line_qc = wsp->defaults[1];
	m_line_color = m_line_qc;
	if ((m_line_color >= MAX_COLOR) || (m_line_color < 0))
		m_line_color = 1; 
		m_line_pixel = m_globals->MAP_COL(m_line_color);

	m_mark_qi = wsp->defaults[2];
	if ((m_mark_qi > MAX_MARK_INDEX) || (m_mark_qi < 1))
		m_mark_qi = 3;
	m_mark_index = m_mark_qi - 1;

	m_mark_color = wsp->defaults[3];
	if ((m_mark_color >= MAX_COLOR) || (m_mark_color < 0))
		m_mark_color = 1; 
	m_mark_qc = m_mark_color;
	m_mark_pixel = m_globals->MAP_COL( m_mark_color );

	m_mark_height = DEF_MKHT;
	m_mark_scale = 1;

	m_fill_style = wsp->defaults[6];
	if ((m_fill_style > MX_FIL_STYLE) || (m_fill_style < 0))
		m_fill_style = 0; 

	m_NEXT_PAT = 0;
	m_fill_qi = wsp->defaults[7];
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

	m_fill_color = wsp->defaults[8];
	if ((m_fill_color >= MAX_COLOR) || (m_fill_color < 0))
		m_fill_color = 1; 
	m_fill_qc = m_fill_color;
	m_fill_pixel = m_globals->MAP_COL(m_fill_color );

	m_xfm_mode = wsp->xymode;

	st_fl_ptr();
	m_WRT_MODE  = 0; 		/* default is replace mode	*/
	m_write_qm = 1;
	m_line_qw = m_line_width = DEF_LWID;
	m_line_beg = m_line_end = 0;	
					/* default to squared ends	*/
	m_loc_mode = 0;		/* default is request mode	*/
	m_val_mode = 0;    		/* default is request mode	*/
	m_chc_mode = 0; 		/* default is request mode	*/
	m_str_mode = 0;		/* default is request mode	*/
	m_fill_per = TRUE;		/* set fill perimeter on */
	m_globals->m_HIDE_CNT = 1;	/* turn the cursor off as default */
	m_XMN_CLIP = 0;
	m_YMN_CLIP = 0;
	m_XMX_CLIP = xres;
	m_YMX_CLIP = yres;
	m_CLIP = 0;
	m_FLIP_Y = 1;

	m_bg_pixel = m_globals->MAP_COL(0);

	for (i = 0; i < 45; i++)
	{
		wsr->data[i]   = m_globals->m_DEV_TAB[i];
	}
	for (i = 0; i < 6; i++)
	{
		wsr->points[i].x = m_globals->m_SIZ_TAB[i].x;
		wsr->points[i].y = m_globals->m_SIZ_TAB[i].y;
	}
	wsr->data[0] = m_globals->m_requestW - 1;	
	wsr->data[1] = m_globals->m_requestH - 1;	
}
