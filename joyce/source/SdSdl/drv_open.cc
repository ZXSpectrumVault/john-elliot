/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"

SdSdl::SdSdl()
{
	m_sdl_bpp = 0;
	m_sdl_pitch = 0;

	m_text_color = 0;
	m_text_pixel = 0;
	m_bg_pixel = 0;
	m_line_index = m_line_color = m_line_qi = 0;
	m_line_qc = m_line_width = 0;
	m_line_qw = 0;
	m_line_pixel = 0;

	m_LN_RMASK = 0;
	m_LN_MASK = 0;
	m_LSTLIN = m_X1 = m_X2 = m_Y1 = m_Y2 = 0;
	m_num_qc_lines = 0;
	memset(m_q_circle, 0, sizeof m_q_circle);
	m_line_beg = m_line_end = m_yinc = 0;
	m_BOX_MODE = 0;
	m_FG_BP_1 = 0;
	/* Patterns */
	m_NEXT_PAT = 0;
	memset(m_line_sty, 0, sizeof m_line_sty);

	/* Marker variables */
	m_mark_height = m_mark_scale = 0;
	m_mark_index  = m_mark_color = m_mark_qi = 0;
	m_mark_qc = 0;
	m_mark_pixel = 0;
	/* Fill variables */
	m_fill_style = m_fill_index = m_fill_color = m_fill_per = m_fill_qi = 0;
	m_fill_qc = m_fill_qp = 0;
	m_fill_pixel = 0;
	/* Fill "work" vars */
	m_fill_miny = m_fill_maxy = m_fill_minx = m_fill_maxx = 0;

	m_val_mode = m_chc_mode = m_loc_mode = m_str_mode = 0;
	m_write_qm = 0;

/* filled area variables */

	m_y = m_odeltay = m_deltay = m_deltay1 = m_deltay2 = 0;
	m_fill_intersect = 0;
	m_patptr = NULL;
	m_patmsk = 0;

/* gdp area variables */
	m_xc = m_yc = m_xrad = m_yrad = m_del_ang = m_beg_ang = m_end_ang = 0;
	m_start = m_angle = m_n_steps = 0;

/* attribute environment save variables */
	m_s_fill_per = 0;
	m_s_patptr   = NULL;
	m_s_patmsk = 0;
	m_s_nxtpat = 0;
	m_s_begsty = 0;
	m_s_endsty = 0;

	m_CLIP = false;
	m_XMN_CLIP = 0;
	m_XMX_CLIP = 0;
	m_YMN_CLIP = 0;
	m_YMX_CLIP = 0;
	memset(m_DITHER, 0, sizeof m_DITHER);
	m_udpt_np = 0;	
	memset(m_UD_PATRN, 0, sizeof m_UD_PATRN);
	m_SOLID = 0;
	m_WRT_MODE = 0;
	m_FLIP_Y = m_xfm_mode = 0;
	m_h_align = m_v_align = m_height = m_width = 0;
	m_L_OFF = m_R_OFF = m_ACTDELY = m_SPECIAL = m_MONO_STATUS = 0;
	m_WEIGHT = m_CHR_HT = 0;
	m_rot_case = 0;
	memset(&m_FONT_INF, 0, sizeof m_FONT_INF);
	m_font_top = &fntFirst;
	m_cur_font = m_act_font = m_cur_head = NULL;
	memset(m_def_buf, 0, sizeof m_def_buf);
	m_txbuf1 = m_txbuf2 = m_sav_buf1 = m_sav_buf2 = NULL;
	m_ini_font_count = 0;

}


SdSdl::SdSdl(SdSdl *tmpl)
{
#define DUP(s) s = tmpl-> s ;
	DUP(m_sdl_bpp);
	DUP(m_sdl_pitch);
	DUP(m_globals)
	DUP(m_bg_pixel)
	DUP(m_text_color)
	DUP(m_text_pixel)
	DUP(m_line_index)
	DUP(m_line_color)
	DUP(m_line_qi)
	DUP(m_line_qc)
	DUP(m_line_width)
	DUP(m_line_qw)
	DUP(m_line_pixel)

	DUP(m_LN_RMASK)
	DUP(m_LN_MASK)
	DUP(m_LSTLIN)
	DUP(m_X1)
	DUP(m_X2)
	DUP(m_Y1)
	DUP(m_Y2)
	DUP(m_num_qc_lines)
	memcpy(m_q_circle, tmpl->m_q_circle, sizeof m_q_circle);
	DUP(m_line_beg)
	DUP(m_line_end)
	DUP(m_yinc)
	DUP(m_BOX_MODE)
	DUP(m_FG_BP_1)
	/* Patterns */
	DUP(m_NEXT_PAT)
	memcpy(m_line_sty, tmpl->m_line_sty, sizeof m_line_sty);

	/* Marker variables */
	DUP(m_mark_height)
	DUP(m_mark_scale)
	DUP(m_mark_index)
	DUP(m_mark_color)
	DUP(m_mark_qi)
	DUP(m_mark_qc)
	DUP(m_mark_pixel)
	/* Fill variables */
	DUP(m_fill_style)
	DUP(m_fill_index)
	DUP(m_fill_color)
	DUP(m_fill_per)
	DUP(m_fill_qi)
	DUP(m_fill_qc)
	DUP(m_fill_qp)
	DUP(m_fill_pixel)
	/* Fill "work" vars */
	DUP(m_fill_miny)
	DUP(m_fill_maxy)
	DUP(m_fill_minx)
	DUP(m_fill_maxx)

	DUP(m_val_mode)
	DUP(m_chc_mode)
	DUP(m_loc_mode)
	DUP(m_str_mode)
	DUP(m_write_qm)

/* filled area variables */

	DUP(m_y)
	DUP(m_odeltay)
	DUP(m_deltay)
	DUP(m_deltay1)
	DUP(m_deltay2)
	DUP(m_fill_intersect)
	DUP(m_patptr)
	DUP(m_patmsk)

/* gdp area variables */
	DUP(m_xc)
	DUP(m_yc)
	DUP(m_xrad)
	DUP(m_yrad)
	DUP(m_del_ang)
	DUP(m_beg_ang)
	DUP(m_end_ang)
	DUP(m_start)
	DUP(m_angle)
	DUP(m_n_steps)

/* attribute environment save variables */
	DUP(m_s_fill_per)
	DUP(m_s_patptr)
	DUP(m_s_patmsk)
	DUP(m_s_nxtpat)
	DUP(m_s_begsty)
	DUP(m_s_endsty)

	DUP(m_CLIP)
	DUP(m_XMN_CLIP)
	DUP(m_XMX_CLIP)
	DUP(m_YMN_CLIP)
	DUP(m_YMX_CLIP)
	memcpy(m_DITHER, tmpl->m_DITHER, sizeof m_DITHER);
	DUP(m_udpt_np)	
	memcpy(m_UD_PATRN, tmpl->m_UD_PATRN, sizeof m_UD_PATRN);
	DUP(m_SOLID)
	DUP(m_WRT_MODE)
	DUP(m_FLIP_Y)
	DUP(m_xfm_mode)
	DUP(m_h_align)
	DUP(m_v_align)
	DUP(m_height)
	DUP(m_width)
	DUP(m_L_OFF)
	DUP(m_R_OFF)
	DUP(m_ACTDELY) 
	DUP(m_SPECIAL)
	DUP(m_MONO_STATUS)
	DUP(m_WEIGHT)
	DUP(m_CHR_HT)
	DUP(m_rot_case)
	memcpy(&m_FONT_INF, &tmpl->m_FONT_INF, sizeof m_FONT_INF);
	DUP(m_font_top)
        DUP(m_cur_font)
	DUP(m_act_font)
	DUP(m_cur_head)
        memcpy(m_def_buf, tmpl->m_def_buf, sizeof m_def_buf);
        DUP(m_txbuf1)	// Pointers will need to be rejigged; they'll
	DUP(m_txbuf2)	// still be pointing at the old SdSdl, not the new one
	DUP(m_sav_buf1)	// Fortunately openVirtual() calls text_init(), which
	DUP(m_sav_buf2)	// does this.
        DUP(m_ini_font_count)
}
#undef DUP



SdSdl::~SdSdl()
{
        --m_globals->m_refcount;
        if (!m_globals->m_refcount)
        {
        	delete m_globals;
        }

}




void	SdSdl::exitCur()
{
	SDL_Color clrset[MAX_COLOR];
	int n;

	if (m_globals->m_own_surface)
	{
		if (m_globals->m_requestW < 0) m_globals->m_requestW = DRV_W;
		if (m_globals->m_requestH < 0) m_globals->m_requestH = DRV_H;
		m_globals->m_surface = SDL_SetVideoMode(m_globals->m_requestW, 
				m_globals->m_requestH, 8, SDL_SWSURFACE);
	}
	m_globals->m_bytes_line = (m_globals->m_surface->w / 8);

	if (!m_globals->m_surface)
	{
		fprintf(stderr, "Can't get %dx%d video mode: %s\n",
				DRV_W, DRV_H, SDL_GetError());
		return;
	}
	if (m_globals->m_requestW < 0) 
	{
		m_globals->m_requestW = m_globals->m_surface->w;
	}
	if (m_globals->m_requestH < 0) 
	{
		m_globals->m_requestH = m_globals->m_surface->h;
	}
	m_globals->m_DEV_TAB[0] = m_globals->m_requestW - 1;
	m_globals->m_DEV_TAB[1] = m_globals->m_requestH - 1;
	m_globals->m_xres = m_globals->m_requestW - 1;
	m_globals->m_yres = m_globals->m_requestH - 1;
        m_sdl_bpp   = m_globals->m_surface->format->BytesPerPixel; 
        m_sdl_pitch = m_globals->m_surface->pitch;

/* Set up the palette */
	for (n = 0; n < MAX_COLOR; n++)
	{
		clrset[n].r = m_globals->m_clr_red[n];
                clrset[n].g = m_globals->m_clr_green[n];
                clrset[n].b = m_globals->m_clr_blue[n];
	}	
	SDL_SetColors(m_globals->m_surface, clrset, 0, MAX_COLOR);	

	clear();
}


void	SdSdl::enterCur(void)
{

}

bool SdSdl::open(const GWSParams *wsp, GWSResult *wsr)
{
	return open(wsp, wsr, NULL, -1, -1);
}


bool SdSdl::open(const GWSParams *wsp, GWSResult *wsr, SDL_Surface *s,
		int w, int h)
{
	m_globals = new DrvGlobals(s);

        m_globals->m_refcount = 1;
	m_globals->m_requestW = w;
	m_globals->m_requestH = h;

	exitCur();
	if (!m_globals->m_surface)
	{
		fprintf(stderr, "No surface!");
		return false;
	}
	text_init(wsp);
	init_patterns();
	init_wk(wsp, wsr);
	INIT_G();
	return true;
}


void SdSdl::close()
{
	enterCur();
	closeVirtual();
}



SdSdl *SdSdl::openVirtual(const GWSParams *wsp, GWSResult *wsr)
{
	SdSdl *snew = new SdSdl(this);
	if (!snew) return NULL;

	snew->m_globals->m_refcount++;
	text_init(wsp);
	snew->init_patterns();
	snew->init_wk(wsp, wsr);
	return snew;
}


void SdSdl::closeVirtual()
{
}

static Sint32 init_data[45] = 
	{
		DRV_W, DRV_H,
		1,
		419, 419,	/* Pixel sizes in microns */
		0,
		8,
		40,
		8,
		8,
		0,
		24,
		12,
		256,		/* XXX Make this a real no. of colours*/
		10,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		3, 0, 3, 3, 3, 0, 3, 0, 3, 2,
		1,		/* Colour */
		1,		/* Text rotation */
		1,		/* Fill polygon */
		0,		/* Cell arry */ 
		256,		/* Palette size */
		2,		/* Mouse & kbd */
		1,		/* Valuators */
		1,		/* Choices */
		1,		/* Strings */
		2		/* Workstation type */
};
static GPoint init_points[6] =
{
	 {0,  0},   {0,   0},		/* Character sizes */
	 {1,  0},  {40,   0},		/* Line sizes */
	{15, 15}, {120, 120}		/* Marker sizes */
};

#define PREDEF_COLS 24		/* Start with 24 fixed colours */

static Uint8 init_cols[3 * PREDEF_COLS] = 
{
	255, 255, 255,		/* GEM primaries */
	  0,   0,   0, 	
	255,   0,   0,
	  0, 255,   0,
	  0,   0, 255,
          0, 255, 255,
	255, 255,   0,
	255,   0, 255,

	170, 170, 170,		/* GEM/1-GEM/4 */
	 85,  85,  85,
	170,   0,   0,
	  0, 170,   0,
	  0,   0, 170,
	  0, 170, 170,
	170, 170,   0,
	170,   0, 170,
	
	  0, 117, 107,		/* GEM/5 */
	255, 195,   0, 
	 99,  97,  99,
	173, 195, 173,
	173, 195, 173,
	239, 235, 239,
	198, 195, 198,
	156, 154, 156 
};

DrvGlobals::DrvGlobals(SDL_Surface *s)
{
	int n;

	if (s)
	{
		m_own_surface = false;
		m_surface = s;
	}
	else
	{
		m_own_surface = true;
		m_surface = NULL;
	}
	m_sdl_locked = 0;
	m_SIZ_TAB = init_points;
	memcpy(m_DEV_TAB, init_data, sizeof(m_DEV_TAB));
	m_HIDE_CNT = 0;


	/** Initialise colours **/

	for (n = 0; n < PREDEF_COLS; n++)
	{
		m_clr_red  [n] = init_cols[n * 3];
		m_clr_green[n] = init_cols[n * 3 + 1]; 
		m_clr_blue [n] = init_cols[n * 3 + 2];

		m_req_red  [n] = (1000 * m_clr_red  [n]) / 255;
                m_req_green[n] = (1000 * m_clr_green[n]) / 255;
                m_req_blue [n] = (1000 * m_clr_blue [n]) / 255;
	}

	/* nb: PREDEF_COLS upto 128 are unused */
	for (n = PREDEF_COLS; n < 128; n++)
	{
		m_clr_blue [n] = 0;
		m_clr_green[n] = 0;
		m_clr_red  [n] = 0;
		m_req_blue [n] = 0;
		m_req_green[n] = 0;
		m_req_red  [n] = 0;
	}

	for (n = 128; n < MAX_COLOR; n++)
	{
		int b = (n - 128);

		if (b < 64) 
		{
			m_clr_blue[n]  =  (b &    3)       * 85;
			m_clr_green[n] = ((b & 0x0C) >> 2) * 85;
			m_clr_red [n]  = ((b & 0x30) >> 2) * 85;

	                m_req_red  [n] = (1000 * m_clr_red  [n]) / 255;
       		        m_req_green[n] = (1000 * m_clr_green[n]) / 255;
               		m_req_blue [n] = (1000 * m_clr_blue [n]) / 255;

		}
		else
		{
			m_clr_blue[n] = m_clr_green[n] = m_clr_red[n] = 
				(b & 0x3F) * 4;	
			m_req_red[n] = m_req_green[n] = m_req_blue[n] = 
				(m_clr_blue[n] * 1000) / 255;	
		}
	}
}

DrvGlobals::~DrvGlobals()
{

}


void SdSdl::queryExt(int which, GWSResult *wsr)
{
	int i;

	switch(which)
	{
		case 0: for (i = 0; i < 45; i++)
        		{
				wsr->data[i]   = m_globals->m_DEV_TAB[i];
        		}
			for (i = 0; i < 6; i++)
			{
				wsr->points[i].x = m_globals->m_SIZ_TAB[i].x;
				wsr->points[i].y = m_globals->m_SIZ_TAB[i].y;
			}
			break;

	}	
}


