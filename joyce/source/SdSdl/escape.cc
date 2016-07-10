/*************************************************************************
**	 Copyright 2005, John Elliott                                   **
**       Copyright 1999, Caldera Thin Clients, Inc.                     ** 
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                ** 
**                                                                      ** 
**                  Historical Copyright                                ** 
**                                                                      **
**                                                                      **
**                                                                      **
**  Copyright (c) 1987, Digital Research, Inc. All Rights Reserved.     **
**  The Software Code contained in this listing is proprietary to       **
**  Digital Research, Inc., Monterey, California and is covered by U.S. **
**  and other copyright protection.  Unauthorized copying, adaptation,  **
**  distribution, use or display is prohibited and may be subject to    **
**  civil and criminal penalties.  Disclosure to others is prohibited.  **
**  For the terms and conditions of software code use refer to the      **
**  appropriate Digital Research License Agreement.                     **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"

void SdSdl::escape(long type, long pcount, const GPoint *pt,
	           long icount, const long *ival, wchar_t *text,
		   long *pcout, GPoint *ptout,
		   long *icout, long *intout)
{
	switch(type)
	{
		case 1: *icout = 2;
			intout[1] = m_globals->m_requestH / 16;
			intout[0] = m_globals->m_requestW / 8;
			break;

		// XXX Implement escapes 2,3,...
		case 16:	// Check for mouse
			*icout = 1;
			*intout = 1;
			break;
		case 18:
			if (m_HIDE_CNT > 0)
			{
				m_HIDE_CNT = 1;
				DIS_CUR();
				SDL_WarpMouse(pt[0].x, pt[0].y);
			}
			break;
		case 19:
			HIDE_CUR();
			break;
	}
}

