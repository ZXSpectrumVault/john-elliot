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


bool SdSdl::MONO8XHT(const wchar_t *str, bool justified)
{
	if (justified) return false;

//	if (m_DEST.x & 7) return 0;	// No point

	if (m_CLIP)
	{
		if (m_DEST.x          <  m_XMN_CLIP) return 0;
		if (m_DEST.y          <  m_YMN_CLIP) return 0;
		if (m_DEST.y + m_FONT_INF.form_height >= (Uint32)m_YMX_CLIP) return 0;
		if (m_DEST.x + 8 * wcslen(str) >= (Uint32)m_XMN_CLIP) return 0;
	}
	// Clip at edge of screen
	if (m_DEST.x < 0) return 0;
	if (m_DEST.y < 0) return 0;
	if (m_DEST.y + m_FONT_INF.form_height >= xres) return 0;
	if (m_DEST.x + 8 * wcslen(str) >= xres) return 0;
	set_fnthdr(*str);
	Uint8 *bits = concat(m_DEST.x, m_DEST.y);

	switch(m_WRT_MODE)
	{	
		case 0: MONO8XHT_REPLACE(bits, str); break;
		case 2: MONO8XHT_XOR(bits, str);     break;
		case 1:
		case 3: MONO8XHT_TRAN(bits, str);    break;
	}
	update_rect(m_DEST.x, m_DEST.y, m_DEST.x + 8 * wcslen(str),
			m_DEST.y + m_FONT_INF.form_height);
	return true;
}

Uint8 *SdSdl::sdl_repl_byte(Uint8 *bits, Uint8 byte, Uint32 fg)
{
	Uint8 mask = 0x80;

//	printf("%02x: ", byte);
	for (int n = 0; n < 8; n++) 
	{
//		printf("%c", (byte & mask) ? '*' : ' ');
		if (byte & mask) sdl_plot(bits, fg);
		else		 sdl_plot(bits, m_bg_pixel);
		mask >>= 1;
		bits += m_sdl_bpp;
	}
//	printf("\n");
	return bits;
}

Uint8 *SdSdl::sdl_tran_byte(Uint8 *bits, Uint8 byte, Uint32 fg)
{
        Uint8 mask = 0x80;

        for (int n = 0; n < 8; n++)
        {
                if (byte & mask) sdl_plot(bits, fg);
                mask >>= 1;
		bits += m_sdl_bpp;
        }
	return bits;
}


Uint8 *SdSdl::sdl_xor_byte(Uint8 *bits, Uint8 byte, Uint32 fg)
{
        Uint8 mask = 0x80;

        for (int n = 0; n < 8; n++) 
        {
                if (byte & mask) sdl_xor(bits, fg);
                mask >>= 1;
		bits += m_sdl_bpp;
        }
	return bits;
}

	
/*******************************
  MONO8XHT_REPLACE
		ES:DI = SCREEN POINTER

*******************************/

void SdSdl::MONO8XHT_REPLACE(Uint8 *bits, const wchar_t *str)
{
	if (lock_surface()) return;

	Uint32 fw = m_FONT_INF.form_width;
	Uint32 fh = m_FONT_INF.form_height;	

	while(*str)
	{	
		wchar_t ch = *str++;
		Uint8 *data = m_FONT_INF.dat_table + ch;

		for (Uint32 n = 0; n < fh; n++)
			sdl_repl_byte(bits + n * m_sdl_pitch, data[n*fw], m_text_pixel);
		bits += 8 * m_sdl_bpp;
	}
	unlock_surface();
}


void SdSdl::MONO8XHT_TRAN(Uint8 *bits, const wchar_t *str)
{
        if (lock_surface()) return;

        Uint32 fw = m_FONT_INF.form_width;
        Uint32 fh = m_FONT_INF.form_height;
	Uint32 pixel = m_text_pixel;
	if (m_WRT_MODE == 3) pixel = m_bg_pixel;

        while(*str)
        {
                wchar_t ch = *str++;
                Uint8 *data = m_FONT_INF.dat_table + ch;

                for (Uint32 n = 0; n < fh; n++)
                        sdl_tran_byte(bits + n * m_sdl_pitch, data[n*fw], pixel);
                bits += 8 * m_sdl_bpp;
        }
	unlock_surface();
}


void SdSdl::MONO8XHT_XOR(Uint8 *bits, const wchar_t *str)
{
        if (lock_surface()) return;

        Uint32 fw = m_FONT_INF.form_width;
        Uint32 fh = m_FONT_INF.form_height;

        while(*str)
        {
                wchar_t ch = *str++;
                Uint8 *data = m_FONT_INF.dat_table + ch;

                for (Uint32 n = 0; n < fh; n++)
                        sdl_xor_byte(bits + n * m_sdl_pitch, data[n*fw], m_text_pixel);
                bits += 8 * m_sdl_bpp;
        }
	unlock_surface();
}

