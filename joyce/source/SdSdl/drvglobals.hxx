/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/
/* Variables shared by all virtual workstations */


#include "drvdata.sdl"

#define KQUEUE_SIZE 20

class DrvGlobals
{
public:
	DrvGlobals(SDL_Surface *s = NULL);
	~DrvGlobals();
	
	int m_refcount;
	int m_sdl_locked;
	Sint32 m_HIDE_CNT;	
	Sint32 m_DEV_TAB[45];
	GPoint *m_SIZ_TAB;

	Uint8	m_clr_red  [MAX_COLOR],
		m_clr_green[MAX_COLOR], 
		m_clr_blue [MAX_COLOR];
	Uint16  m_req_red  [MAX_COLOR],
		m_req_green[MAX_COLOR],
		m_req_blue [MAX_COLOR];

	bool	m_own_surface;	  /* Does SdSdl own the video surface? */
	SDL_Surface *m_surface;   /* The video surface */
        Uint8 m_txtblt_rrot_table_1[2048]; 
        Uint8 m_txtblt_rrot_table_2[2048]; 
        Uint16 m_double_table[256]; 
	Sint32 m_requestW, m_requestH;

	Sint32 m_xres, m_yres;

	Uint32 m_bytes_line;
        Uint32 MAP_COL(Uint32 colour);
	Sint16 m_mouse_status_byte;
	Sint32 m_mouse_x;
	Sint32 m_mouse_y;
	Sint16 m_mouse_switch_byte;
	SDLMod m_shifts;

	USERMOT m_motv;
	USERCUR m_curv;
	USERBUT m_butv;

	SDL_Event m_kqueue[KQUEUE_SIZE];
	SDL_Event *m_kqueue_read, *m_kqueue_write;
public:
	void setColor(long index, short r, short g, short b);
	long qColor(long color, short mode, short *r, short *g, short *b);
};

