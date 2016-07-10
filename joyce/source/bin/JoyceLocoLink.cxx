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

// The PCW (slave) LocoLink interface

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

JoyceLocoLink::JoyceLocoLink() : PcwLibLink("locolink", "slave") 
{
	XLOG("JoyceLocoLink::JoyceLocoLink()");
}


JoyceLocoLink::~JoyceLocoLink()
{
	XLOG("JoyceLocoLink::~JoyceLocoLink()");
}
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool JoyceLocoLink::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_l;
	caption = "  LocoLink  ";	
	return true;
}



void JoyceLocoLink::out(byte b)
{
	if (!isEnabled()) return;

        if (!m_lldev) open();
        if (!m_lldev) return;
	ll_error_t e = ll_lsend(m_lldev, 0, b);
	showError(e);
}


byte JoyceLocoLink::in(void)
{
	byte b;

	if (!isEnabled()) return 0xFF;
        if (!m_lldev) open();
        if (!m_lldev) return 0xFF;
	ll_error_t e = ll_lrecv_poll(m_lldev, 0, &b);
	showError(e);
	return b;
}


string JoyceLocoLink::getTitle()
{
	return "  LocoLink interface  ";
}
