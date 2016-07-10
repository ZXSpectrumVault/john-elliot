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

typedef bool (SdSdl::*PREBLIT) (Sint32 *x, Sint32 *width, Sint32 offset,
		Sint32 *srcx, Uint8 **bits);
typedef void (SdSdl::*POSTBLIT)(Sint32 *x, Sint32 offset);

void SdSdl::txtblt_rep(Uint32 char_wp, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask)
{
	while (char_wp)
	{
		if ((*font_bits) & font_mask) 
		{
			sdl_plot(bits, m_text_pixel);
		}
		else 
		{
			sdl_plot(bits, m_bg_pixel);
		}
		bits += m_sdl_bpp;
		font_mask >>= 1; 
		if (!font_mask) { font_mask = 0x80; ++font_bits; }
		--char_wp;
	}
}

void SdSdl::txtblt_tran(Uint32 char_wp, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask, Uint32 pixel)
{
        while (char_wp)
        {
                if ((*font_bits) & font_mask) sdl_plot(bits, pixel);
                bits += m_sdl_bpp;
                font_mask >>= 1;
                if (!font_mask) { font_mask = 0x80; ++font_bits; }
                --char_wp;
        }
}

void SdSdl::txtblt_xor(Uint32 char_wp, Uint8 *bits, Uint8 *font_bits, Uint8 font_mask)
{
        while (char_wp)
        {
                if ((*font_bits) & font_mask) sdl_xor(bits, m_text_pixel);
                bits += m_sdl_bpp;
                font_mask >>= 1;
                if (!font_mask) { font_mask = 0x80; ++font_bits; }
                --char_wp;
        }
}

POSTBLIT post_blt_table[] = { &SdSdl::post_blt_0,   &SdSdl::post_blt_90, 
			      &SdSdl::post_blt_180, &SdSdl::post_blt_270 };

PREBLIT pre_blt_table[] = { &SdSdl::pre_blt_0, &SdSdl::pre_blt_90,
			    &SdSdl::pre_blt_180, &SdSdl::pre_blt_270 };
/*
line_table	dw	clr_skew_next_line_0
		dw	clr_skew_next_line_90
		dw	clr_skew_next_line_180
		dw	clr_skew_next_line_270

pixel_table	dw	clr_skew_next_pixel_0
		dw	clr_skew_next_pixel_90
		dw	clr_skew_next_pixel_180
		dw	clr_skew_next_pixel_270
*/

/****************************************************************
 * TEXT_BLT							*
 ****************************************************************/

void SdSdl::TEXT_BLT(const wchar_t *str, bool justified)
{
	Uint32 update_x, update_y, update_x1, update_y1;
	Sint32 right_x;
	int mode = m_WRT_MODE;
	
	update_x = update_x1 = m_DEST.x;
	update_y = update_y1 = m_DEST.y;
/*
 * If skewing and the writing mode is replace or reverse transparent, force the
 * mode to transparent.  In the latter case, substitute background color for
 * the foreground.*/
	if (m_SPECIAL & SKEW) switch(mode)
	{
		case 3:	m_text_pixel = m_globals->MAP_COL(0);	// FALL THROUGH
		case 0: mode = 1;
	}
	m_text_mode = mode;
	m_text_ix   = m_text_pixel;

	if (m_SPECIAL)
	{	
		text_blt_special(str, justified);
		return;
	}

// Clip in the y direction.  First, compare the cell against the clipping
// window bottom.
 	m_top_overhang = 0;
	Uint32 offset = 0;
	Sint32 dely   = m_ACTDELY;
	Sint32 top    = m_DEST.y;
	Sint32 bottom = m_DEST.y + dely - 1;
	Uint32 char_wp;

	if (bottom > m_YMX_CLIP)	// The text goes outside clip rect.
	{
		if (top > m_YMX_CLIP) return;	// completely outside	
		dely   = (m_YMX_CLIP - top) + 1;
		bottom = m_YMX_CLIP;
	}
// At least part of the cell is above the bottom of the clipping rectangle.
// Check against the top of the clipping rectangle.
	if (top < m_YMN_CLIP)
	{
		if (bottom < m_YMN_CLIP) return;	// Totally above
		Uint32 newdely = bottom - m_YMN_CLIP;	// New visible height
		top = m_YMN_CLIP;			// New top
		m_top_overhang = dely - newdely;	// Bit that's lost
		dely = newdely;
// Calcualte the new offset into the font
		offset = m_FONT_INF.form_width * m_top_overhang;
	} 
	chk_fnt();
	m_font_off = offset + m_FONT_INF.dat_table;
	m_tempdely = dely;
	m_DEST.y   = top;
// Prepare for left clipping.
 	Uint32 char_x = m_DEST.x;
	Uint32 clip_x = m_XMN_CLIP;

// Clip the string on the left side.
	
	while (*str)
	{
		wchar_t ch = set_fnthdr(*str++);
		m_font_off =  m_FONT_INF.dat_table + (m_top_overhang * m_FONT_INF.form_width);

		m_isspace = ch;
		wchar_t glyph = (ch - m_FONT_INF.first_ade);
		Uint32 *poffset = m_FONT_INF.off_table + glyph;
		Uint32 char_offset = poffset[0];
// 11/6/86 DH
// added horizontal offset code here

		if (m_FONT_INF.flags & HORZ_OFF) 
		{
			m_lt_hoff = m_FONT_INF.hor_table[2 * glyph];
			m_rt_hoff = m_FONT_INF.hor_table[2 * glyph + 1];

			char_x       -= m_lt_hoff;
		}
		char_wp = poffset[1] - poffset[0];
		if (char_x < clip_x)	// text_no_lxclip
		{
			right_x = char_x + char_wp - 1;
			if (right_x < (Sint32)clip_x)	// text_clip_leftx_found
			{
				if (justified)	// text_clip_leftx_notjust
				{
					if (m_isspace == ' ')
					{
						char_wp += m_JUST_DEL_WORD;
						if (m_JUST_DEL_WORD_REM)
						{
							char_wp += m_JUST_DEL_WORD_SIGN;
							--m_JUST_DEL_WORD_REM;
						}
					} 
					else
					{
						char_wp += m_JUST_DEL_CHAR;
						if (m_JUST_DEL_CHAR_REM)
						{
       	        		                         char_wp += m_JUST_DEL_CHAR_SIGN;
		                                        --m_JUST_DEL_CHAR_REM;
						}
					}
				}
				char_x += char_wp;
			        if (m_FONT_INF.flags & HORZ_OFF)
					char_x -= m_rt_hoff;
				continue;
			}
			char_offset += (char_wp - (right_x + 1 - clip_x));
			char_x = clip_x;
			char_wp = (right_x + 1 - clip_x);
			
		}
/* Ready to draw the first character.  The status of the registers is as
 * follows:
 *      char_wp      = width of the visible part of the first character
 *      char_x      = starting position (destination) of the first character
 *      char_offset = first character's font address (clip adjusted)
 */
// text_no_lxclip:
	
	    Uint8 *bits  = concat(char_x, m_DEST.y);
// Top of the character output loop.  The registers are set up as follows:
//	char_offset = character font address (clip adjusted)
//	char_wp      = character width
//	char_x      = destination start x
	    right_x = char_x + char_wp - 1;
	    if (right_x > m_XMX_CLIP)
	    {
		if ((Sint32)char_x > m_XMX_CLIP) return;
		char_wp = (m_XMX_CLIP - char_x + 1);
	    }
// Take care of fudging for justified text.
	    if (justified)
	    {
		if (m_isspace == ' ')
		{
			right_x += m_JUST_DEL_WORD;	
			if (m_JUST_DEL_WORD_REM)
			{ 
				right_x += m_JUST_DEL_WORD_SIGN;
				--m_JUST_DEL_WORD_REM;
			}	
		}
		else
		{
			right_x += m_JUST_DEL_CHAR;
			if (m_JUST_DEL_CHAR_REM)
			{
				right_x += m_JUST_DEL_CHAR_SIGN;
				--m_JUST_DEL_CHAR_REM;
			}
		}
	    }
// Because SDL doesn't have a multiple-plane memory model, I'm going to
// skip the "right and left masks" bit.
//
// What we have to do is plot "tempdely" lines of "char_wp" pixels, which
// are in the font at line m_font_off, pixel char_offset 

	    for(unsigned y = 0; y < m_tempdely; y++)
	    { 
		Uint8 *font_bits = m_font_off + (y * m_FONT_INF.form_width) + 
				(char_offset >> 3); 
		Uint8 font_mask = 0x80 >> (char_offset & 7);

		switch(m_text_mode)
		{
			case 0: txtblt_rep(char_wp, bits, font_bits, font_mask); break;
			case 1: txtblt_tran(char_wp, bits, font_bits, font_mask, m_text_ix); break;
			case 2: txtblt_xor(char_wp, bits, font_bits, font_mask); break;
			case 3: txtblt_tran(char_wp, bits, font_bits, font_mask, m_bg_pixel); break;
		}
		bits += m_sdl_pitch;
	    }
// Done with the character.  Prepare for the next one.
 	    char_x = right_x;
	    update_x1 = right_x;
	    update_y1 = update_y + m_tempdely;
	    if (m_FONT_INF.flags & HORZ_OFF) char_x -= m_rt_hoff;
	}
	// All of the characters which can be output have been written.  All that is
	// left is to underline them, if requested.
	text_blt_done();
	update_rect(update_x, update_y, update_x1, update_y1);
}

void SdSdl::text_blt_done()
{
	if (m_SPECIAL & UNDER)
	{
		Uint32 *save_patptr, save_patmsk, save_fg_bp1;
		Sint32 save_NEXT_PAT;

		save_patptr = m_patptr; save_patmsk = m_patmsk;
		save_NEXT_PAT = m_NEXT_PAT; save_fg_bp1 = m_FG_BP_1;

		m_NEXT_PAT = 0;
		if (m_SPECIAL & LIGHT)
		{
			m_patmsk = DITHRMSK;
			m_patptr = &DITHER[12];
		}	
		else
		{	
			m_patmsk = 0;
			m_patptr = SOLID;
		}
		m_FG_BP_1  = m_text_pixel;
		RECTFILL();
		m_FG_BP_1  = save_fg_bp1;
		m_NEXT_PAT = save_NEXT_PAT;
		m_patmsk   = save_patmsk;
		m_patptr   = save_patptr;		
	}
}

/****************************************************************
* TEXT_BLT_SPECIAL						*
*****************************************************************/

void SdSdl::text_blt_special(const wchar_t *str, bool justified)
{
	PREBLIT  pre_blt;
	POSTBLIT post_blt;
	Uint32 offset_to_top;
	Sint32 clip;
	unsigned nchar;
	wchar_t ch;
	Uint32 char_wb, char_h, char_r, char_b;
	Uint32 form_siz, form_h;
	Sint32 char_y, char_x, ch_offset, srcx, char_wp;
	Uint8 *bits;
	unsigned n;

	chk_fnt();
// Save the address of the appropriate pre- and post-blt routines (a function
// of rotation).

	n = ((m_SPECIAL & ROTATE) >> 6);

	pre_blt  = pre_blt_table[n];
	post_blt = post_blt_table[n];
	
// Perform preliminary clipping against the non-baseline axis of the text.

	if (!(m_SPECIAL & ROTODD))
	{
// Clip in the y direction.  First, compare the cell against the clipping
// window bottom.
		offset_to_top = 0;	// Offset to top in form
		form_h = m_ACTDELY;	// Character form height
		char_y = m_DEST.y;	// Character cell top
		clip = m_YMX_CLIP;

		char_b = char_y + form_h - 1;	// Character cell bottom

		// Cell is entirely above clip bottom
		if (char_b > clip)
		{
			// Cell is entirely below clip bottom
			if (char_y > clip) return;
			char_b = clip;		// New cell bottom
			form_h = (clip - char_y) + 1;	// New cell height
		}
// At least part of the cell is above the bottom of the clipping rectangle.
// Check against the top of the clipping rectangle.
		clip = m_YMN_CLIP;
		// Cell top below clip top
		if (char_y < clip)
		{
			char_y = clip;
			// Cell totally above clip top
			if (char_b < clip) return;
			char_b -= (clip - 1);	// New cell height
	
			// Calculate new offset into the font
			offset_to_top = form_h - char_b;// The bit of the cell we're losing
			form_h = char_b;		// Cell height
		}
	// Save the clipping results.
		m_foffindex  = offset_to_top;
		m_tempdely   = form_h;
		m_y_position = char_y;
		char_x = m_DEST.x;		// Cell left x
	}
	else	// m_SPECIAL & ROTODD
	{
		offset_to_top = 0;	// Offset to top in font
		form_siz = m_ACTDELY;	// Character form height
		char_x = m_DEST.x;		// Character cell left
		clip = m_XMX_CLIP;

		char_r = char_x + form_siz - 1;	// Cell right
		if (char_r > clip)
		{
			if (char_x > clip)  return;
		
			char_r = clip;		// New cell right
			form_siz = clip - char_x + 1;	// New cell width		
		}
// At least part of the cell is left of the right edge  of the clipping
// rectangle.  Check against the left edge of the clipping rectangle.
		clip = m_XMN_CLIP;
		if (char_x < clip)
		{
			char_x = clip;	// New left-hand limit
			if (char_r < clip) return;
			char_r -= (clip - 1);
// Calculate the new offset into the font.
			offset_to_top = form_siz - char_r;	// New offset into font
			form_siz = char_r;	// New cell width
		}
// Save the clipping results.
		m_form_offset = offset_to_top;
		m_form_width  = form_siz;
		m_y_position = m_DEST.y;
	}
	m_char_count = wcslen(str);
// Top of the character output loop.  Get the character, the offset
// into the font form, and the width of the character in the font.
spec_char_loop:
	for (nchar = 0; str[nchar]; nchar++)
	{
// 11/6/86 horizontal offset enhancement
		m_rt_hoff = m_lt_hoff = 0;
		m_isspace = set_fnthdr(str[nchar]);	// get a character

		wchar_t glyph = (m_isspace - m_FONT_INF.first_ade);
		Uint32 *poffset = m_FONT_INF.off_table + glyph;
// 11/6/86 horizontal offset enhancement

                if (m_FONT_INF.flags & HORZ_OFF)
                {
                        m_lt_hoff = m_FONT_INF.hor_table[2 * glyph];
                        m_rt_hoff = m_FONT_INF.hor_table[2 * glyph + 1];
                }
		// Character width, pixels
		char_wp = poffset[1] - poffset[0];
	
		// Don't draw 0-width chars
		if (!char_wp) continue;
		
// Point the destination to the temporary buffer.
		m_buff_ptr = m_txbuf1;
// If doubling, adjust the character buffer form width (char_wb).
		char_wb = char_wp;
		if (!(m_SPECIAL & SCALE))
		{
			char_wb += m_CHAR_DEL;
		}
		// Convert pixels to bytes 
		char_wb = (char_wb + 7) / 8;
// Move the character from the font form to the temporary buffer.
		form_siz = text_move(char_wb, char_wp, poffset[0]);
// Apply scaling, if necessary.
		if (m_SPECIAL & SCALE) text_scale(form_siz, &char_wp, &char_wb);
// The status of the registers when calling the special effects routines
// is as follows:
//	bl = character buffer width, in bytes
//	cx = height of the character cell
//	dx = character width, in pixels
//	es:di -> temporary character buffer
//
// dx -> ch_offset
// si -> char_wp
//
		char_h = m_ACTDELY;
		ch_offset = char_wp;
		if (m_SPECIAL & SKEW)
		{
			text_italic(char_wp, char_h, char_wb);
			char_wp += m_L_OFF + m_R_OFF;
		}
		if (m_SPECIAL & THICKEN)
		{
// Apply bolding, if necessary.  Bump the character width only if the font is
// not monospaced.
			text_bold(char_h, char_wb);
//
// Move this 'char_wp += m_WEIGHT' outside the 'if', so that the cell size
// actually matches the character size. The existing GEM driver source leaves
// it in, which stops upside-down boldface characters from being drawn. This
// bug is present in SDCGA8 and SDPSM8, but in SDPSC9 it is fixed.
//
// Note that this causes boldface strings to come out wider than normal 
// when printed upside-down; right-side up, their width is the same. This
// is because pre_blt_180 adjusts character position by character width, not
// by cell offset; if you change it to use cell offset, italic text gets
// scrambled.
//
			char_wp += m_WEIGHT; 	
			if (!m_MONO_STATUS) 
			{ 
				ch_offset += m_WEIGHT; 
			} 
		}
// Apply lightening, if necessary.
		if (m_SPECIAL & LIGHT) text_grey(char_h, char_wb);
// Rotate, if necessary.
		if (m_SPECIAL & ROTATE) text_rotate(char_h, &char_wp, &char_wb, ch_offset);
// Special effects and attributes have been bound.

// Perform the necessary pre-blt operations, based on the rotation angle.
// Output the character and perform the necessary post-blt operations.
//
 		int cy = (this->*pre_blt)(&char_x, &ch_offset, char_wp, &srcx, &bits);
		if (!cy)
		{
/*
printf("About to text_blit; y=%d char_wb=%d ch_offset=%d srcx=%d char_x=%d\n"
		"%02x%02x%02x%02x%02x%02x%02x%02x\n"
		"%02x%02x%02x%02x%02x%02x%02x%02x\n",
		m_y_position,
		char_wb, ch_offset, srcx, char_x,
		m_buff_ptr[0], m_buff_ptr[1], m_buff_ptr[2], m_buff_ptr[3],
		m_buff_ptr[4], m_buff_ptr[5], m_buff_ptr[6], m_buff_ptr[7],
		m_buff_ptr[8], m_buff_ptr[9], m_buff_ptr[10], m_buff_ptr[11],
		m_buff_ptr[12], m_buff_ptr[13], m_buff_ptr[14], m_buff_ptr[15]
		);
*/
			text_blit(bits, char_wb, char_wp, srcx, char_x);
			if (justified) ch_offset = text_attr_just(ch_offset);
			(this->*post_blt)(&char_x, ch_offset);
		}
		--m_char_count;
		if (!m_char_count) break;
	}
/*Done with the character.  Prepare for the next one. 
	
spec_draw_done:
		pop	si
		dec	char_count
		jz	text_special_blit_done
		jmp 	spec_char_loop
*/
//text_special_blit_done:
	if (m_SPECIAL & SKEW) char_x += m_R_OFF;
	text_blt_done();

// XXX Work out the real bounding box
	flush();
}



/* Subroutine to adjust for justified text. */
Uint32 SdSdl::text_attr_just(Uint32 char_wp)
{
	if (m_isspace == ' ')
	{
		char_wp += m_JUST_DEL_WORD;
		if (m_JUST_DEL_WORD_REM)
		{
			char_wp += m_JUST_DEL_SIGN;
			--m_JUST_DEL_WORD_REM;
			return char_wp;
		}
	}
	char_wp += m_JUST_DEL_CHAR;
	if (m_JUST_DEL_CHAR_REM) --m_JUST_DEL_CHAR_REM;	
	return char_wp;
}

/****************************************************************
* PRE_BLT_0                                                     *
* Input:        none                                            *
* Input/Output:                                                 *
*       dx = offset to the next character                       *
*       bp = destination x position                             *
*       si = character width                                    *
* Output:                                                       *
*       carry flag:  set if character is clipped out            *
*       char_count:  number of characters to process            *
*       ax = source offset from left edge of form               *
*       di = bitmap address                                     *
* Preserved:    bx                                              *
* Trashed:      cx                                              *
*****************************************************************/

//			bp         dx         si              ax             di
bool SdSdl::pre_blt_0(Sint32 *x, Sint32 *dx, Sint32 char_w, Sint32 *srcx, Uint8 **bits) 
{
	Sint32 right_edge;	/* Right edge of cell */	

	(*x) -= m_lt_hoff;
 	*srcx = 0;
	right_edge = (*x) + char_w - 1;
	// Check for clipping on the left
	if ((*x) < m_XMN_CLIP)
	{
		if (right_edge < m_XMN_CLIP)
		{
			(*dx) = text_attr_just(char_w);
			*x += (*dx);
			return true;	// Character clipped out
		}
		char_w = right_edge - m_XMN_CLIP + 1;	// Visible width
		*srcx = (m_XMN_CLIP - (*x));	// First visible src column
		(*dx) -= *srcx;			// Reduce visible width
		*x = m_XMN_CLIP;
	}
	// Check for clipping on the right.
	if (right_edge > m_XMX_CLIP)
	{
		if ((*x) > m_XMX_CLIP)	// Character completely out of it
		{
			m_char_count = 1;
			return true;
		}
		char_w = (m_XMX_CLIP - (*x)) + 1;
	}
	*bits = concat(*x, m_y_position);
	return false;		
}


//			bp         dx         si              ax             di
bool SdSdl::pre_blt_90(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits) 
{ 
	Sint32 bottom, ax;

// Subtract the form height from the y position to obtain the screen y of the
// character's upper left corner.  The offset to the next character must be
// similarly patched to return the y position for the next pass through.
	m_y_position += m_lt_hoff;
	m_y_position -= (si = m_tempdely);
	*dx -= m_tempdely;
//  Find out if clipping is necessary on the top.
	ax = 0;
	bottom = m_y_position + m_tempdely - 1;
	if (m_y_position < m_YMN_CLIP)	// Clipping on the top
	{
		if (bottom < m_YMN_CLIP)
		{
			m_char_count = 1;
			return true;
		}
		si = bottom + 1 - m_YMN_CLIP;	/* Visible height */
		ax = m_YMN_CLIP - m_y_position;	/* Offset */
		*dx += ax;
		m_y_position = m_YMN_CLIP;
	}
	if (bottom > m_YMX_CLIP)
	{
		/* Character clipped out */
		if (m_y_position > m_YMX_CLIP)
		{
			m_y_position -= text_attr_just(si);
			return true;
		}
		si = 1 + m_YMX_CLIP - m_y_position;	/* Visible height */
	}
	m_foffindex = ax;
	m_tempdely  = si;
	*srcx = m_form_offset;
	*bits = concat(*x, m_y_position);
	return false;		
}
//			bp         dx         si              ax             di
bool SdSdl::pre_blt_180(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits) 
{ 
	Sint32 edge;

// Subtract the form width from the destination x position so that the
// character will be placed correctly.  The distance to the next character
// must also be updated.
	(*x) -= si;
	(*x) += m_lt_hoff;

// Note: This is deliberately "= -" not "-=".
	*dx = -(m_R_OFF + m_L_OFF);	// dx = offset to next

// Find out if clipping is necessary on the left side.

	*srcx = 0;		// Source offset from left
	edge = (*x) + si - 1;	// Right-hand edge of cell
	if ((*x) < m_XMN_CLIP)
	{
		if (edge < m_XMN_CLIP)
		{
			(*dx) = text_attr_just(si);
			*x += (*dx);
			return true;
		}
// Clipping is necessary on the left side.  Do it.
		si = (edge - m_XMN_CLIP) + 1;
		*srcx = (m_XMN_CLIP) - (*x);
		(*dx) += *srcx;
		*x = m_XMN_CLIP;
	}
// Find out if clipping is necessary on the right side.
	if (edge > m_XMX_CLIP)
	{
		if ((*x) > m_XMX_CLIP)
		{
			m_char_count = 1;
			return true;
		}
// Clipping is necessary on the right side.  Do it.
		si = (m_XMX_CLIP - (*x) + 1);
	}
	*bits = concat(*x, m_y_position);
	return false;
}

//			bp         dx         si              ax             di
bool SdSdl::pre_blt_270(Sint32 *x, Sint32 *dx, Sint32 si, Sint32 *srcx, Uint8 **bits) 
{
	Sint32 bottom, ax;
// Get the character y position and the "height" (screen sense) of the
// character.
	m_y_position -= m_lt_hoff;
	si = m_tempdely;
//		push	bp
//		mov	bp, y_position

// Find out if clipping is necessary on the top.
	ax = 0;
	bottom = m_y_position + m_tempdely - 1;
	if (m_y_position < m_YMN_CLIP)
	{
		if (bottom < m_YMN_CLIP)	/* Completely outside clip */
		{
			m_char_count = 1;
			return true;
		}
		si = bottom + 1 - m_YMN_CLIP;	/* Visible height */
		ax = m_YMN_CLIP - m_y_position;
		*dx -= ax;			/* New offset */
		m_y_position = m_YMN_CLIP;
	}
// Find out if clipping is necessary on the bottom.
	if (bottom > m_YMX_CLIP)
	{
		if (m_y_position > m_YMX_CLIP)
		{
			m_y_position += (*dx = text_attr_just(si));
			return true;
		}
		si = 1 + m_YMX_CLIP - m_y_position;
	}
	m_foffindex = ax;
	m_tempdely = si;
	*srcx = m_form_offset;
	*bits = concat(*x, m_y_position);
	return false;
}



/****************************************************************
 * POST_BLT_0							*
 * Input:							*
 *	dx = offset to the next character			*
 * Input/Output:						*
 * 	bp = destination x position				*
 * Output:	none						*
 * Preserved:	ax, bx, cx, dx, si, di				*
 * Trashed:	none						*
 ****************************************************************/

void SdSdl::post_blt_0(Sint32 *x, Sint32 dx)
{
// For 180 degree rotation, move to the left.
		(*x) -= m_rt_hoff;
		(*x) += dx;
}


/****************************************************************
 * POST_BLT_90							*
 * Input:							*
 *	dx = offset to the next character			*
 * Input/Output:						*
 *	y_position:  screen y position				*
 * Output:	none						*
 * Preserved:	ax, bx, cx, dx, si, di, bp			*
 * Trashed:	none						*
 ****************************************************************/

void SdSdl::post_blt_90(Sint32 *x, Sint32 dx)
{
	m_y_position += m_rt_hoff;
	m_y_position -= dx;
}	

/****************************************************************
 * POST_BLT_180							*
 * Input:							*
 *	dx = offset to the next character			*
 * Input/Output:						*
 * 	bp = destination x position				*
 * Output:	none						*
 * Preserved:	ax, bx, cx, dx, si, di				*
 * Trashed:	none						*
 ****************************************************************/

void SdSdl::post_blt_180(Sint32 *x, Sint32 dx)
{
// For 180 degree rotation, move to the left.
		(*x) += m_rt_hoff;
		(*x) -= dx;
}


/****************************************************************
* POST_BLT_270							*
* Input:							*
*	dx = offset to the next character			*
* Input/Output:							*
*	y_position:  screen y position				*
* Output:	none						*
* Preserved:	ax, bx, cx, dx, si, di, bp			*
* Trashed:	none						*
*****************************************************************/

void SdSdl::post_blt_270(Sint32 *x, Sint32 dx)
{
// For 270 degree rotation, move down.
		m_y_position -= m_rt_hoff;
		m_y_position += dx;
}


