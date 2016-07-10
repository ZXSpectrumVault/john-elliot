/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2005  John Elliott <seasip.webmaster@gmail.com>

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
#include "sdsdl.hxx"

#define nPTSIN  1	/* Indices into the control array for sizes of */
#define nPTSOUT 2	/* the other arrays */
#define nINTIN  3
#define nINTOUT 4
#define nESC    5
extern int tr;

PcwGSX::PcwGSX(PcwSystem *s) : PcwSdlTerm("gsx", s)
{
	m_sys = s;
	oldtype = -1;
	m_term = NULL;
	m_sdsdl = NULL;
	m_height = 0;
	m_locatorMode = 1;
}

PcwGSX::~PcwGSX()
{
	if (m_sdsdl) delete m_sdsdl;
	if (m_term) delete m_term;
	m_term = NULL;
}





GPoint *PcwGSX::getPtsin()
{
	int n;
	int max = 1 + getArray(ctrl, nPTSIN); 
	GPoint *pt = new GPoint[max];

	for (n = 0; n < max; n++)
	{
		pt[n].x = getArray(ptsin, n * 2);
		pt[n].y = m_height - getArray(ptsin, n * 2 + 1);
	}
	return pt;
}



void PcwGSX::getColour(Uint32 *col, word array, word off)
{
	Uint8 r, g, b;
	word nc = getArray(array, off);
	if (nc >= 128) nc = 127;

	m_sysVideo->indexToColour(nc, &r, &g, &b);

	*col = r;
	*col = (*col << 8) | g;
	*col = (*col << 8) | b;
	*col = (*col << 8) | 0xff;	/* Alpha */
}



void PcwGSX::setColour(Uint32 *ptr)
{
	int col = getWord(intin);
	getColour(ptr, intin, 0);
	m_sys->m_cpu->storeW(intout, (col > 127) ? 127 : col);
	putArray(ctrl, nINTOUT, 1);	
}





/********************************************************************/



void PcwGSX::openWorkstation(void)
{
	int n;
	GWSParams param;
	GWSResult result;
	GPoint pt[4];
	long styles[8];

	for (n = 0; n < 9; n++)
	{
		param.defaults[n] = getArray(intin, n);
		param.xymode = 1;	/* We are scaling */
	}
        m_oldTerm = m_sys->m_activeTerminal;
	m_sys->selectTerminal(this);

	m_term = new SdlTerm(m_sysVideo->getSurface());
	SDL_Rect &rcTerm = m_term->getBounds();

	/* GSX likes a black background */
	m_term->setBackground(0,0,0);
	m_term->enableCursor(false);
	m_sdsdl = new SdSdl();
	if (!m_sdsdl->open(&param, &result, m_term->getSurface()))
	{
		/* Should never happen */
		return;
	}
	putArray(ctrl, nPTSOUT, 6);
	putArray(ctrl, nINTOUT, 45);

	/* GSX drivers use raster coordinates that have 0 at the bottom
	 * and work up. GEM drivers start at the top and work down. So store
	 * the screen height, and do the conversion here. */
	m_height = result.data[1] - 1;
	/* Similarly, text coordinates need adjusting by the height of the 
	 * font. */
	m_sdsdl->queryText(styles, pt);	// Get text height
	m_txtheight = pt[1].y;		// And save it.

	m_mintxtheight = result.points[0].y + 1;
	for (n = 0; n < 45; n++) 
	{
		putArray(intout, n, result.data[n]);
	}
	// Since SDSDL doesn't yet support multiple fonts, present GEM's 
	// text effects as fonts.
	putArray(intout, 10, 16);	
	for (n = 0; n < 6; n++)
	{
		putArray(ptsout, n*2,   result.points[n].x);
		putArray(ptsout, n*2+1, result.points[n].y);
	}
	// Set palette to white on black (GSX) rather than black on white (GEM)
	m_sdsdl->setColor(0, 0, 0, 0);
	m_sdsdl->setColor(1, 1000, 1000, 1000);
	/* Do this after we've rearranged the palette */
	m_term->setBackground(0, 0, 0);
	m_term->setForeground(0xFF, 0xFF, 0xFF);
	clearPicture();
	m_lastX = -1; m_lastY = -1;
}


void PcwGSX::closeWorkstation()
{
	m_sdsdl->close();
	delete m_sdsdl;
	m_sdsdl = NULL;
	m_term->enableCursor(true);
	m_sys->selectTerminal(m_oldTerm);
	delete m_term;
	m_term = NULL;
}


void PcwGSX::clearPicture()
{
	m_sdsdl->clear();
}

void PcwGSX::updateWorkstation()
{
	m_sdsdl->flush();
}



void PcwGSX::escape(void)
{
	int x,y,txtc;
	GPoint pt;
//
// Rather than use SdSdl for (at the moment) nonexistent escape support, 
// implement them here using the underlying terminal emulator.
//
	switch(getArray(ctrl, nESC))
	{
		case 1:	/* Text screen size in cells */
		putArray(ctrl, nINTOUT, 2);
		putArray(intout, 0, 37);	
		putArray(intout, 1, 100);
		break;

		case 2: /* Enter gfx mode */
//		m_sdsdl->leaveCur();
		m_term->enableCursor(false);
		m_sdsdl->clear();
		break;

		case 3: /* Exit gfx mode */
//		m_sdsdl->enterCur();
		m_sdsdl->clear();
		m_term->enableCursor(true);
		break;

		case 4: /* Cursor up */ m_term->cursorUp(false); break;
		case 5: /* Cursor dn */ m_term->cursorDown(false); break;
		case 6: /* Cursor lt */ m_term->cursorLeft(false); break;
		case 7: /* Cursor rt */ m_term->cursorRight(false); break;
		case 8: /* Cursor hm */ m_term->home(); break;
		case 9: /* Erase EOS */ m_term->clearEOS(); break;
		case 10: /* Erase EOL */ m_term->clearEOL(); break;

		case 11: /* Position cursor */
		m_term->moveCursor(getArray(intin, 1),getArray(intin, 0));
		break;

		case 12: /* Text out */
		for (txtc = 0; txtc < getArray(ctrl, nINTIN); txtc++)
		{
			(*m_term) << ((unsigned char)(getArray(intin, txtc)));
		}
		break;

		case 13: /* Rev vid   */ m_term->setReverse(1); break;
		case 14: /* True vid  */ m_term->setReverse(0); break;
		
		case 15: /* Get cursor position */
		m_term->getCursorPos(&x, &y);
		putArray(ctrl, nINTOUT, 2);
		putArray(intout, 0, y);
		putArray(intout, 1, x);
		break;

		case 16:		/* Yes, we have a pointing device */
		putArray(ctrl, nINTOUT, 1);
		putArray(intout, 0, 1);
		break;
		
		case 17:		/* No hardcopy */
		break;

		case 18:		/* Show pointer */
		pt.x = getArray(ptsin, 0);
		pt.y = getArray(ptsin, 1);
		SDL_WarpMouse(pt.x, m_height - pt.y);
		SDL_ShowCursor(1);
//		m_sdsdl->escape(18, 1, &pt, 0, NULL, NULL, NULL, NULL, NULL, NULL);
		break;

		case 19:		/* Hide pointer */
		SDL_ShowCursor(0);
//		m_sdsdl->escape(19, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
		break;
	}

}



void PcwGSX::drawPolyline(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->polyLine(getArray(ctrl, nPTSIN), pt);
	delete pt;
}


void PcwGSX::drawPolymarker(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->polyMarker(getArray(ctrl, nPTSIN), pt);
	delete pt;
}

void PcwGSX::fillArea(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->fillArea(getArray(ctrl, nPTSIN), pt);
	delete pt;
}

void PcwGSX::graphicText(void)
{
	GPoint pt;
	int txtlen = getArray(ctrl, nINTIN);
	wchar_t *str = new wchar_t[1 + txtlen];
	int n;
	
	for (n = 0; n < txtlen; n++)
	{
		str[n] = getArray(intin, n);
	}
	str[n] = 0;
	pt.x = getArray(ptsin, 0);
	pt.y = (m_height - getArray(ptsin, 1));
	m_sdsdl->graphicText(&pt, str);
	delete str;
}


#if 0


static void OutputIndexArray(void)
{
	int x , y, rx, ry, c;
	int rowlen  = getArray(ctrl, 5);
	int rowused = getArray(ctrl, 6);
	int nrows   = getArray(ctrl, 7);
	int wmode   = getArray(ctrl, 8);
	
	switch(wmode)
	{
		case 0: wmode = 0;     break;
		case 1: wmode = GrOR;  break;
		case 2: wmode = GrXOR; break;
		case 3: wmode = GrAND; break;
	}

	getptsin();
	ry = 0;
	for (y = Y0; y < Y1; y++)
	{
		rx  = 0;
		for (x = X0; x < X1; x++)
		{
			GetColour(&c, intin, ry * rowlen + rx);
			if (wmode == GrAND) c = gsxbg; else c |= wmode;
			GrPlot(x, y, c);
			++rx; if (rx >= rowused) rx = 0;
		}
		++ry; if (ry >= nrows) ry = 0;
	}
}


#endif

void PcwGSX::gdp()
{
	int npts = getArray(ctrl, nPTSIN);
	int nint = getArray(ctrl, nINTIN);
	GPoint *pt = new GPoint[1 + npts];
	long *iv = new long[1 + nint];
	wchar_t *str = new wchar_t[1 + nint];
	int n;
	for (n = 0; n < npts; n++)
	{
		pt[n].x = getArray(ptsin, n*2);
		pt[n].y = m_height - getArray(ptsin, n*2+1);
	}
	for (n = 0; n < nint; n++)
	{
		iv[n] = str[n] = getArray(intin, n);
	}
	str[n] = 0;
	m_sdsdl->gdp(getArray(ctrl, nESC), npts, pt, nint, iv, str);
	delete str;
	delete iv;
	delete pt;
}


void PcwGSX::setTextDirection(void)
{
	m_sdsdl->stRotation((short)getArray(intin, 0));
}


void PcwGSX::setColourIndex(void)
{
	int red   = getArray(intin, 1);
	int green = getArray(intin, 2);
	int blue  = getArray(intin, 3);
	int ncol  = getArray(intin, 0);

	m_sdsdl->setColor(ncol, red, green, blue);
}

void PcwGSX::setLineStyle(void)
{
	long style = m_sdsdl->slType(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::setMarkerStyle(void)
{
	long style = m_sdsdl->smType(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setTextStyle(void)
{
	// Since SDSDL doesn't yet support multiple fonts, present GEM's 
	// text effects as fonts.
	long style = m_sdsdl->stStyle(getWord(intin) - 1);
	m_sys->m_cpu->storeW(intout, 1 + style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::setTextColour(void)
{
	long style = m_sdsdl->stColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setMarkerColour(void)
{
	long style = m_sdsdl->smColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setLineColour(void)
{
	long style = m_sdsdl->slColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setLineWidth(void)
{
	GPoint pt[2];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	m_sdsdl->slWidth(&pt[0], &pt[1]);
	m_txtheight = pt[1].y;
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ctrl, nPTSOUT, 1);
}

void PcwGSX::setTextHeight(void)
{
	GPoint pt[3];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	if (pt[0].y < m_mintxtheight) 	// Don't let it get unreadably small
	{
		pt[0].y = m_mintxtheight;
	}
	m_sdsdl->stHeight(&pt[0], &pt[1]);
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ptsout, 2, pt[2].x);
	putArray(ptsout, 3, pt[2].y);
	putArray(ctrl, nPTSOUT, 2);
}

void PcwGSX::setMarkerHeight(void)
{
	GPoint pt[2];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	m_sdsdl->smHeight(&pt[0], &pt[1]);
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ctrl, nPTSOUT, 1);
}

void PcwGSX::setFillStyle(void)
{
	long style = m_sdsdl->sfStyle(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setFillColour(void)
{
	long style = m_sdsdl->sfColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setFillIndex(void)
{
	long style = m_sdsdl->sfIndex(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::inquireColour(void)
{
	long colour = getArray(intin, 0);
	short mode  = getArray(intin, 1);
	short r, g, b, v;

	v = m_sdsdl->qColor(colour, mode, &r, &g, &b);

	putArray(intout, 0, v);
	putArray(intout, 1, r);
	putArray(intout, 2, g);
	putArray(intout, 3, b);
	putArray(ctrl, nINTOUT, 4);
}


/*
static void InquireCellArray(void)
{
	int x , y, rx, ry, c;
	short c1;
	int rowlen  = getArray(ctrl, 5);
	int nrows   = getArray(ctrl, 6);
	int rowused = 0;         
	int nrused  = 0;
	int error   = 0;
	
	getptsin();
	ry = 0;
	for (y = Y0; y < Y1; y++)
	{
		rx  = 0;
		for (x = X0; x < X1; x++)
		{
			c = GrPixel(x, y);
			for (c1 = 0; c1 < 128; c1++)
				if (m_colourMap[c1] == c) break;
			if (c1 == 128) 
			{	
				c1 = (-1);
				error = 1;
			}
			putArray(intin, ry * rowlen + rx, c1);
			++rx; if (rx >= nrused) nrused = rx;
		}
		++ry; if (ry >= nrused) nrused = ry;
		if (nrused > nrows) break;
	}
	putArray(ctrl, 7, rowused);
	putArray(ctrl, 8, nrused);
	putArray(ctrl, 9, error);
}

*/



void PcwGSX::setWriteMode(void)
{
	int mode = getArray(intin, 0);
	putArray(ctrl, nINTOUT, 1);
	putArray(intout, 0, m_sdsdl->swrMode(mode));
}


void PcwGSX::readLocator(void)
{
// Don't try to integrate SDSDL's event loop with our own; instead, 
// handle the locator (the mouse) here.
//
	long dev;
	GPoint pt[2]; 
	short termch;
	Uint8 b;
	int x, y, state;
	SDL_Event ev;

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	dev = getArray(intin, 0);
	termch = 0;
//	m_sdsdl->qLocator(dev, &pt[0], &pt[1], &termch);
	switch(m_locatorMode)
	{
		case 1: // Modal
			state = SDL_ShowCursor(-1);
			SDL_ShowCursor(1);
			while (!termch)
			{
				if (!SDL_WaitEvent(&ev)) break;

				switch(ev.type)
				{
					case SDL_QUIT: goodbye(99);
					case SDL_MOUSEBUTTONDOWN:
						pt[1].x = ev.button.x;
						pt[1].y = ev.button.y;
						switch(ev.button.button)
						{
							case SDL_BUTTON_LEFT:
							termch = 0x20;
							break;
							case SDL_BUTTON_MIDDLE:
							termch = 0x22;
							break;
							case SDL_BUTTON_RIGHT:
							termch = 0x21;
							break;
						}
						break;
				}
			}	
			SDL_ShowCursor(state);
			// Modal always returns coords and buttons
			putArray(ctrl, nINTOUT, 1);
			putArray(intout, 0, termch);
			putArray(ctrl, nPTSOUT, 1);
			putArray(ptsout, 0, pt[1].x);
			putArray(ptsout, 1, m_height - pt[1].y);
			break;
		case 2: // Modeless. Return the current state of the mouse
			// as recorded by the main JOYCE event loop.
			b = m_sys->getMouseBtn();
			pt[1].x = m_sys->getMouseX();
			pt[1].y = m_sys->getMouseY();
			if (b & SDL_BUTTON_LMASK) termch = 0x20;
			else if (b & SDL_BUTTON_MMASK) termch = 0x22;
			else if (b & SDL_BUTTON_RMASK) termch = 0x21;
	/* If mouse has not moved, don't return a point */ 
			if (m_lastX == pt[1].x && m_lastY == pt[1].y)
			{
				putArray(intout, 0, termch);
				if (termch)
				{
					putArray(ctrl, nINTOUT, 1);
				}
				else
				{
					putArray(ctrl, nINTOUT, 0);
				}
				putArray(ctrl, nPTSOUT, 0);
			}
			else /* Return point but no button */
			{
				putArray(ctrl, nPTSOUT, 1);
				putArray(ptsout, 0, pt[1].x);
				putArray(ptsout, 1, m_height - pt[1].y);
			}
			m_lastX = pt[1].x;
			m_lastY = pt[1].y;
			break;
	}

}




void PcwGSX::setInputMode()
{
	short devtype = getArray(intin, 0);
	short mode = getArray(intin, 1);

	if (mode != 2) mode = 1;
	switch(devtype)
	{
		case 1: m_locatorMode = mode; break;
	}
// Don't try to integrate SDSDL's event loop with our own; instead, 
// handle the locator (the mouse) here.
//
//	short mode = m_sdsdl->sinMode(getArray(intin,0), getArray(intin,1));
	putArray(intout, 0, mode);
	putArray(ctrl, nINTOUT, 1);
}




void PcwGSX::command(word pb)
{
	int j;
	word function;

	m_surface = m_sys->m_activeTerminal->getSysVideo()->getSurface();
	gsxpb  = pb;
	ctrl   = getWord(pb);
	intin  = getWord(pb + 2);
	ptsin  = getWord(pb + 4);
	intout = getWord(pb + 6);
	ptsout = getWord(pb + 8);	

	putArray(ctrl, nINTOUT, 0);
	putArray(ctrl, nPTSOUT, 0);

	function = getWord(ctrl);
	if (function != 1 && m_sdsdl == NULL) return;
/* Uncomment this to get a log of GSX calls made */
/* 
	FILE *fp = fopen("pcwgsx.log", "a");
	if (function == 5 || function == 11)
	{
		fprintf(fp, "fn: %d/%d  intin: ", function,
				getArray(ctrl, 5));
	}
	else
	{
		fprintf(fp, "fn: %d  intin: ", function);
	}
	for (j = 0; j < getArray(ctrl, nINTIN) + 1; j++) 
		fprintf(fp, "%d;", getArray(intin, j));
	fprintf(fp, "\n  ptsin: ");
	for (j = 0; j < 2*getArray(ctrl, nPTSIN); j++) 
		fprintf(fp, "%d;", getArray(ptsin, j));
	fprintf(fp, "\n");
	fflush(fp);
*/
	switch(function)
	{
		case 1: openWorkstation();  break;
		case 2: closeWorkstation(); break;
		case 3: clearPicture();	    break;
		case 4:	updateWorkstation();break;
		case 5: escape();	    break;
		case 6: drawPolyline();	    break;
		case 7: drawPolymarker();   break;
		case 8: graphicText();	    break;
		case 9: fillArea();	    break;
//		case 10: OutputIndexArray();break;
		case 11: gdp();		    break;
		case 12: setTextHeight();   break;
		case 13: setTextDirection();break;
		case 14: setColourIndex();  break;
		case 15: setLineStyle();    break;
		case 16: setLineWidth();    break;
		case 17: setLineColour();   break;
		case 18: setMarkerStyle();  break;
		case 19: setMarkerHeight(); break;
		case 20: setMarkerColour(); break;
		case 21: setTextStyle();    break;
		case 22: setTextColour();   break;
		case 23: setFillStyle();    break;
		case 24: setFillIndex();    break;
		case 25: setFillColour();   break;
		case 26: inquireColour();   break;
//		case 27: InquireCellArray();break;
		case 28: readLocator();	    break;
		case 32: setWriteMode();    break;
		case 33: setInputMode();    break;
		default:		    break;		
	}
/* 
	fprintf(fp, "  intout: ");
	for (j = 0; j < getArray(ctrl, nINTOUT); j++) 
		fprintf(fp, "%d;", getArray(intout, j));
	fprintf(fp, "\n  ptsout: ");
	for (j = 0; j < 2*getArray(ctrl, nPTSOUT); j++) 
		fprintf(fp, "%d;", getArray(ptsout, j));
	fprintf(fp, "\n");
	fclose(fp);
*/
}

word PcwGSX::getWord(word addr)
{
	return m_sys->m_cpu->fetchW(addr);
}

word PcwGSX::getArray(word addr, word idx)
{
	return m_sys->m_cpu->fetchW(addr + 2 * idx);
}

void PcwGSX::putArray(word addr, word idx, word value)
{
	m_sys->m_cpu->storeW(addr + 2 * idx, value);
}

bool PcwGSX::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

bool PcwGSX::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

PcwTerminal& PcwGSX::operator<<(unsigned char c)
{
	return *this;
}


void PcwGSX::setForeground(byte r, byte g, byte b)
{
	if (m_term) m_term->setForeground(r,g,b);
}

void PcwGSX::setBackground(byte r, byte g, byte b)
{
	if (m_term) m_term->setBackground(r,g,b);
}

				        


