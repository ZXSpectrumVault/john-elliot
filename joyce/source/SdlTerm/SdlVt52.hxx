/************************************************************************

    SdlTerm v1.00: Terminal emulation in an SDL framebuffer
                  SdlVt52 parses escape sequences and passes them
                  down to SdlTerm for action.

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

class SdlVt52 : public SdlTerm
{
public:
	SdlVt52(SDL_Surface *s, SDL_Rect *bounds = NULL);
	virtual ~SdlVt52();

protected:
        virtual void write(unsigned char c);
	virtual void writeSys(unsigned char c);
protected:
	void escMulti(unsigned char first, int count);
	char m_esc;	
	int  m_escCount;
	unsigned char m_escBuf[10];
};


