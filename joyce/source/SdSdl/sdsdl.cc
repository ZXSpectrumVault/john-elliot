/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"


void SdSdl::clear(void)
{
        SDL_Rect rect;
        Uint32 pixel;

        rect.x = rect.y  =0;
        rect.w = m_globals->m_xres + 1;
        rect.h = m_globals->m_yres + 1;
        pixel = m_globals->MAP_COL(0); // SDL_MapRGB(m_globals->m_surface->format, 255, 255, 255);
        if (lock_surface()) return;
        SDL_FillRect(m_globals->m_surface, &rect, pixel);
        unlock_surface();
	flush();
}


void SdSdl::flush(void)
{
        SDL_UpdateRect(m_globals->m_surface, 0, 0, m_globals->m_xres + 1,
			m_globals->m_yres + 1);
}




void SdSdl::contourFill(long fill, const GPoint *xy)
{
	/* Not implemented even in real GEM */
}


int SdSdl::getPixel(const GPoint *xy, long *pel, long *index)
{
	/* Not implemented even in real GEM */
	return FALSE;
}
