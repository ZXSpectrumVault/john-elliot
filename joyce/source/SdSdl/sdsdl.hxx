/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wchar.h>
#include <SDL.h>
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "gemutil.h"
#include "types.h"
#include "drv_bits.h"

#include "drvglobals.hxx"

#define DRV_W 640
#define DRV_H 480

#include "driver.hxx"	// Public driver interface
#include "fontdef.h"


class SdSdl: public VdiDriver
{
protected:
	DrvGlobals *m_globals;
public:
	SdSdl();
	SdSdl(SdSdl *tmpl);
	virtual ~SdSdl();

	virtual bool open(const GWSParams *wsp, GWSResult *wsr);
	virtual bool open(const GWSParams *wsp, GWSResult *wsr, SDL_Surface *s, int w = -1, int h = -1);
	virtual void close();
	virtual void clear();
	virtual void flush();
	virtual void escape(long type, long pcount, const GPoint *pt, 
			long icount, const long *ival, wchar_t *text,
                        long *pcout, GPoint *ptout,
                        long *icout, long *intout);
	virtual void enterCur();
	virtual void exitCur();
	virtual void polyLine   (long count, const GPoint *pt);
	virtual void polyMarker (long count, const GPoint *pt);
	virtual void graphicText(const GPoint *pt, const wchar_t *str);
	virtual void fillArea(long count, const GPoint *pt);
	virtual void gdp(long type, long pcount, const GPoint *pt, long icount,
			const long *ival, wchar_t *text);
	virtual void fillRect(const GPoint pt[2]);
	virtual void stHeight(const GPoint *height, GPoint *result);
	virtual long stRotation(long angle);
	virtual void setColor(long index, short r, short g, short b);
	virtual long slType(long type);
	virtual void slWidth(const GPoint *width, GPoint *result);
	virtual long slColor(long index);
	virtual long smType(long type);
	virtual void smHeight(const GPoint *height, GPoint *result);
	virtual long smColor(long color);
	
	virtual long stFont	(long font);
	virtual long stColor	(long color);
	virtual long sfStyle	(long style);
	virtual long sfIndex	(long index);
	virtual long sfColor	(long color);
	virtual long qColor	(long color, short mode, short *r, short *g, short *b);
	virtual short swrMode	(short mode);
        virtual short qLocator  (short device, const GPoint *ptin,
                                        GPoint *ptout, short *termch);

	virtual short sinMode	(short device, short mode);
	virtual short qinMode   (short device);
	virtual void queryLine	(long *styles, GPoint *width);
	virtual void queryMarker(long *styles, GPoint *size);
	virtual void queryFill	(long *styles);
	virtual void queryText	(long *styles, GPoint *size);
	virtual void stAlign	(long h, long v, long *ah, long *av);

	virtual SdSdl *openVirtual(const GWSParams *wsp, GWSResult *wsr);
	virtual void closeVirtual();

	virtual void queryExt       (int question, GWSResult *wsr);
	virtual void contourFill    (long fill, const GPoint *xy) ;
	virtual long sfPerimeter    (long visible);
	virtual int  getPixel	    (const GPoint *xy, long *pel, long *index);
	virtual void queryTextExtent(const wchar_t *str, GPoint *extent);

// not yet in the base class or CORBA interface
	virtual void setClip(short do_clip, const GPoint *pt);
	virtual void slEnds(const long *intin, long *intout);
	virtual void udLinestyle(long pattern);
	virtual long stStyle(long style);
	virtual long stPoint(long point, GPoint *sizes);
	virtual int  sfUdPat(long count, unsigned long *pattern);

        virtual USERMOT exMotv(USERMOT um);
        virtual USERCUR exCurv(USERCUR uc);
        virtual USERBUT exButv(USERBUT ub);
	
protected:
	// opttext.cc 
	void graphicText(const GPoint *pt, const wchar_t *str, bool justified, 
			Uint32 *just_w);
	void text_init(const GWSParams *wsp);
	void text_blt_special(const wchar_t *str, bool justified);
	Uint32 text_move(Uint32 form_w, Uint32 char_w, Uint32 x);
	void text_italic(Uint32 char_w, Uint32 char_h, Uint32 buf_w);
	void text_scale(Uint32 form_w, Sint32 *char_w, Uint32 *buf_w);
	void text_blit(Uint8 *bits, Uint32 form_wb, Uint32 char_wp, Uint32 sx, Uint32 dx);
	void text_bold(Uint32 char_h, Uint32 buf_w);
	void text_grey(Uint32 char_h, Uint32 buf_w);
	void text_rotate(Uint32 char_h, Sint32 *char_wb, Uint32 *buf_w, Uint32 offset);
	void text_blt_done();
	Uint32 text_attr_just(Uint32 cell_w);
public:
	bool pre_blt_0(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits);
	bool pre_blt_90(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits);
	bool pre_blt_180(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits);
	bool pre_blt_270(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits);
	void post_blt_0(Sint32 *x, Sint32 dx);
	void post_blt_90(Sint32 *x, Sint32 dx);
	void post_blt_180(Sint32 *x, Sint32 dx);
	void post_blt_270(Sint32 *x, Sint32 dx);
protected:
	
	inline void cpy_head()
		{ memcpy (m_cur_head, m_cur_font, sizeof(FontHead)); }
	inline void inc_lfu() { ++m_act_font->lfu; }
	void make_header();
	void sel_font(void);
	FontHead *sel_effect(FontHead *f_ptr);
	FontHead *sel_size(FontHead *f_ptr);
	// opttdraw.cc
	void in_rot();
	void in_doub();	
	wchar_t set_fnthdr(wchar_t ch);
	int chk_ade(wchar_t ch);
	FontHead *find_ade_seg(wchar_t ch, FontHead *si);
	FontHead *find_font_seg0(FontHead *si);
	void chk_fnt();
	Uint32 ACT_SIZ(Uint32 size);
	double CLC_DDA(double actual, double requested);

#define txtblt_xor_rl_0 txtblt_xor_rl_s
#define txtblt_xor_rr_0 txtblt_xor_rr_s

	void txtblt_xor_rl_s(int vertical_scan, int rcount, int bytes_w, 
			int source_w, Uint8 lmask, Uint8 rmask, 
			Uint8 **mem_bits, Uint8 *dest_bits);
	
	void txtblt_xor_rr_s(int vertical_scan, int rcount, int bytes_w, 
			int source_w, Uint8 lmask, Uint8 rmask, 
			Uint8 **mem_bits, Uint8 *dest_bits);

	// opttxt1.cc 	
	void txtblt_rep(Uint32 char_w, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask);
        void txtblt_xor(Uint32 char_w, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask);
        void txtblt_tran(Uint32 char_w, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask, Uint32 pixel);
	void TEXT_BLT(const wchar_t *str, bool justified);
	// monoblit.cc 
	bool MONO8XHT(const wchar_t *str, bool justified);
	void MONO8XHT_REPLACE(Uint8 *bits, const wchar_t *str);
        void MONO8XHT_TRAN(Uint8 *bits, const wchar_t *str);
        void MONO8XHT_XOR(Uint8 *bits, const wchar_t *str);
	Uint8 *sdl_repl_byte(Uint8 *bits, Uint8 byte, Uint32 fg);
        Uint8 *sdl_xor_byte(Uint8 *bits, Uint8 byte, Uint32 fg);
	Uint8 *sdl_tran_byte(Uint8 *bits, Uint8 byte, Uint32 fg);

	void clr_skew();
	// monout.c 
	void init_wk(const GWSParams *wsp, GWSResult *wsr);
	int clip_line();
	int code( Sint32 x, Sint32 y );
	void wline(long count, const GPoint *pt);
	void pline(long count, const GPoint *pt);
	void plygn(long count, const GPoint *pt);
	void do_arrow(long count, const GPoint *pt);
	void arrow(GPoint *xy, Sint32 inc, long count, GPoint *pt);
	void do_circ(GPoint *xy);
	void s_fa_attr();
	void r_fa_attr();
	void perp_off(GPoint *xy);
	void quad_xform(int quad, GPoint *xy, GPoint *txy);
	void st_fl_ptr();
	void gdp_arc(int filled, const GPoint *pt, const long *angles);
	void gdp_ell(int filled, const GPoint *pt, const long *angles);
	void clc_nsteps();
	void clc_arc(int type, const GPoint *pt);
	void Calc_pts(int j, GPoint *ptwork);
	void gdp_rbox(int type, const GPoint *pt);
	void d_justified(const GPoint *pt, const wchar_t *str, 
			Uint32 word_space, Uint32 char_space);
	Sint32 d_justif_calc(const GPoint *pt, wchar_t *str, 
			Uint32 word_space, Uint32 char_space,
			Uint32 *out);
	// sdlline.cc
public:
	void sdl_plot(Uint8 *bits, Uint32 pixel);
	void sdl_xor (Uint8 *bits, Uint32 pixel);
	void sdl_mov_cur(Uint32 x, Uint32 y);
	bool handleEvent(SDL_Event *e);

protected:
	void BOX_FILL_LINE(long Y, Uint8 **b);
	void BOX_FILL(void);

	Uint8 *concat(Sint32 x, Sint32 y);
	long xline_noswap(Uint8 **bits);
	long xline_swap(Uint8 **bits);
	long tennis(long dx);
	long do_line(void);


	int lock_surface();
	void unlock_surface();
	void update_rect(Sint32 x1, Sint32 y1, Sint32 x2, Sint32 y2);
	void ABLINE();
	void HABLINE();
	void RECTFILL();
	void HLINE_CLIP();

	// sdlfill.cc
	void CLC_FLIT(long count, GPoint *vertices);

	// sdlmouse.cc 
	void INIT_G();
	void HIDE_CUR();
	void DIS_CUR();
	short GLOC_KEY(short modal);
	short GCHR_KEY();
	short mouse_function(short fn, short *termc);
	void keyboard_mouse(short button, Sint32 dx, Sint32 dy);
	void clip_cross(Sint32 *x, Sint32 *y);
	// sdlpat.cc
	void init_patterns();
	// monobj.cc
	void arb_corner(const GPoint *ptsin, GPoint *ptsout, int type);
protected:
	// Surface characteristics, cached for speed
	Uint8   m_sdl_bpp;
	Uint16  m_sdl_pitch;
	/* Line variables */
	Sint32	m_line_index, m_line_color, m_line_qi;
	Sint32  m_line_qc, m_line_width;
	Sint32	m_line_qw;
	Uint32	m_line_pixel;
	/* Line "work" variables */
	Uint32	m_LN_RMASK;
	Uint32	m_LN_MASK;
	Sint32 m_LSTLIN, m_X1, m_X2, m_Y1, m_Y2;
	Sint32	m_num_qc_lines, m_q_circle[MAX_L_WIDTH];
	Sint32	m_line_beg, m_line_end, m_yinc;
public:
	Uint32  m_bg_pixel;
	Sint32	m_BOX_MODE;
	Uint32	m_FG_BP_1;
protected:
	/* Patterns */
	Sint32	m_NEXT_PAT;
	Sint32	m_line_sty[MAX_LINE_STYLE];
#define ud_lstyl line_sty[MAX_LINE_STYLE - 1]
	

	/* Marker variables */
	Sint32	m_mark_height,m_mark_scale;
	Sint32  m_mark_index,m_mark_color,m_mark_qi;
	Sint32	m_mark_qc;
	Uint32	m_mark_pixel;
	/* Fill variables */
	Sint32	m_fill_style,m_fill_index,m_fill_color,m_fill_per,m_fill_qi;
	Sint32	m_fill_qc,m_fill_qp;
	Uint32	m_fill_pixel;
	/* Fill "work" vars */
	Sint32	m_fill_miny, m_fill_maxy, m_fill_minx, m_fill_maxx;

	Sint32	m_val_mode,m_chc_mode,m_loc_mode,m_str_mode;
	Sint32	m_write_qm;

/* filled area variables */

	Sint32	m_y,m_odeltay,m_deltay,m_deltay1,m_deltay2;
	Sint32	m_fill_intersect;
	Uint32	*m_patptr;
	Sint32	m_patmsk;

/* gdp area variables */
	Sint32	m_xc, m_yc, m_xrad, m_yrad, m_del_ang, m_beg_ang, m_end_ang;
	Sint32	m_start, m_angle, m_n_steps;

/* attribute environment save variables */
	Sint32	m_s_fill_per, m_s_patmsk, m_s_nxtpat;
	Sint32	m_s_begsty, m_s_endsty;
	Uint32  *m_s_patptr;

	bool	m_CLIP;
	Sint32  m_XMN_CLIP, m_XMX_CLIP, m_YMN_CLIP, m_YMX_CLIP;
	Sint32	m_DITHER[128];
	Sint32	m_udpt_np;
	Uint32	m_UD_PATRN[32];
	Sint32	m_SOLID;

/* Writing mode */
	Sint32	m_WRT_MODE;
public:
	Sint32	m_FLIP_Y;
	Sint32	m_xfm_mode;
private:
	Sint32	m_text_color;
	Uint32  m_text_pixel;
	Uint32  m_text_mode,  m_text_ix; // XXX
/* Text output variables */
	Sint32	   m_h_align, m_v_align, m_height, m_width;
	Sint32	   m_L_OFF, m_R_OFF, m_ACTDELY, m_SPECIAL, m_MONO_STATUS;
	Sint32	   m_WEIGHT, m_CHR_HT;
	GPoint	   m_DEST;
	Sint16	   m_rot_case;
	FontHead   m_FONT_INF;
	FontHead   *m_cur_font, *m_act_font; 
	FontHead   *m_cur_head;	
	FontHead   *m_font_top; 
	Uint8      m_def_buf[2 * cell_size];
	Uint8	   m_ptsin_buf[512];
	Uint8      *m_txbuf1, *m_txbuf2, *m_sav_buf1, *m_sav_buf2; 
	Uint32	   m_ini_font_count;
// XXX Below this point, not initialised or DUPed

	Sint32 m_rq_size;
	Uint32 m_rq_attr, m_dbl, m_rq_type, m_loaded, m_CHAR_DEL;
	Uint32 m_rq_font;
	double m_XACC_DDA;
	Uint32 m_sav_pixel;
	double m_DDA_INC;	// 0.0 to 1.0
	Uint32 m_T_SCLSTS;
	Uint32 m_tempdely, m_lt_hoff, m_rt_hoff, m_foffindex;
	Sint32 m_y_position;
	Uint32 m_form_offset, m_form_width;
	Uint8  *m_font_off, *m_buff_ptr;
	wchar_t m_isspace;
	Uint32	m_rot_height;
	Uint32  m_char_count;
// Text justification
	Sint32 m_JUST_DEL_WORD, m_JUST_DEL_WORD_REM, m_JUST_DEL_WORD_SIGN,
	       m_JUST_DEL_CHAR, m_JUST_DEL_CHAR_REM, m_JUST_DEL_CHAR_SIGN,
	       m_JUST_DEL_SIGN, m_JUST_NEG;
	Uint32 m_top_overhang;
	bool m_FIRST_MOTN, m_mouse_lock;
	Sint32 m_mousedx, m_mousedy, m_gcurx, m_gcury;
	Uint16 m_MOUSE_BT, m_m_old_mouse_but;
	short m_TERM_CH;
	bool m_CONTROL_STATUS;
	bool m_KBD_MOUSE_STS;
	Uint32 m_LAST_CONTRL;
	Uint32 m_HIDE_CNT;
};

/* muldiv.c */
Sint32 SMUL_DIV (Sint32 m1, Sint32 m2, Sint32 d1);
Sint32 vec_len  (Sint32 dx, Sint32 dy);

	/* Fill patterns & marker shapes */
extern Uint32 HAT_1_MSK, HAT_0_MSK, OEMMSKPAT, DITHRMSK;
extern Uint32 HATCH0[], HATCH1[], OEMPAT[], DITHER[], SOLID[], HOLLOW[]; 
extern Sint32 m_dot[], m_plus[], m_star[], m_square[], m_cross[], m_dmnd[];
	/* Standard fonts */
extern FontHead fntFirst;

// Integer sin and cos. XXX 16-bit only
short Isin( short angle  );
short Icos( short angle  );

