/************************************************************************

    SdlTerm v1.01: Terminal emulation in an SDL framebuffer
                  Note that this does not parse escape sequences;
                  this is done by subclasses (of which, at the moment,
                  SdlVt52 is the only one)

    Copyright (C) 1996, 2001, 2006  John Elliott <seasip.webmaster@gmail.com>

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

#include <string>	// C++ standard string, please

// Raster ops
#define SDLT_DRAW  0
#define SDLT_AND   1
#define SDLT_OR    2
#define SDLT_XOR   3

// Cursor shapes
#define CSR_MIN       1 // First shape
#define CSR_BLOCK     1	// Block
#define CSR_HALFBLOCK 2 // Half-block
#define CSR_UNDERLINE 3 // Underline
#define CSR_MAX       3 // Last shape

// Shift states
#define SH_NONE       0
#define SH_SHIFT      1
#define SH_CTRL       2
#define SH_CSHIFT     (SH_CTRL | SH_SHIFT)
#define SH_ALT        4
#define SH_ASHIFT     (SH_ALT | SH_SHIFT)
#define SH_ACTRL      (SH_ALT | SH_CTRL)
#define SH_ACSHIFT    (SH_ALT | SH_CTRL | SH_SHIFT)

class SdlTerm : public SdlContext
{
public:
	SdlTerm(SDL_Surface *s, SDL_Rect *bounds = NULL);
	virtual ~SdlTerm();

public:
	void	onTimer50(void);	// Must be called at ~50Hz

////////////////////////////////////////////////////////////////////////////
// Input
	unsigned char read(void);
	bool kbhit(void);
	int  onEvent(SDL_Event &ev);
	int  poll(void);
// Keyboard reassignment
        void assignKeyToken(const char *keyname, int shifts, const char *s);
        char *getKeyToken(const char *keyname, int shifts);

protected:
	int  onKeyUp(SDL_KeyboardEvent &ev);
	int  onKeyDown(SDL_KeyboardEvent &ev);

private:
	void pushChar(std::string &s);
	void pushChar(unsigned char c);	 // Add character to typeahed FIFO
	bool pullChar(unsigned char *c); // Retrieve from typeahead FIFO
	void resetInput(void);
	void resetKeyMap(void);

	std::string m_inpBuf;
	bool m_keyState[SDLK_LAST];
	std::string *m_keyMap[8][SDLK_LAST];

////////////////////////////////////////////////////////////////////////////
// Output
public:
        void    reset(void);
        void    setFont(Uint8 *font);
        inline Uint8    *getFont() { return m_font; }

// Status line
	void	enableStatus(bool bEnable = true);
	inline bool isStatusEnabled()         { return m_statLine; }
	void clearStatus(void);
	void sysCursorRight(void);
// Special effects
	inline bool isBold()                  { return m_bold;    }
	inline void setBold(bool b = true)    { m_bold = b;       }
	inline bool isGrey()                  { return m_grey;    }
	inline void setGrey(bool b = true)    { m_grey = b;       }
	inline bool isItalic()                { return m_italic;  }
	inline void setItalic(bool b = true)  { m_italic = b;     }
        inline bool isReverse()               { return m_reverse; }
        inline void setReverse(bool b = true) { m_reverse = b;    }
        inline bool isAltFont()               { return m_altFont; }
        inline void setAltFont(bool b = true) { m_altFont = b;    }
	inline Uint8 getUnderline()           { return m_ul;      }
	inline void setUnderline(Uint8 ul = 0xFF) { m_ul = ul;    }
	void   setForeground(Uint8 r, Uint8 g, Uint8 b);
	void   setBackground(Uint8 r, Uint8 g, Uint8 b);
	inline void   setForeground(SDL_Colour &c) { setForeground(c.r,c.g,c.b); }
	inline void   setBackground(SDL_Colour &c) { setBackground(c.r,c.g,c.b); }
	inline void   setForeground(Uint32 pixel) { m_pixelFG = pixel; }
	inline void   setBackground(Uint32 pixel) { m_pixelBG = pixel; }
	inline Uint32 getForeground() { return m_pixelFG; }
	inline Uint32 getBackground() { return m_pixelBG; }
	void getForeground(Uint8 *r, Uint8 *g, Uint8 *b);
        void getBackground(Uint8 *r, Uint8 *g, Uint8 *b);

// Cursor
	void	enableCursor(bool bEnable = true);
	inline  bool isCursorEnabled() { return m_curEnabled; }
	void    setCursorShape(int shape = CSR_BLOCK);
	inline  int getCursorShape() { return m_curShape; }

private:
	void hideCursor(void);
	void showCursor(void);
	void drawCursor(void);
public:
// Moving the cursor
	void    cr(void);
	void	lf(void);
	void	cursorUp(bool wrap);
	void 	cursorDown(bool wrap);
	void 	cursorLeft(bool wrap);
	void	cursorRight(bool wrap);
	inline  void home(void) { moveCursor(0,0); }
	void    moveCursor(int x, int y);
	void 	saveCursorPos(void);
	inline void	getCursorPos(int *x, int *y) { *x = m_cx; *y = m_cy; }
	inline void	loadCursorPos(void) { moveCursor(m_scx, m_scy); }
	
	inline  void setWrap(bool b = true)     { m_wrap = b;        }
	inline  bool isWrap(void)               { return m_wrap;     }
	inline  void setPageMode(bool b = true) { m_pageMode = b;    }
	inline  bool isPageMode(void)           { return m_pageMode; } 

// Windowing
	void setCentred(int w, int h);
	void setMaxWin(void);
	void setWindow(int x, int y, int w, int h);
	void getWindow(SDL_Rect *rc);

// Clearing bits of screen
 	void clearWindow(void);
	void clearLineRange(int first, int last);
	void clearCurLine(void);
	void clearEOS(void);
	void clearEOL(void);
	void clearBOS(void);
	void clearBOL(void);
// Insert/delete bits
	void insertLine(void);
	void deleteLine(void);
	void deleteChar(void);
// Scrolling
 	void scrollUp(void);
	void scrollLineRangeUp(int first, int last);
	void scrollLineRangeDown(int first, int last);
	void scrollDown(void);
// Misc
	void setLanguage(int lang);
	void setMode(int mode);
	void blit(SDL_Rect &sr, SDL_Rect &dr);
// Beeper
 	void bel(void);
// Output
	inline SdlTerm& operator << (signed char c) 
		{ return operator<<((unsigned char)c); }
	SdlTerm& operator << (unsigned char c);
	SdlTerm& operator << (const char *s);
	SdlTerm& operator << (Uint16 u);
        SdlTerm& operator << (Sint16 s);
        SdlTerm& operator << (Uint32 u);
        SdlTerm& operator << (Sint32 s);

        virtual void    writeSys(unsigned char c);

protected:
	virtual void	write(unsigned char c);

	void rawGlyph(int x, int y, Uint8 *bitmap, int rast_op = SDLT_DRAW);
	void glyph(unsigned char ch);
	void glyphSys(unsigned char ch);

protected:
	bool	m_curEnabled;	// Cursor enabled?
	bool	m_curVisible;	// Has cursor actually been drawn?
	int	m_curShape;	// Cursor shape (block or underline?)
	SDL_Rect m_windowP, m_windowC;
	int	m_charw, m_charh, m_charmax;	// Font dimensions (PSF)
	int	m_cx, m_cy, m_scx, m_scy, m_slcx;

	bool	m_bold, m_grey, m_italic, m_reverse, m_altFont; // Special effects
	Uint8   m_ul;	// Underline bitmap
	bool	m_wrap;	// Wrap text at left/right edge?
	bool	m_pageMode; // Wrap text at bottom of screen?
	bool	m_statLine; // Status line enabled?
	int	m_language; // Current language
	int	m_autoX;    // When the status line is disabled, expand viewport?
	
	Uint8  *m_font;	// Font (PSF format)
	Uint32 m_pixelFG, m_pixelBG;	// Pixel values for text fore/background
// For batching character updates when drawing strings...
	int	m_x0, m_x1, m_y0, m_y1;
	bool	m_oldManualUpdate;
private:
	void calcWindow();
	void lswap(int language);
	void beginMergeUpdate(void);	// Queue all calls to SDL_UpdateRect...
	void endMergeUpdate(void);	// ... until this is called.
	void condUpdate(SDL_Rect *rc);	
	void condUpdate(int x, int y, int w, int h);	
};


