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

#include <SDL.h>
#include <string>
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiLabel.hxx"
#include "UiTextInput.hxx"


UiTextInput::UiTextInput(SDLKey id, string prompt, UiDrawer *d) : UiLabel(prompt, d)
{
	setKey(id);

	int ul = prompt.find('_');
	int ur = prompt.rfind('_');
	if (ul > 0)
	{
		m_prompt = prompt.substr(0, ul);
		m_length = ur - ul + 1;
	}
	else
	{
		m_prompt = prompt;
		m_length = 1;
	}
	m_text   = "";
	m_cx     = 0;
}


UiTextInput::~UiTextInput()
{
}



void UiTextInput::draw(int selected)
{
	int dcx, lcx;
	Uint16 w,h;
	string s;
	char ch;

	s = m_text;
	lcx = 0;
	if (m_cx > m_length)
	{
		lcx = m_cx - m_length;
		s = m_text.substr(lcx);
	}	
	m_caption = m_prompt + s.substr(0, m_length);
	
//	m_drawer->hideCursor();
	UiLabel::draw(selected);

	dcx = m_cx - lcx;
	s = m_prompt + m_text.substr(lcx, dcx).substr(0, m_length);
	
	m_drawer->measureString(s, &w, &h);

	s = m_text.substr(m_cx);
	if (s == "") ch = ' ';
	else         ch = s[0];

	m_drawer->drawCursor(getX() + w, getY(), ch);
}


bool UiTextInput::canFocus(void)
{
	return true;
}


UiEvent UiTextInput::onEvent(SDL_Event &ev)
{
	UiEvent e = m_drawer->menuEvent(ev);

	if (e == UIE_QUIT) return e;

	if (e == UIE_CONTINUE) return UiLabel::onEvent(ev);

	char c = m_drawer->getKey();
	if (c) return onChar(c);

	return UIE_CONTINUE;	
}


UiEvent UiTextInput::onChar(char c)
{
	if (c == 127)
	{
		if (m_text.size() && m_cx) 
		{ 
			--m_cx;
			// ministl doesn't have erase()
			string left = m_text.substr(0, m_cx);
			string right = m_text.substr(m_cx + 1);
			m_text = left + right;
			//m_text.erase(m_cx, m_cx+1); 
			draw(1);
		}
		return UIE_CONTINUE;
	}
	if (c >= ' ')
	{
		string l = m_text.substr(0, m_cx);
		string r = m_text.substr(m_cx);

		m_text = l + c;
		m_text += r;
		++m_cx;
		draw(1);
		return UIE_CONTINUE;	
	}
	if (c == '\n' || c == '\r') return UIE_OK;
	if (c == 0x1B) return UIE_CANCEL;
	if (c == 1 && m_cx)
	{
		--m_cx;
		draw(1);
		return UIE_CONTINUE;
	}
	if (c == 6 && m_cx < (int)m_text.size())
	{
		++m_cx;
		draw(1);
		return UIE_CONTINUE;

	}	
	return UIE_CONTINUE;	
}


bool UiTextInput::wantAllKeys(void)
{
	return true;
}

