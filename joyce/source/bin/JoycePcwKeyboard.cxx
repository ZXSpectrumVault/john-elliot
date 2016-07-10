/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

*************************************************************************/


#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiMenu.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiSetting.hxx"
#include "UiCommand.hxx"

#define UP    1
#define DOWN  2
#define LEFT  4
#define RIGHT 8
#define FIRE1 16
#define FIRE2 32

//
// The default PC -> PCW keymap
//
static Uint16 gl_defaultMap[] = { 
	SDLK_BACKSPACE, 9, 128, 9, 128,
	SDLK_TAB,       8, 16,  8, 16,
//	SDLK_CLEAR,
	SDLK_RETURN,    2, 4,   2, 4, 
	SDLK_ESCAPE,    1, 1,   1, 1,
	SDLK_SPACE,     5, 128, 5, 128,
	SDLK_QUOTE,     3, 16,  3, 16,
	SDLK_COMMA,     4, 128, 4, 128,
	SDLK_MINUS,     3, 2,   3, 2,
	SDLK_PERIOD,    3, 128, 3, 128,
	SDLK_SLASH,     3, 64,  3, 64,
	SDLK_0,         4, 1,   4, 1,
	SDLK_1,         8, 1,   8, 1,
	SDLK_2,         8, 2,   8, 2,
 	SDLK_3,         7, 2,   7, 2,
	SDLK_4,         7, 1,   7, 1,
	SDLK_5,         6, 2,   6, 2,
	SDLK_6,         6, 1,   6, 1,
	SDLK_7,         5, 2,   5, 2,
	SDLK_8,         5, 1,   5, 1,
	SDLK_9,         4, 2,   4, 2,
	SDLK_BACKSLASH, 2, 64,  2, 64,
	SDLK_SEMICOLON, 3, 32,  3, 32,
	SDLK_EQUALS,    3, 1,   3, 1,
	SDLK_KP0,       0, 2,   0, 2,
	SDLK_KP1,       2, 16,  1, 128,
	SDLK_KP2,       10,64,  0, 128,
	SDLK_KP3,       0, 16,  0, 64,
	SDLK_KP4,       1,128,  1, 32,
	SDLK_KP5,       0,128,  1, 64,
	SDLK_KP6,       0, 64,  0, 32,
	SDLK_KP7,       1, 32,  2, 16,
	SDLK_KP8,       1, 64,  1, 16,
	SDLK_KP9,       0, 32,  0, 16,
	SDLK_KP_PERIOD, 10, 4, 10, 64,
	SDLK_KP_DIVIDE,  3,64,  3, 64,
	SDLK_KP_MULTIPLY,1, 2,  1,  2,
	SDLK_KP_MINUS,  10,  8,10,  8,	// [v1.9.3]
	SDLK_KP_PLUS,    2,128, 2,128,
	SDLK_KP_ENTER,   10,32,10, 32,
//	SDLK_KP_EQUALS = 86,
	SDLK_LEFTBRACKET,3,  4, 3,  4,
	SDLK_HASH,        2, 8, 2,  8,
	SDLK_RIGHTBRACKET,2, 2, 2,  2,
	SDLK_BACKQUOTE,   8, 4, 8,  4,
	SDLK_a,           8,32, 8, 32,
	SDLK_b,           6,64, 6, 64,
	SDLK_c,          7, 64, 7, 64,
	SDLK_d,          7, 32, 7, 32,
	SDLK_e,          7,  4, 7,  4,
	SDLK_f,          6, 32, 6, 32,
	SDLK_g,          6, 16, 6, 16,
	SDLK_h,          5, 16, 5, 16,
	SDLK_i,          4,  8, 4,  8,
	SDLK_j,          5, 32, 5, 32,
	SDLK_k,          4, 32, 4, 32,
	SDLK_l,          4, 16, 4, 16,
	SDLK_m,          4, 64, 4, 64,
	SDLK_n,          5, 64, 5, 64,
	SDLK_o,          4,  4, 4,  4,
	SDLK_p,          3,  8, 3,  8,
	SDLK_q,          8,  8, 8,  8,
	SDLK_r,          6,  4, 6,  4,
	SDLK_s,          7, 16, 7, 16,
	SDLK_t,          6,  8, 6,  8,
	SDLK_u,          5,  4, 5,  4,
	SDLK_v,          6,128, 6,128,
	SDLK_w,          7,  8, 7,  8,
	SDLK_x,          7,128, 7,128,
	SDLK_y,          5,  8, 5,  8,
	SDLK_z,          8,128, 8,128,
	SDLK_DELETE,     2,  1, 2,  1,
	SDLK_F1,         0,  4, 0,  4,
	SDLK_F2,         0,  4, 0,  4,
	SDLK_F3,         0,  1, 0,  1,
	SDLK_F4,         0,  1, 0,  1,
	SDLK_F5,        10,  1,10,  1,
	SDLK_F6,        10,  1,10,  1,
	SDLK_F7,        10, 16,10, 16,
	SDLK_F8,        10, 16,10, 16,
	SDLK_F9,        16,  1,16,  1,	// Joyce actions
	SDLK_F10,       16,  2,16,  2,	// Joyce exit
	SDLK_F11,        2,128, 2,128,
	SDLK_F12,       10,  8,10,  8,
	SDLK_F13,       10,  2,10,  2,
	SDLK_F14,       10,  2,10,  2,
	SDLK_F15,       16,  1,16,  1,
	SDLK_PAUSE,     16,  1,16,  1,
	SDLK_NUMLOCK,    0,  2, 0,  2,
	SDLK_UP,         1, 64, 1, 64,
	SDLK_DOWN,      10, 64,10, 64,
	SDLK_RIGHT,      0, 64, 0, 64,
	SDLK_LEFT,       1,128, 1,128,
	SDLK_INSERT,     1,  4, 1,  4,
	SDLK_HOME,       1,  8, 1,  8,
	SDLK_END,       10,  4,10,  4,	/* Was 9, 8, 9, 8 ???? */
	SDLK_PAGEUP,     0,  8, 0,  8,
	SDLK_PAGEDOWN,   1, 16, 1, 16,
	SDLK_CAPSLOCK,   10,32,10, 32,
	SDLK_SCROLLOCK,  8, 64, 8, 64,
	SDLK_RSHIFT,     2, 32, 2, 32,
	SDLK_LSHIFT,     2, 32, 2, 32,
	SDLK_RCTRL,      10, 2, 10, 2,
	SDLK_LCTRL,      10, 2, 10, 2,
	SDLK_RALT,       10,128,10,128,
	SDLK_LALT,       10,128,10,128,
	SDLK_RMETA,      10,128,10,128,
	SDLK_LMETA,      10,128,10,128,
//	SDLK_HELP = 164,
	SDLK_PRINT,      1, 2, 1, 2,  
	SDLK_SYSREQ,     1, 2, 1, 2,
	SDLK_MENU,       16,1,16, 1,
	SDLK_BREAK,      16,2,16, 2,
//	SDLK_EURO,     
	SDLK_POWER,      16,2,16,2, 
	SDLK_LSUPER,     10,2,10,2,
	SDLK_RSUPER,     10,2,10,2,
	0
};

static unsigned char stick[] = 
{
/* WADX (if LK2 is present) */
	0x07, 0x08, 0x0C, 0x01,
	0x07, 0x80, 0x0C, 0x02,
	0x08, 0x20, 0x0C, 0x04,
	0x07, 0x20, 0x0C, 0x08,
	0x07, 0x10, 0x0C, 0x10,
	0x02, 0x20, 0x0C, 0x20,
/* Inverted-T */
	0x00, 0x01, 0x0C, 0x01,
	0x00, 0x04, 0x0C, 0x02,
	0x01, 0x01, 0x0C, 0x04,
	0x00, 0x02, 0x0C, 0x08,
	0x05, 0x80, 0x0C, 0x10,
	0x0A, 0x20, 0x0C, 0x20,
/* Keypad */
	0x01, 0x40, 0x0D, 0x01,
	0x0A, 0x40, 0x0D, 0x02,
	0x01, 0x80, 0x0D, 0x04,
	0x00, 0x40, 0x0D, 0x08,
	0x00, 0x80, 0x0D, 0x10,
	0x05, 0x80, 0x0D, 0x20,
/* ASDFGHJ sets */
	0x08, 0x20, 0x0E, 0x01,	/* A */
	0x07, 0x10, 0x0E, 0x01,	/* S */
	0x07, 0x20, 0x0E, 0x01,	/* D */
	0x06, 0x20, 0x0E, 0x01,	/* F */
	0x06, 0x10, 0x0E, 0x01,	/* G */ 
	0x05, 0x10, 0x0E, 0x01,	/* H */ 
	0x05, 0x20, 0x0E, 0x01,	/* J */ 
	0x08, 0x80, 0x0E, 0x02,	/* Z */ 
	0x07, 0x80, 0x0E, 0x02,	/* X */ 
	0x07, 0x40, 0x0E, 0x02,	/* C */ 
	0x06, 0x80, 0x0E, 0x02,	/* V */ 
	0x06, 0x40, 0x0E, 0x02,	/* B */ 
	0x05, 0x40, 0x0E, 0x02,	/* N */ 
	0x04, 0x40, 0x0E, 0x02,	/* M */ 
	0x08, 0x08, 0x0E, 0x04,	/* Q */	
	0x07, 0x04, 0x0E, 0x04,	/* E */	
	0x04, 0x04, 0x0E, 0x04,	/* O */	
	0x03, 0x04, 0x0E, 0x04,	/* [ */	
	0x04, 0x10, 0x0E, 0x04,	/* L */	
	0x03, 0x10, 0x0E, 0x04,	/* < */	
	0x04, 0x80, 0x0E, 0x04,	/* , */	
	0x03, 0x40, 0x0E, 0x04,	/* / */	
	0x07, 0x08, 0x0E, 0x08,	/* W */	
	0x06, 0x08, 0x0E, 0x08,	/* R */	
	0x03, 0x08, 0x0E, 0x08,	/* P */	
	0x02, 0x04, 0x0E, 0x08,	/* ] */	
	0x03, 0x20, 0x0E, 0x08,	/* ; */	
	0x02, 0x08, 0x0E, 0x08, /* > */
	0x03, 0x80, 0x0E, 0x08,	/* . */
	0x02, 0x40, 0x0E, 0x08,	/* 1/2 */
	0x05, 0x80, 0x0E, 0x10, /* Space */
	0x02, 0x20, 0x0E, 0x20, /* Shift */
/* HJKL sets */
	0x05, 0x10, 0x0F, 0x01,	/* H */ 
	0x05, 0x20, 0x0F, 0x01,	/* J */ 
	0x04, 0x20, 0x0F, 0x01,	/* K */	
	0x04, 0x10, 0x0F, 0x01,	/* L */	
	0x03, 0x20, 0x0F, 0x01,	/* ; */	
	0x03, 0x10, 0x0F, 0x01,	/* < */	
	0x02, 0x08, 0x0F, 0x01, /* > */
	0x06, 0x40, 0x0F, 0x02,	/* B */ 
	0x05, 0x40, 0x0F, 0x02,	/* N */ 
	0x04, 0x40, 0x0F, 0x02,	/* M */ 
	0x04, 0x80, 0x0F, 0x02,	/* , */	
	0x03, 0x80, 0x0F, 0x02,	/* . */
	0x03, 0x40, 0x0F, 0x02,	/* / */	
	0x02, 0x40, 0x0F, 0x02,	/* 1/2 */
	0x08, 0x08, 0x0F, 0x04,	/* Q */	
	0x07, 0x04, 0x0F, 0x04,	/* E */	
	0x04, 0x04, 0x0F, 0x04,	/* O */	
	0x03, 0x04, 0x0F, 0x04,	/* [ */	
	0x08, 0x20, 0x0F, 0x04,	/* A */
	0x07, 0x20, 0x0F, 0x04,	/* D */
	0x08, 0x80, 0x0F, 0x04,	/* Z */
	0x07, 0x40, 0x0F, 0x04,	/* C */
	0x07, 0x08, 0x0F, 0x08,	/* W */	
	0x06, 0x08, 0x0F, 0x08,	/* R */	
	0x03, 0x08, 0x0F, 0x08,	/* P */	
	0x03, 0x04, 0x0F, 0x08,	/* [ */	
	0x07, 0x10, 0x0F, 0x08,	/* S */
	0x06, 0x20, 0x0F, 0x08,	/* F */
	0x07, 0x80, 0x0F, 0x08,	/* X */ 
	0x06, 0x80, 0x0F, 0x08,	/* V */ 
	0x05, 0x80, 0x0F, 0x10, /* Space */
	0x02, 0x20, 0x0F, 0x20, /* Shift */
};

#define JSTK_COUNT (sizeof(stick) / sizeof(stick[0]))



JoycePcwKeyboard::JoycePcwKeyboard(JoyceSystem *s) : 
	PcwDevice("keyboard", "pcw"), 
	PcwInput(s),
	m_stick1("1"),
	m_stick2("2")
{
	XLOG("JoycePcwKeyboard::JoycePcwKeyboard()");
	m_swapAlt = false;
	m_swapDel = false;
	m_lk1 = false;
	m_lk2 = false;
	m_lk3 = false;
	reset();
}


JoycePcwKeyboard::~JoycePcwKeyboard()
{
	XLOG("JoycePcwKeyboard::~JoycePcwKeyboard()");
}

void JoycePcwKeyboard::clearKeys(void)
{
	memset(m_pcwKeyMap, 0, sizeof(m_pcwKeyMap));
	memset(m_oldKeyMap, 0, sizeof(m_oldKeyMap));
}


void JoycePcwKeyboard::reset(void)
{
        int n;
        Uint16 *b;

	clearKeys();
        memset(m_keybMap,   0, sizeof(m_keybMap));

        for (b = gl_defaultMap; *b; b += 5)
        {
                n = b[0];
                m_keybMap[n*4]   = b[1];
                m_keybMap[n*4+1] = b[2];
                m_keybMap[n*4+2] = b[3];
                m_keybMap[n*4+3] = b[4];
        }
	m_autoShift = false;
	m_trueShift = false;
	m_keyPress  = false;
	m_shiftLock = false;

	keySet();
	m_stick1.reset();
	m_stick2.reset();
	m_ticker = 0;
}



void JoycePcwKeyboard::setRange(int *R, byte a, byte b, byte c, byte d)
{
        int n;

        for (; R[0] >= 0; ++R)
        {
                n = *R;
                m_keybMap[n*4]   = a;
                m_keybMap[n*4+1] = b;
                m_keybMap[n*4+2] = c;
                m_keybMap[n*4+3] = d;
        }

}

static int gl_ALTS[] = { SDLK_LALT,  SDLK_RALT, SDLK_LMETA, SDLK_RMETA, -1 };
static int gl_CTLS[] = { SDLK_LCTRL, SDLK_RCTRL, -1 };
static int gl_BKSP[] = { SDLK_BACKSPACE, -1 };
static int gl_DELS[] = { SDLK_DELETE,    -1 };

void JoycePcwKeyboard::keySet(void)
{
        if (m_swapAlt)
        {
                setRange(gl_ALTS, 10,   2, 10,   2);
                setRange(gl_CTLS, 10, 128, 10, 128);
        }
        else
        {
                setRange(gl_ALTS, 10, 128, 10, 128);
                setRange(gl_CTLS, 10,   2, 10,   2);
        }
        if (m_swapDel)
        {
                setRange(gl_BKSP, 2,   1, 2,   1);
                setRange(gl_DELS, 9, 128, 9, 128);
        }
        else
        {
                setRange(gl_BKSP, 9, 128, 9, 128);
                setRange(gl_DELS, 2,   1, 2,   1);
        }
}



int JoycePcwKeyboard::handleEvent(SDL_Event &e)
{
	bool b = false;
	int rv = 0;
	Uint16 *km;
	SDLKey keysym = SDLK_UNKNOWN;
	Uint8 shiftLock = m_pcwKeyMap[8] & 0x40;

	if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) 
	{
		b = (e.type == SDL_KEYDOWN);
		keysym = e.key.keysym.sym; 

///		printf("Key event: b=%d keysym=%d\n", b, keysym);
	}

	if (keysym == SDLK_F2 || keysym == SDLK_F4 || 
	    keysym == SDLK_F6 || keysym == SDLK_F8) m_autoShift = b;

	if (keysym == SDLK_LSHIFT || keysym == SDLK_RSHIFT) m_trueShift = b;
	
	switch(e.type)
	{
		case SDL_QUIT: rv = -99; break;
		case SDL_KEYDOWN:  
			km = m_keybMap + 4 * keysym;	
			m_pcwKeyMap[km[0]] |= km[1];
			m_keyPress = true;
			rv = 1;
			break;
		case SDL_KEYUP:
			km = m_keybMap + 4 * keysym;
			m_pcwKeyMap[km[0]] &= ~km[1];
			m_keyPress = true;
			rv = 1;
			break;
		case SDL_JOYBUTTONDOWN:
			m_keyPress = true;
			break;
		case SDL_JOYBUTTONUP:
			m_keyPress = true;
			break;
		case SDL_JOYAXISMOTION:
			m_keyPress = true;
			break;
	}
	if (rv)
	{
		if ((m_pcwKeyMap[8] & 0x40) && !shiftLock)
		{
			m_shiftLock = !m_shiftLock;
		}

		if (m_trueShift || m_autoShift) m_pcwKeyMap[2] |= 32;
		else			        m_pcwKeyMap[2] &= ~32;
	}
	return rv;
}


void JoycePcwKeyboard::updateMatrix()
{
	unsigned n;
	unsigned first = m_lk2 ? 0 : 24;	/* LK2 to skip WAXD diamond */

	/* Zero bits that are computed by this function. */
	m_pcwKeyMap[12] &= 0xC0;
	m_pcwKeyMap[13] = 0;
	m_pcwKeyMap[14] = 0;
	m_pcwKeyMap[15] &= 0xC0;

	for (n = first; n < JSTK_COUNT; n += 4)
	{
		if (m_pcwKeyMap[stick[n]] & stick[n+1])
		{
			m_pcwKeyMap[stick[n+2]] |= stick[n+3];
		}
	}
	/* If LK2 is 0, Shift cancels Shift Lock */
	if (m_lk2 == 0 && (m_pcwKeyMap[2] & 0x20))
	{
		m_shiftLock = 0;
	}

	/* Include shift lock LED & links */
	if (m_shiftLock)	m_pcwKeyMap[13] |= 0x40;
	if (!m_lk2)		m_pcwKeyMap[13] |= 0x80;
	if (m_lk3)		m_pcwKeyMap[14] |= 0x80;
	if (m_lk1)		m_pcwKeyMap[14] |= 0x40;
}



void JoycePcwKeyboard::poll(Z80 *R)
{
	JoyceZ80 *JR = (JoyceZ80 *)R;

	m_ticker++;

	// Keyboard ticker. Bit 7 set if keyboard is currently sending 
	// data, reset if scanning keys. Bit 6 toggles with each data 
	// transmission, i.e. at half the rate of bit 7. 
	// Timings are, in all probability, miles adrift, but no PCW 
	// software seems to depend on them.
	//
	m_pcwKeyMap[15] &= 0x3F;
	if (m_ticker & 1) m_pcwKeyMap[15] |= 0x80;
	if (m_ticker & 2) m_pcwKeyMap[15] |= 0x40;
	PCWRAM[0xFFFF] = m_pcwKeyMap[15];

	// If there have been no SDL key events received, don't bother doing
	// the full keyboard-scanning routine.
	if (!m_keyPress) return;
	m_keyPress = 0;

	// [2.2.4] Calculate the bits of keyboard RAM that depend on other
	// bits (and on the option links).
	updateMatrix();
	for (int k = 0; k < 16; k++) 
	{
                PCWRAM[0xFFF0+k]  = m_pcwKeyMap[k];
	}	
	// XXX These specials may need to go elsewhere...
        if (m_pcwKeyMap[0x10]&1)
        {
		JR->stopwatch();
                m_sys->m_termMenu->mainMenu(JR);
                for (int keyno=0;keyno<16;keyno++)
                {
                        PCWRAM[0xFFF0+keyno]=0;
                }
                m_pcwKeyMap[0x10] = 0;
                JR->startwatch(1);
                return;
        }
        if (m_pcwKeyMap[0x10]&2)
        {
		JR->stopwatch();
                m_sys->m_termMenu->exitMenu();
                for (int keyno=0;keyno<16;keyno++)
                {
                        PCWRAM[0xFFF0+keyno]=0;
                }
                m_pcwKeyMap[0x10] = 0;
                JR->startwatch(1);
		return;
        }
	PCWRAM[0xFFF9] &= 0xC0;
	PCWRAM[0xFFF9] |= m_stick1.getPosition(LEFT, RIGHT, UP, DOWN, FIRE1, FIRE2);
	PCWRAM[0xFFFB] &= 0xC0;
	PCWRAM[0xFFFB] |= m_stick2.getPosition(LEFT, RIGHT, UP, DOWN, FIRE1, FIRE2);
}



void JoycePcwKeyboard::assignKey(Uint16 keynum, Uint8 l, Uint8 h)
{
        int n = 4 * keynum;
        m_keybMap[n]   = l;
        m_keybMap[n+1] = h;
        m_keybMap[n+2] = l;
        m_keybMap[n+3] = h;
}

void JoycePcwKeyboard::edfe(Z80 *R)
{
        int n, l, h, k, count;

	/* 2.1.9: Key is in C, not B */
        switch(R->c)
        {
                case 0:
                R->l = m_keybMap[R->getDE() * 4];
                R->h = m_keybMap[R->getDE() * 4 + 1];
                break;

                case 1:
		/* 2.1.9: Key token is in DE, not BC. */
                assignKey(R->getDE(), R->l, R->h);
                break;

                case 2:
                count = R->getDE();
                for (n = 0; n < count; n++)
                {
                        k =       fetch2(R->ix + 4*n);
                        l =       fetch(R->ix + 4*n + 2);
                        h =       fetch(R->ix + 4*n + 3);

                        assignKey(k, l, h);
                }
                break;
	}
}



bool JoycePcwKeyboard::hasSettings(SDLKey *key, string &caption)
{
	*key     = SDLK_k;
	caption = "  Keyboard  ";
	return true;
}

UiEvent JoycePcwKeyboard::settings(UiDrawer *d)
{
	UiMenu uim(d);
	UiEvent uie;
	int sel = -1;

	uim.add(new UiLabel    ("   Keyboard settings   ", d)) 
	   .add(new UiSeparator(d))
	   .add(new UiSetting  (SDLK_a,false, "  PC ALT is PCW ALT    ", d))
           .add(new UiSetting  (SDLK_e,false, "  PC ALT is PCW EXTRA  ", d))
	   .add(new UiSeparator(d))
	   .add(new UiSetting  (SDLK_d,false, "  PC DEL is PCW DEL->  ", d))
	   .add(new UiSetting  (SDLK_b,false, "  PC DEL is PCW <-DEL  ", d))
	   .add(new UiSeparator(d))
	   .add(new UiCommand  (SDLK_j,       "  Joystick 1 ", d))
           .add(new UiCommand  (SDLK_k,       "  Joystick 2 ", d))
	   .add(new UiSeparator(d))
	   .add(new UiSetting  (SDLK_1,m_lk1,  "  Link LK1 ", d))
	   .add(new UiSetting  (SDLK_2,m_lk2,  "  Link LK2 ", d))
	   .add(new UiSetting  (SDLK_3,m_lk3,  "  Link LK3 ", d))
	   .add(new UiSeparator(d))
           .add(new UiCommand  (SDLK_ESCAPE,  "  EXIT", d));
 
	d->centreContainer(uim);	
	do
	{
		((UiSetting &)uim[2]).setChecked(!m_swapAlt);
		((UiSetting &)uim[3]).setChecked(m_swapAlt);
		((UiSetting &)uim[5]).setChecked(!m_swapDel);
		((UiSetting &)uim[6]).setChecked(m_swapDel);
		((UiSetting &)uim[11]).setChecked(m_lk1);
		((UiSetting &)uim[12]).setChecked(m_lk2);
		((UiSetting &)uim[13]).setChecked(m_lk3);

		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;

		sel = uim.getSelected();
		switch(sel)
		{
			case 2: m_swapAlt = 0; break;
			case 3: m_swapAlt = 1; break;
			case 5: m_swapDel = 0; break;
			case 6: m_swapDel = 1; break;
			case 8: uie = m_stick1.settings(d); 
				if (uie == UIE_QUIT) return uie;
				break;
			case 9: uie = m_stick2.settings(d); 
				if (uie == UIE_QUIT) return uie;
				break;
			case 11: m_lk1 = !m_lk1; break;
			case 12: m_lk2 = !m_lk2; break;
			case 13: m_lk3 = !m_lk3; break;
			case 15: keySet(); break;
		}
	} while (sel != 15);
	return UIE_OK;
}

//
// nb: Ability to dump state is not yet provided, but would go here.
//

bool JoycePcwKeyboard::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	xmlChar *s = xmlGetProp(cur, (xmlChar *)"swapctrl");
	
	if (s) { m_swapAlt = atoi((char *)s) ? true : false; xmlFree(s); }
	s = xmlGetProp(cur, (xmlChar *)"swapdel");
	if (s) { m_swapDel = atoi((char *)s) ? true : false; xmlFree(s); }
	s = xmlGetProp(cur, (xmlChar *)"lk1");
	if (s) { m_lk1 = atoi((char *)s) ? true : false; xmlFree(s); }
	s = xmlGetProp(cur, (xmlChar *)"lk2");
	if (s) { m_lk2 = atoi((char *)s) ? true : false; xmlFree(s); }
	s = xmlGetProp(cur, (xmlChar *)"lk3");
	if (s) { m_lk3 = atoi((char *)s) ? true : false; xmlFree(s); }

	// See if there are settings for our joysticks
	xmlNode *child;
	for (child = cur->xmlChildrenNode; child; child = child->next)
	{
		if (strcmp((char *)child->name, "joystick") || child->ns != ns)
			continue;
		s = xmlGetProp(child, (xmlChar *)"name");
		if (s == NULL) continue;
		if (!strcmp((char *)s, "1")) m_stick1.parseNode(doc, ns, child);	
		if (!strcmp((char *)s, "2")) m_stick2.parseNode(doc, ns, child);
		xmlFree(s);	
	}	
	return true;
}


bool JoycePcwKeyboard::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	bool found1 = false, found2 = false;
	xmlChar *s;

	xmlSetProp(cur, (xmlChar *)"swapctrl", (xmlChar *)(m_swapAlt ? "1" : "0"));
	xmlSetProp(cur, (xmlChar *)"swapdel",  (xmlChar *)(m_swapDel ? "1" : "0"));
	xmlSetProp(cur, (xmlChar *)"lk1",  (xmlChar *)(m_lk1 ? "1" : "0"));
	xmlSetProp(cur, (xmlChar *)"lk2",  (xmlChar *)(m_lk2 ? "1" : "0"));
	xmlSetProp(cur, (xmlChar *)"lk3",  (xmlChar *)(m_lk3 ? "1" : "0"));

// See if there are entries for the joysticks. If not found, create them.
	xmlNode *child;
	for (child = cur->xmlChildrenNode; child; child = child->next)
	{
		if (strcmp((char *)child->name, "joystick") || child->ns != ns)
			continue;
		s = xmlGetProp(child, (xmlChar *)"name");
		if (s == NULL) continue;
		if (!strcmp((char *)s, "1")) 
		{
			m_stick1.storeNode(doc, ns, child);	
			found1 = true;
		}
		if (!strcmp((char *)s, "2")) 
		{
			m_stick2.storeNode(doc, ns, child);	
			found2 = true;
		}
		xmlFree(s);
	}
	if (!found1)
	{
		child = xmlNewNode(ns, (xmlChar *)"joystick");
		xmlSetProp(child, (xmlChar *)"name", (xmlChar *)"1");
		m_stick1.storeNode(doc, ns, child);
		xmlAddChild(cur, child);
	}	
	if (!found2)
	{
		child = xmlNewNode(ns, (xmlChar *)"joystick");
		xmlSetProp(child, (xmlChar *)"name", (xmlChar *)"2");
		m_stick2.storeNode(doc, ns, child);
		xmlAddChild(cur, child);
	}	
	return true;
}


