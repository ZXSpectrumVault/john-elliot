/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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

class PcwInput
{
protected:
	PcwSystem *m_sys;
public:
	PcwInput(PcwSystem *s);
	virtual ~PcwInput();

	virtual int  handleEvent(SDL_Event &e) = 0;
	virtual void poll(Z80 *R);
        virtual void edfe(Z80 *R);
	virtual byte in(word address);
	virtual void out(word address, byte value);
};


