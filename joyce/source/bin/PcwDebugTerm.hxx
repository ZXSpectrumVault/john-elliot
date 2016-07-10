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

//
// Debugger console
//

class PcwDebugTerm: public PcwSdlTerm
{
	friend class PcwSystem;

public:
	PcwDebugTerm(PcwSystem *s);
	virtual ~PcwDebugTerm();
        void setSysVideo(PcwSdlContext *s);
        virtual PcwTerminal &operator << (unsigned char c);

protected:
        virtual void onGainFocus();     // Terminal is gaining focus
        virtual void onLoseFocus();     // Terminal is about to lose focus
};

