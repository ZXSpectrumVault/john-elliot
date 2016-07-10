/************************************************************************

    SdlContext v1.00: C++ wrapper for an SDL surface

    Copyright (C) 2001  John Elliott <seasip.webmaster@gmail.com>

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

#include "uksdl.hxx"
#include <string.h>	/* for memcpy */

class SdlContext
{
public:
	SdlContext(SDL_Surface *s, SDL_Rect *bounds = NULL);
	virtual ~SdlContext();

public:
	bool	lock(void);
	void	unlock(void);

	void	rect(const SDL_Rect &rc, Uint32 pixel);
	void	rect(int x, int y, int w, int h, Uint32 pixel);	
	void	fillRect(const SDL_Rect &rc, Uint32 pixel);
	void	fillRect(int x, int y, int w, int h, Uint32 pixel);

	inline SDL_Surface *getSurface() { return m_surface; }
	inline SDL_Rect    &getBounds()  { return m_bounds;  }

        inline void setManualUpdate(bool b = true) { m_manualUpdate = b; }
        inline bool getManualUpdate(void) { return m_manualUpdate; }

	void dot(int x, int y, Uint32 pixel);
        void dotA(int x, int y, Uint32 pixel);
        void dotO(int x, int y, Uint32 pixel);
        void dotX(int x, int y, Uint32 pixel);

	Uint8 *getDotPos(int x, int y, int *bpp);
	Uint32 pixelAt(int x, int y);

protected:
	bool	m_manualUpdate;
	Uint32 pixelAt(Uint8 *bits, int bpp);
        void dot(Uint8 *bits, Uint32 pixel, int bpp);
        void dotA(Uint8 *bits, Uint32 pixel, int bpp);
        void dotO(Uint8 *bits, Uint32 pixel, int bpp);
        void dotX(Uint8 *bits, Uint32 pixel, int bpp);
protected:
	SDL_Surface *m_surface;
	SDL_Rect m_bounds;
};


#define SXLOG(s)

//#define SXLOG(s) do { FILE *fp = fopen("xlog", "a"); \
//                    fprintf(fp, "%s...", s); fclose(fp); \
//                    std::string *str[8000]; int n; \
//                    for (n = 0; n < 8000; n++) str[n] = new std::string(); \
//                    for (n = 0; n < 8000; n++) delete str[n]; \
//                    fp = fopen("xlog", "a"); \
//                    fprintf(fp, "OK\n", s); fclose(fp); } while(0)

