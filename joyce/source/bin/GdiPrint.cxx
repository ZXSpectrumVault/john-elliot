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
#include "Pcw.hxx"
#include "GdiPrint.hxx"

#ifdef HAVE_WINDOWS_H

GdiPrint::GdiPrint()
{
	XLOG("GdiPrint::GdiPrint()");
	m_hDC = NULL;
	m_pDevmode = NULL;
	m_docOpen = false;
	m_hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);	
	m_hPen   = (HPEN)GetStockObject(BLACK_PEN);

/* Get settings for default printer */
	PRINTDLG pd;
	memset(&pd, 0, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.Flags = PD_RETURNDEFAULT;
	PrintDlg(&pd);

	m_hDevmode  = pd.hDevMode;
	m_hDevnames = pd.hDevNames;

	/* v2.1.6 Deal with NULL return */
	if (m_hDevmode)	m_pDevmode  = (LPDEVMODE )GlobalLock(m_hDevmode);
	else		m_pDevmode  = NULL;

	if (m_hDevnames) m_pDevnames = (LPDEVNAMES)GlobalLock(m_hDevnames);
       	else	         m_pDevnames = NULL;

	if (m_pDevnames)
	{
		m_sDriver = ((char *)m_pDevnames) + m_pDevnames->wDriverOffset;
		m_sDevice = ((char *)m_pDevnames) + m_pDevnames->wDeviceOffset;
	}
	else
	{
		m_sDriver = "";
		m_sDevice = "";
	}
	string s = "GdiPrint::GdiPrint() m_sDriver = " + m_sDriver +
                   "m_sDevice = " + m_sDevice;
	XLOG(s.c_str());
	m_fontSize = 100;
	m_fontFace = "Courier New";
}


GdiPrint::~GdiPrint()
{
	if (m_hDC) close();
	DeleteObject(m_hPen);
	DeleteObject(m_hBrush);
	if (m_hDevmode) 
	{
		GlobalUnlock(m_hDevmode); 
		GlobalFree(m_hDevmode);
	}
	if (m_hDevnames)
	{
		GlobalUnlock(m_hDevnames);
		GlobalFree(m_hDevnames);
	}
}

bool GdiPrint::startPage()
{
	if (!m_hDC) 
	{
		if (!open()) return false;	
	}
	if (StartPage(m_hDC) > 0) 
	{
		m_pageOpen = true;	
		SetMapMode(m_hDC, MM_TWIPS);
		SetBkMode(m_hDC, TRANSPARENT);
		m_hFont = CreateFont(m_fontSize,	// height
				       0,	// width
				       0,	// escapement
				       0,	// orientation
			       FW_NORMAL,	// weight
				       0,	// italic
                                       0, 	// underline
                                       0,       // strikeout
                            ANSI_CHARSET,	// charset
		      OUT_DEFAULT_PRECIS,	// Default precision
		     CLIP_DEFAULT_PRECIS,	// Default clipping
			 DEFAULT_QUALITY,	// Quality
			     FIXED_PITCH,	// Pitch
			m_fontFace.c_str());	// Face name
		m_hFontOld  = (HFONT)SelectObject(m_hDC, m_hFont);
		m_hPenOld   = (HPEN)SelectObject(m_hDC, m_hPen);
		m_hBrushOld = (HBRUSH)SelectObject(m_hDC, m_hBrush);
		return true;
	}	
	return false;
}


bool GdiPrint::endPage()
{
	if (!m_hDC) return false;
	SelectObject(m_hDC, m_hPenOld);
	SelectObject(m_hDC, m_hBrushOld);
	SelectObject(m_hDC, m_hFontOld);
	DeleteObject(m_hFont);
	EndPage(m_hDC);
	m_pageOpen = false;	
	return true;
}


void GdiPrint::putDot(float x, float y)
{
	long lx, ly;
	/* x,y are coordinates using 360dpi units. Multiply by 4 to get twips
	 * units. */
	lx = (long)((x * 4.0) + 0.499999);	
	ly = (long)((y * 4.0) + 0.499999);	
	ly = -ly;

	Ellipse(m_hDC, lx - 8, ly - 8, lx + 8, ly + 8);
}


void GdiPrint::putChar(float x, float y, char c)
{
	long lx, ly;
	lx = (long)((x * 4.0) + 0.499999);	
	ly = (long)((y * 4.0) + 0.499999);	
	TextOut(m_hDC, lx, -ly, &c, 1);
}


bool GdiPrint::open()
{
	DOCINFO di;

	XLOG("GdiPrint::open()");
	m_hDC = CreateDC(m_sDriver.c_str(), m_sDevice.c_str(), NULL, m_pDevmode);
	if (!m_hDC) return false;
	XLOG("GdiPrint::CreateDC() worked");

	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.lpszDocName = "JOYCE output";
	
	if (StartDoc(m_hDC, &di) > 0) m_docOpen = true; 

	return true;
}


void GdiPrint::close()
{
	XLOG("GdiPrint::close()");
	if (!m_hDC) return;

	if (m_pageOpen) { EndPage(m_hDC); m_pageOpen = false; }
	if (m_docOpen)  { EndDoc(m_hDC);  m_docOpen  = false; }
	XLOG("Page & doc ended");
	DeleteDC(m_hDC);
	m_hDC = NULL;
	XLOG("DC closed");
}

#endif
