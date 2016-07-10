/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2002  John Elliott <seasip.webmaster@gmail.com>

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

#ifdef HAVE_WINDOWS_H
#ifndef GDIPRINT_H_INCLUDED

#define GDIPRINT_H_INCLUDED 1

class GdiPrint
{
public:
	GdiPrint();
	~GdiPrint();

	bool startPage();
	bool endPage();
	void putDot(float x, float y);
	void putChar(float x, float y, char c);
protected:
	HDC	m_hDC;
	HPEN	m_hPen, m_hPenOld;
	HBRUSH  m_hBrush, m_hBrushOld;
	HFONT	m_hFont,  m_hFontOld;
	string  m_sDriver;
	string  m_sDevice;
//	string  m_sOutput;
	HANDLE    m_hDevnames;
	HANDLE    m_hDevmode;
	DEVMODE  *m_pDevmode;
	DEVNAMES *m_pDevnames;
	bool	m_docOpen;
	bool	m_pageOpen;
	bool	open();	
public:
	int 	m_fontSize;
	string	m_fontFace;
	void	close();
};
#endif
#endif

