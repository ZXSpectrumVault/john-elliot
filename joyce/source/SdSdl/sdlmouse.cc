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

USERMOT SdSdl::exMotv(USERMOT mot)
{
	USERMOT mot2 = m_globals->m_motv;
	m_globals->m_motv = mot;
	return mot2;
}

USERCUR SdSdl::exCurv(USERCUR cur)
{
	USERCUR cur2 = m_globals->m_curv;
	m_globals->m_curv = cur;
	return cur2;
}


USERBUT SdSdl::exButv(USERBUT but)
{
	USERBUT but2 = m_globals->m_butv;
	m_globals->m_butv = but;
	return but2;
}

static void umotvec(VdiDriver *self, unsigned long x, unsigned long y) { }
static void ubutvec(VdiDriver *self, unsigned short buttons)     { }
static void mov_cur(VdiDriver *self, unsigned long x, unsigned long y) 
{
	SdSdl *me = (SdSdl *)self;
	me->sdl_mov_cur(x, y);
}

void SdSdl::sdl_mov_cur(Uint32 x, Uint32 y)
{
	if (m_gcurx != x || m_gcury != y)
	{
		SDL_WarpMouse(x, y);
		m_gcurx = x;
		m_gcury = y;
	}
}

void SdSdl::INIT_G()
{
	m_HIDE_CNT = 1;
	m_KBD_MOUSE_STS = true;
	m_LAST_CONTRL = 0;
	m_globals->m_kqueue_read  = m_globals->m_kqueue;
	m_globals->m_kqueue_write = m_globals->m_kqueue;
	exMotv(umotvec);
	exButv(ubutvec);
	exCurv(mov_cur);
	mouse_function(1, NULL);
}

void SdSdl::HIDE_CUR()
{
	SDL_ShowCursor(0);
}

void SdSdl::DIS_CUR()
{
	SDL_ShowCursor(1);
}

/************************************************************************
 *GLOC_KEY                                                              *
 *      Get Locator key                                                 *
 *              Entry  none                                             *
 *              Exit                                                    *
 *                      ax=0 nothing happened                           *
 *                                                                      *
 *                      ax=1 button press                               *
 *                              TERM_CH = 16 bit character information  *
 *                                                                      *
 *                      ax=2 coordinate information                     *
 *                              X1 = new x                              *
 *                              Y1 = new y                              *
 *                                                                      *
 *                      ax=4 keyboard coord infor                       *
 *                              X1 = new x                              *
 *                              Y1 = new y                              *
 ************************************************************************/

short SdSdl::GLOC_KEY(short modal)
{
	short k;
	short termc;
	SDL_Event ev;
//
// Slightly more complicated than the GEM one, because in the modal 
// version we wait for an event rather than letting the caller spin in 
// a tight loop.
//
	if (modal) 
	{
		if (SDL_WaitEvent(&ev)) handleEvent(&ev);
	}
	else
	{
		if (SDL_PollEvent(&ev)) handleEvent(&ev);
	}
	// Handle keypress
	k = GCHR_KEY();
	if (k) return 1;
	
	// Handle mouse event
	k = mouse_function(3, &termc);
	if (!k)	return 0;
	m_TERM_CH = termc;
	return k;
}



static Sint32 kbd_mouse_tbl[] = 
{
	              SDLK_HOME,    1, 0, 0,
	0x40000000L | SDLK_HOME,    1, 0, 0,
	              SDLK_END,  0x81, 0, 0,
	0x40000000L | SDLK_END,  0x81, 0, 0,
	              SDLK_UP,      0, 0, -cur_mot_max_y,
	0x40000000L | SDLK_UP,      0, 0, -1,
	              SDLK_DOWN,    0, 0, cur_mot_max_y,
	0x40000000L | SDLK_DOWN,    0, 0, 1,
	              SDLK_LEFT,    0, -cur_mot_max_x, 0,
	0x40000000L | SDLK_LEFT,    0, -1, 0,
	              SDLK_RIGHT,   0, cur_mot_max_x, 0,
	0x40000000L | SDLK_RIGHT,   0,  1, 0,
};


/************************************************************************
 *GCHR_KEY                                                              *
 *      Get character for string input                                  *
 *              Entry  none                                             *
 *                                                                      *
 *              Exit                                                    *
 *                      ax=0 nothing happened                           *
 *                                                                      *
 *                      ax=1 button press                               *
 *                         TERM_CH =  16 bit character information      *
 *                                                                      *
 ************************************************************************/
#define KQUEUE_INC(e) { e++; if ((e - m_globals->m_kqueue) >= KQUEUE_SIZE) \
			e = m_globals->m_kqueue; }
short SdSdl::GCHR_KEY()
{
	int n;
	bool keyhit = false;
	Uint32 cl;

	// key hit
	while (m_globals->m_kqueue_read != m_globals->m_kqueue_write)
	{
		if (m_globals->m_kqueue_read->type == SDL_KEYDOWN)
		{
			m_TERM_CH = m_globals->m_kqueue_read->key.keysym.sym; 
			// XXX Need proper keyboard translation here...
			m_globals->m_shifts = m_globals->m_kqueue_read->key.keysym.mod;
			KQUEUE_INC(m_globals->m_kqueue_read)	
			keyhit = true;
			break;
		}	
	}
	// Check for control toggle
	cl = (m_globals->m_shifts & (KMOD_CTRL | KMOD_RSHIFT));
       	if (cl != (KMOD_CTRL | KMOD_RSHIFT)) cl = 0;
	if (cl != m_LAST_CONTRL)
	{
		m_LAST_CONTRL = cl;
		if (cl)	// Control-Shift just pressed. Set CONTROL_STATUS.
		{
			if (!keyhit) m_CONTROL_STATUS = true;
		}
		else	// Control-Shift released. If it was pressed,
		{	// then toggle control mode.
			if (m_CONTROL_STATUS)
			{
				m_KBD_MOUSE_STS = !m_KBD_MOUSE_STS;
				m_CONTROL_STATUS = 0;
				keyboard_mouse(0, 0, 0);
				// XXX Beep
			}

		}
	}
	// If keyboard is moving the mouse pointer, interpret keypresses
	// as mouse movement
	if (m_KBD_MOUSE_STS && keyhit)
	{
		Uint32 cx = m_TERM_CH;
		Uint32 btn;
		if (m_globals->m_shifts & KMOD_SHIFT)
		{
			cx |= 0x40000000UL;
		}
		for (int n = 0; n < 12; n++)
		{
			if (kbd_mouse_tbl[n*4] == cx)
			{
				btn = kbd_mouse_tbl[n*4 + 1];
				keyboard_mouse(btn & 0x0F, 
					kbd_mouse_tbl[n*4+2],
					kbd_mouse_tbl[n*4+3]);
				if (!(btn & 0x80))
				{
					keyboard_mouse(0,0,0);
				}
				return 0;
			}
		}

	}
	return keyhit ? 1 : 0;
}


/******************************************************************************
*mouse_function                                                               *
*       Entry point for all mouse code                                        *
*                                                                             *
*               Entry   bl = function number                                  *
*                                                                             *
*                       bl = 0 Reserved                                       *
*                                                                             *
*                       bl = 1 Initialize mouse                               *
*                               Set's baud rate, parity, stop bits            *
*                               Initializes the interrupt vector location     *
*                                                                             *
*                              Exit none                                      *
*                                                                             *
*                       bl = 2 Deinitialize the mouse                         *
*                               Puts interrupt vector location back           *
*                               Turns off receive interrupt                   *
*                                                                             *
*                              Exit none                                      *
*                                                                             *
*                       bl =  3 Return mouse status/coordinates               *
*                                                                             *
*                              Exit                                           *
*                                                                             *
*                                       al = 0 nothing happened               *
*                                                                             *
*                                       al = 1 button press                   *
*                                               ah = character information    *
*                                                                             *
*                                       al = 2 coordinate information         *
*                                               x1 = current x                *
*                                               y1 = current y                *
*                                                                             *
*                                                                             *
*******************************************************************************/

short SdSdl::mouse_function(short bl, short *termch)
{
	short rv;
	switch(bl)
	{
		default: return 0;
		case 1: HIDE_CUR(); 	// Init
			m_FIRST_MOTN = false;
			m_mouse_lock = false;
			m_globals->m_mouse_status_byte = 0;
			m_globals->m_mouse_x = xres / 2;
			m_globals->m_mouse_y = yres / 2;
			m_mousedx = 0;
			m_mousedy = 0;
			m_gcurx = xres / 2;
			m_gcury = xres / 2;
			SDL_WarpMouse(xres / 2, yres / 2);
			m_MOUSE_BT = 0;
			return 0;
		case 2: DIS_CUR(); return 0;	// Deinit
		case 3: *termch = m_globals->m_mouse_switch_byte;
			m_X1 = m_globals->m_mouse_x;
			m_Y1 = m_globals->m_mouse_y;
			rv = m_globals->m_mouse_status_byte;
			m_globals->m_mouse_status_byte = 0;
			return rv;
	}
}

bool SdSdl::handleEvent(SDL_Event *ev)
{
	short mask;
	SDL_Event *ev2 = m_globals->m_kqueue_write;
	KQUEUE_INC(ev2);

	switch(ev->type)
	{
		case SDL_KEYUP:
			m_globals->m_shifts = ev->key.keysym.mod;
			if (ev2 == m_globals->m_kqueue_read) // queue full
				return true;
			m_globals->m_kqueue_write[0] = *ev;
			m_globals->m_kqueue_write = ev2;
			return true;
		case SDL_KEYDOWN:
			m_globals->m_shifts = ev->key.keysym.mod;
			if (ev2 == m_globals->m_kqueue_read) // queue full
				return true;
			m_globals->m_kqueue_write[0] = *ev;
			m_globals->m_kqueue_write = ev2;
			return true;
		case SDL_MOUSEMOTION:
			m_CONTROL_STATUS = 0;
			if (!m_FIRST_MOTN)
			{
				m_FIRST_MOTN = true;
				m_KBD_MOUSE_STS = 0;
			}
			if (m_mouse_lock) return true;
			m_mouse_lock = true;	
			// Call user button callback if there is one.
			(*m_globals->m_motv)(this, ev->motion.x, ev->motion.y);
			m_globals->m_mouse_x = m_gcurx = ev->motion.x;
			m_globals->m_mouse_y = m_gcury = ev->motion.y;
			(*m_globals->m_curv)(this, ev->motion.x, ev->motion.y);
			
			m_mouse_lock = false;	
			break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			mask = 0;
			switch(ev->button.button)
			{
				case 1: mask = 1; break;
				case 3: mask = 2; break;
				case 2: mask = 4; break;
			}
			if (ev->type == SDL_MOUSEBUTTONUP)
				m_MOUSE_BT &= ~mask;
			else	m_MOUSE_BT |=  mask;
// Only used for PS/2 mouse	m_old_mouse_but = m_MOUSE_BT;
			
			// Call user button callback if there is one.
			(*m_globals->m_butv)(this, m_MOUSE_BT);
			
			// If there's a pending button event don't queue
			// another.
			if (m_globals->m_mouse_status_byte & 1) return true;
			switch(ev->button.button)
			{
				case 1: m_globals->m_mouse_switch_byte = 0x20; break;
				case 3: m_globals->m_mouse_switch_byte = 0x22; break;
				case 2: m_globals->m_mouse_switch_byte = 0x21; break;
			}
			m_globals->m_mouse_status_byte |= 1;
			return true;	
	}	
	return false;
}


void SdSdl::keyboard_mouse(short button, Sint32 dx, Sint32 dy)
{
	Sint32 x,y;
	m_FIRST_MOTN = true;

	if (dx || dy)
	{
		m_globals->m_mouse_status_byte = 2;
		x = m_globals->m_mouse_x + dx;
		y = m_globals->m_mouse_y + dy;
		clip_cross(&x, &y);
		m_globals->m_mouse_x = x;
		m_globals->m_mouse_y = y;
		(*m_globals->m_motv)(this, x, y);
		(*m_globals->m_curv)(this, x, y);
	}
	else
	{
		m_globals->m_mouse_status_byte = 1;
		m_MOUSE_BT = button;
		(*m_globals->m_butv)(this, button);
		switch(button & 3)
		{
			case 1: m_globals->m_mouse_switch_byte = 0x20; break;
			case 2: m_globals->m_mouse_switch_byte = 0x21; break;
			case 3: m_globals->m_mouse_switch_byte = 0x22; break;
		}
	}
}



void SdSdl::clip_cross(Sint32 *x, Sint32 *y)
{
	// Unlike real GEM, we don't mind if the mouse pointer isn't 
	// completely visible.
	if (*x < 0) *x = 0;
	if (*x >= m_globals->m_xres) *x = m_globals->m_xres;
	if (*y < 0) *y = 0;
	if (*y >= m_globals->m_yres) *y = m_globals->m_yres;
}
