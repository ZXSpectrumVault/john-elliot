/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/

#include "fontdef.h"
/* Variables specific to a virtual workstation */

typedef struct drvdata
{
	DrvGlobals *globals;	/* Variables common to all virt. workstations */

	Uint32		bg_pixel;	/* Background colour */

	/* Line variables */
	CORBA_long	line_index,line_color, line_qi,line_qc,line_width;
	CORBA_long	line_qw;
	Uint32		line_pixel;
	/* Line "work" variables */
	Uint32		LN_RMASK;
	CORBA_long	LN_MASK, LSTLIN, X1, X2, Y1, Y2;
	CORBA_long	num_qc_lines, q_circle[MAX_L_WIDTH];
	CORBA_long	line_beg, line_end, yinc;
	CORBA_long	BOX_MODE;
	Uint32		FG_BP_1;
	/* Patterns */
	CORBA_long	NEXT_PAT;
	CORBA_long	line_sty[MAX_LINE_STYLE];
#define ud_lstyl line_sty[MAX_LINE_STYLE - 1]
	

	/* Marker variables */
	CORBA_long	mark_height,mark_scale,mark_index,mark_color,mark_qi;
	CORBA_long	mark_qc;
	Uint32		mark_pixel;
	/* Fill variables */
	CORBA_long	fill_style,fill_index,fill_color,fill_per,fill_qi;
	CORBA_long	fill_qc,fill_qp;
	Uint32		fill_pixel;
	/* Fill "work" vars */
	CORBA_long	fill_miny, fill_maxy, fill_minx, fill_maxx;
	CORBA_long	fill_intersect;

	CORBA_long	val_mode,chc_mode,loc_mode,str_mode;
	CORBA_long	write_qm;
	CORBA_long	xfm_mode;

/* filled area variables */

	CORBA_long	y,odeltay,deltay,deltay1,deltay2;
	CORBA_long	fill_CORBA_longersect;
	CORBA_long	*patptr;
	CORBA_long	patmsk;

/* gdp area variables */
	CORBA_long	xc, yc, xrad, yrad, del_ang, beg_ang, end_ang;
	CORBA_long	start, angle, n_steps;

/* attribute environment save variables */
	CORBA_long	s_fill_per, *s_patptr, s_patmsk, s_nxtpat;
	CORBA_long	s_begsty, s_endsty;

	CORBA_long	CLIP, XMN_CLIP, XMX_CLIP, YMN_CLIP, YMX_CLIP;
	CORBA_long	DITHER[128];
	CORBA_long	udpt_np;
	CORBA_long	UD_PATRN[32];
	CORBA_long	SOLID;

/* Writing mode */
	CORBA_long	WRT_MODE;
	CORBA_long	FLIP_Y;

/* Text output variables */
	CORBA_long h_align, v_align, height, width;
	CORBA_long L_OFF, R_OFF, ACTDELY, SPECIAL, MONO_STATUS;
	CORBA_long FIR_CHR, WEIGHT, CHR_HT;
	GPoint	DEST;
	CORBA_short rot_case;
	FontHead   *FONT_INF;
} DrvData;

