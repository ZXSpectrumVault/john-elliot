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

enum UiGlyph
{
	UIG_NONE, UIG_CHECKED, UIG_UNCHECKED, UIG_COMMAND, UIG_SUBMENU
};

enum UiEvent
{
	UIE_CONTINUE, UIE_SELECT, UIE_OK, UIE_CANCEL, UIE_QUIT
};

#ifndef SDL_Colour
#define SDL_Colour SDL_Color
#endif

using namespace std;

