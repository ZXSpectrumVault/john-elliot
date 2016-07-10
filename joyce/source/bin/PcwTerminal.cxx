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

#include "Pcw.hxx"

PcwTerminal::PcwTerminal(const char *devname) : 
	PcwDevice("terminal", devname)
{
	m_framebuf = NULL;
	m_sysVideo = NULL;
	m_debugMode = false;
	reset();
}


PcwTerminal::~PcwTerminal()
{
	setSysVideo(NULL);
}


//
// Hook this terminal up to system video
//
void PcwTerminal::setSysVideo(PcwSdlContext *s)
{
        if (m_framebuf)
        {
                SDL_FreeSurface(m_framebuf->getSurface());
                delete m_framebuf;
        }
	m_sysVideo = s;
	if (!s) return;	

	SDL_Surface *s1 = s->getSurface();
	SDL_Surface *s2 = SDL_AllocSurface(SDL_SWSURFACE,
				s1->w, s1->h, s1->format->BitsPerPixel, 0,0,0,0);
	if (s2) 
	{
        	SDL_SetColours(s2, s->m_colours, 0, 256);
		m_framebuf = new SdlContext(s2);
	}
}



void PcwTerminal::onGainFocus()
{
	if (m_framebuf && m_sysVideo)
	{
                SDL_Rect rc;
                SDL_Surface *s = m_sysVideo->getSurface();

                rc.x = 0;
		rc.y = 0;
                rc.w = s->w;
                rc.h = s->h / 2;

		if (!m_debugMode) 
			SDL_BlitSurface(m_framebuf->getSurface(), NULL, s, NULL);
		else	SDL_BlitSurface(m_framebuf->getSurface(), &rc,  s, &rc);
			
		SDL_UpdateRect(s, 0, 0, s->w, s->h);	
	}
}


void PcwTerminal::onLoseFocus()
{
	if (m_framebuf && m_sysVideo)
	{
                SDL_BlitSurface(m_sysVideo->getSurface(), NULL,
                                m_framebuf->getSurface(), NULL);
	}
}

void PcwTerminal::onDebugSwitch(bool d)
{
	m_debugMode = d;
}

void PcwTerminal::onScreenTimer()
{

}

void PcwTerminal::out(word port, byte value)
{

}


byte PcwTerminal::in (word port)
{
	return 0xFF;
}


void PcwTerminal::reset(void)
{
	beepOff();
}

///////////////////////////////////////////////////////////////////////////
//
// Output
//
PcwTerminal & PcwTerminal::operator << (const char *s)
{
	while (*s) { (*this) << ((unsigned char)(*s)); ++s; }

	return *this;
}


PcwTerminal & PcwTerminal::operator << (int i)
{
	return printf("%d", i);
}


PcwTerminal & PcwTerminal::vprintf(const char *s, va_list args)
{
	char buf[2048];
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, 2047, s, args);
#else
	vsprintf(buf, s, args);
#endif
	return operator << (buf);	
}


PcwTerminal & PcwTerminal::printf(const char *s, ...)
{
	va_list ap;
	va_start (ap, s);
	vprintf(s, ap);
	va_end(ap);
	return (*this);
}


void PcwTerminal::beepOff(void)
{
	m_beepStat = 0;
}

void PcwTerminal::beepOn(void)
{
	m_beepStat = 1;
}



void flipXbm(int w, int h, Uint8* bits)
{
        int n, m;
        byte b, b2;

        for (n = (((w + 7) / 8) * h) - 1; n >= 0; n--)
        {
                b = bits[n];
                b2 = 0;
                for (m = 0; m < 8; m++)
                {
                        b2 = b2 >> 1;
                        if (b & 0x80) b2 |= 0x80;
                        b  = b  << 1;
                }
                bits[n] = b2;
        }
}


void PcwTerminal::tick() {}

bool PcwTerminal::doBlit(unsigned xfrom, unsigned yfrom, unsigned w, unsigned h,
                                unsigned xto, unsigned yto)
{
	return false;
}

bool PcwTerminal::doRect(unsigned x, unsigned y, unsigned w, unsigned h,
			 byte fill, byte rasterOp, byte *pattern)
{
	return false;
}




bool PcwTerminal::doBitmap(unsigned x, unsigned y, unsigned w, unsigned h,
                                byte *data, byte *mask, int slice)
{
	return false;
}


