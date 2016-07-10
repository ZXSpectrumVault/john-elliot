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

/****************************************************************
 * TEXT_BLIT							*
 * moves the character from temp form to screen			*
 * Inputs:							*
 *		ES:DI		Pointer to the screen		*
 *		foffindex	Source form lines to skip	*
 *		tempdely	Char height in pels		*
 *		bl		Char buffer form width in bytes	*
 *		si		char width in pels		*
 *		ax		source x			*
 *		bp		dest x				*
 * Uses:		all					*
 * Returns:	none						*
 ****************************************************************/

void SdSdl::text_blit(Uint8 *bits/*di*/,
			Uint32 form_wb, Uint32 char_wp, 
			Uint32 ax/*sx*/, Uint32 destx)
{
	m_font_off = m_buff_ptr + m_foffindex * form_wb;
// Because SDL doesn't have a multiple-plane memory model, I'm going to
// skip the "right and left masks" bit. Just plot tempdely lines of 
// char_wp pixels. 

	for(unsigned y = 0; y < m_tempdely; y++)
	{
		Uint8 *font_bits = m_font_off + (y * form_wb);
		Uint8 mask = 0x80;
		switch(m_text_mode)
		{
			case 0: txtblt_rep(char_wp, bits, font_bits, mask); break;
			case 1: txtblt_tran(char_wp, bits, font_bits, mask, m_text_ix); break;
			case 2: txtblt_xor(char_wp, bits, font_bits, mask); break;
			case 3: txtblt_tran(char_wp, bits, font_bits, mask, m_bg_pixel); break;
		}
                bits += m_sdl_pitch;
	}
}


static unsigned char byte_mask_table[9] = { 0xff, 0x7f, 0x3f, 0x1f,
					    0x0f, 0x07, 0x03, 0x01, 
					    0x00 };


/****************************************************************
* TEXT_MOVE							*
*								*
* moves the character from font form to temp			*
* Inputs:							*
*		ES:DI	Pointer to the char buffer		*
*		DELY	Char height in pels			*
*		foff	font offset 				*
*		fseg	font segment				*
*		bl	Char buffer form width in bytes		*
*		si	char width in pels			*
*		ax	source x				*
* Uses:								*
*		all						*
* Returns:							*
*		dx = to next form				*
*****************************************************************/

Uint32 SdSdl::text_move(Uint32 form_w, Uint32 char_w, Uint32 src_x)
{
	Uint8  lmask, rmask;
	Uint32 dely;
	Uint8 *si, *di = m_buff_ptr;

       	dely = m_cur_font->form_height;
	memset(m_buff_ptr, 0, form_w * dely);	// clear out the temp buffer

	int bp = 0;
//char_w = CHAR WIDTH		calculate the masks , rotate counts
//bp = destx = 0
//src_x = sourcex	
	rmask = byte_mask_table[char_w & 7];	// Right mask
	lmask = 0;				// Left mask
	
// calculate rotate value
// rotate value = bh 0000drrr	d=0 if right 1 if left rrr = count 0 - 7
// Rather than trying to understand and replicate the 8086 logic (which put 
// me off this for about 4 years), it's easier just to look up the values
// in a table...

	static Uint8 rotate[8] = { 0x00, 0x0F, 0x0E, 0x0D, 
				   0x0C, 0x0B, 0x0A, 0x09 };
	Uint8 bh = rotate[src_x & 7];

// calculate the vertical loop count
// vertical loop count = cl
       	dely = m_cur_font->form_height;
	
// calculate the middle byte count
// middle byte count = ch

	Uint8 bytes_w;
	Uint8 ah = bh;
	if (char_w < 8) 
	{
		lmask |= rmask;
		rmask = 0xFF;
		bytes_w = 0;
	}
	else bytes_w = ((char_w - 8) >> 3);

	// SI -> character bitmap
	si = (src_x >> 3) + m_cur_font->dat_table;
	// BP = vertical spacing
	bp = m_cur_font->form_width;
	int rl  = bh & 8;
	bh &= 7;	
/*
	fprintf(stderr, "bh=%d bytes_w=%d bp=%d lmask=%x rmask=%x si=%p->"
			"%02x%02x%02x%02x%02x%02x%02x%02x\n",
			bh, bytes_w, bp, lmask, rmask, si,
			si[0], si[0x100], si[0x200], si[0x300], si[0x400],
			si[0x500], si[0x600], si[0x700]);
*/		
	if (rl)	while(dely)
	{
		txtblt_xor_rl_s(1, bh, bytes_w, bp, lmask, rmask, &si, di);
		di += form_w;
		--dely;
	}
	else while (dely)
	{
		txtblt_xor_rr_s(1, bh, bytes_w, bp, lmask, rmask, &si, di);
		di += form_w;
		--dely;
	}
	return form_w * m_cur_font->form_height;
}

/****************************************************************
 * TEXT_ITALIC							*
 * Italicizes the form passed to it				*
 * Inputs:							*
 *		ES:DI	Pointer to the char buffer		*
 *		cx	Char height in pels			*
 *		bl	Char buffer width in bytes		*
 *		dx	Char width in pels			*
 * Uses:	all						*
 * Returns:	none						*
 ****************************************************************/

void SdSdl::text_italic(Uint32 char_w, Uint32 rows, Uint32 buf_w)
{
	Uint8 *src, *dest;
	Uint8 leftx, rightx;
	Uint8 leftmask, rightmask, mid_bytes;

	rightx = (char_w & 7);
	leftx = 0;
       	src = m_buff_ptr + ((rows - 1) * buf_w);
				/* SI->bottom of char form */
	dest = m_ptsin_buf;
/*
 * es:di = pointer to ptsin	 ds:si = pointer to bottom of char form
 * bh = leftx  = 00000xxx	 bl = rightx = 00000xxx
 * cx = vert scan count in pels	 ax = char form width in bytes
 * bp = char width in pels
 */
	while (1)
	{
/* this code is executed every two scan lines */
		memset(m_ptsin_buf, 0, buf_w); /* Clear out the temp dest */
/* Calculate the left mask */
		leftmask = 0;
		rightmask = byte_mask_table[rightx];
/* If leftx + char_w >= 8, then there's at least one whole byte to
 * copy */
		Sint32 char_w1 = char_w + leftx - 8;
		if (char_w1 >= 0)
		{
			/* Divide out the low order bits */
			mid_bytes = (char_w1 >> 3) & 0xFF;
		}
		else
		{
			char_w -= (8 - leftx);
			leftmask |= rightmask;	/* If it is left only then must
					   	   modify masks */
			rightmask = 0xFF;	/* The right mask is now none */
			mid_bytes = 0;		/* The next add delta is 0 */
		}
//		printf("Italicise odd row:  leftx=%d mid_bytes=%d char_w=%d "
//			"lmask=%x rmask=%x src=%p dest=%p\n",
//			leftx, mid_bytes, char_w, leftmask, rightmask, src, dest);
		Uint8 *si = src;
		txtblt_xor_rr_s(1, leftx, mid_bytes, char_w, leftmask, rightmask, &si, dest); 
/* Copy the rotated buffer back */
		memcpy(src, m_ptsin_buf, buf_w);
		src -= buf_w;	/* Move backwards up the char */
		--rows;
		if (!rows) break;
/* Now the next scanline */
		memset(m_ptsin_buf, 0, buf_w);
		si = src;
//		printf("Italicise even row: leftx=%d mid_bytes=%d char_w=%d "
//			"lmask=%x rmask=%x src=%p dest=%p\n",
//			leftx, mid_bytes, char_w, leftmask, rightmask, src, dest);
		txtblt_xor_rr_s(1, leftx, mid_bytes, char_w, leftmask, rightmask, &si, dest); 
		memcpy(src, m_ptsin_buf, buf_w);
		--rows;
		if (!rows) break;
		src -= buf_w;	/* Move up a row */
		++leftx;
		++rightx;
		if (leftx > 7) ++dest;
		leftx  &= 7;
		rightx &= 7;
	}
}

/****************************************************************
* TEXT_BOLD							*
* Bolds the form passed to it					*
* Inputs:							*
*		ES:DI	Pointer to the char buffer		*	
*		cx	Char height				*
*		bl	Char buffer width in bytes		*
* Uses:		all						*
* Returns:	bx, cx, dx, ds					*
*****************************************************************/
void SdSdl::text_bold(Uint32 char_h, Uint32 buf_w)
{
	Uint32 w, y, x;
	Uint8 *ptr;
	Uint8 cy;
	Uint16 cur;

	for (y = 0; y < char_h; y++)
	{
		ptr = m_buff_ptr + buf_w * y;
		// this loop bolds the scan line  
		for (w = 0; w < m_WEIGHT; w++)
		{
			cy = 0;
			// this loop bolds by 1 pel the scan line
			for (x = 0; x < buf_w; x++)
			{
				cur = ptr[x];		// Original byte
				if (cy) cur |= 0x100;	// Low bit of last byte
				cy = cur  & 1;
				cur = cur >> 1;
				ptr[x] |= cur;
			}
		}
	}
}

/****************************************************************
 * TEXT_GREY							*
 * Greys the form passed to it					*
 * Inputs:							*
 *		ES:DI	Pointer to the char buffer		*
 *		cx	Char height				*
 *		bl	Char buffer width in bytes		*
 * Uses:		ax, bx, cx, di, ds			*
 * Returns:	bx, cx, dx					*
 ****************************************************************/

void SdSdl::text_grey(Uint32 char_h, Uint32 buf_w)
{
	unsigned char mask = 0xAA;
	int x, y;
	unsigned char *ptr = m_buff_ptr;
	
	for (y = 0; y < char_h; y++)
	{
		for (x = 0; x < buf_w; x++) 
		{
			*ptr &= mask;
			++ptr;	
		}
		mask = ~mask;	/* invert the mask on alternating scans */
	}
}


/****************************************************************
 * TEXT_SCALE							*
 * Scales the form passed to it					*
 * Inputs:							*
 *		ES:DI	Pointer to the char buffer		*
 *		cx	Char height				*
 *		bl	Char buffer width in bytes		*
 * 		si	Char width in pels			*
 *		dx	Source form size in bytes		*
 * Uses:	ax, bx, cx, di, ds				*
 * Returns:	bl, si						*
 ****************************************************************
 * ES:DI -> m_buff_ptr
 */ 				
//				dx		si            bl 
void SdSdl::text_scale(Uint32 form_siz, Sint32 *char_w, Uint32 *buf_w)
{
	Uint8 *diptr, *siptr;
	Uint32 cx, ax, char_wb;
	Uint32 dx = form_siz, si = *char_w, bx = *buf_w;
	Sint32 remainder;
	Uint32 bp;
	double y_dda;

/* Shortcut for straight doubling */
	if (m_T_SCLSTS & 1)
	{
		if (m_DDA_INC == 1.0)
		{
		/* double the current character in size */
		/* 11/6/86 horizontal offset enhancement */
			m_lt_hoff *= 2;
			m_rt_hoff *= 2;
			cx = ax = *buf_w;
			si *= 2;
			/* BX = New form width */
			bx = (si + m_CHAR_DEL + 7) >> 3; 
			char_wb = (*char_w * 2 + 7) >> 3; /* char width bytes */
			remainder = bx - char_wb;
//fprintf(stderr,"doubling letter: bx=%d char_wb=%d remainder=%d\n", bx, char_wb, 
//		remainder);
// push bx, si, ds
			siptr = m_buff_ptr;
			m_buff_ptr += form_siz;	
			diptr = m_buff_ptr;
			for (dx = 0; dx < m_ACTDELY; dx+= 2)
			{
// Scale a line of bytes
				Uint8 *dest0 = diptr;
				Uint8 *src0  = siptr;
				cx = char_wb;
				while (cx)
				{
					ax = m_globals->m_double_table[*siptr++];
					*diptr++ = (ax >> 8);
					--cx;
					if (!cx) break;
					*diptr++ = (ax & 0xFF);
					--cx;
				}
				if (remainder)
				{
					memset(diptr, 0, remainder);
					diptr += remainder;
				}
// Duplicate scaled line
				memcpy(diptr, dest0, (diptr-dest0));
				diptr += (diptr - dest0);
				*buf_w = bx;
				*char_w = si;
			}
			return;
		}
		else	/* Arbitrary scale up */
		{	
//printf("Scaling by %f: lt_hoff=%d rt_hoff=%d char_w=%d\n", m_DDA_INC, 
//		m_lt_hoff, m_rt_hoff, *char_w);
			m_buff_ptr += form_siz;
			y_dda = m_XACC_DDA;
			ax = 0;
			double old_xacc_dda = m_XACC_DDA;
			for (cx = 0; cx < m_lt_hoff; cx++)
			{
				y_dda += m_DDA_INC;
				if (y_dda > 1.0) { y_dda -= 1.0; ++ax; }
			}
			m_lt_hoff += ax;
			y_dda = m_XACC_DDA;
			cx = m_rt_hoff;
			for (cx = 0; cx < m_rt_hoff; cx++)
			{
				y_dda += m_DDA_INC;
				if (y_dda > 1.0) { y_dda -= 1.0; ++ax; }
			}
			m_rt_hoff += ax;
			y_dda = m_XACC_DDA;
			ax = 0;
			for (cx = 0; cx < *char_w; cx++)
			{
				y_dda += m_DDA_INC;
				++ax;
				if (y_dda > 1.0) { y_dda -= 1.0; ++ax; }
			}
			m_XACC_DDA = y_dda;
			y_dda = old_xacc_dda;	
			siptr = m_buff_ptr - form_siz;	
					// Original value of buff_ptr
			diptr = m_buff_ptr;
			Uint32 line_wb = ((ax + m_CHAR_DEL + 7) >> 3);
			Uint32 row;
//printf("Scaled by %f: lt_hoff=%d rt_hoff=%d char_w=%d\n", m_DDA_INC, 
//		m_lt_hoff, m_rt_hoff, ax);

// ax = y_dda_acumulator
// bx = bh,bl bh = source vertical count  bl = dest form width in bytes
// cx = source horizontal char width in pels
// es:di = dest		;ds:si = source
// dx = dda increment	;bp = x_dda_accumulator
//
#define INC_RMASK \
	rmask = rmask >> 1; if (!rmask) { rbyte = *rdptr++; rmask = 0x80; }
#define INC_WMASK \
	wmask = wmask >> 1; if (!wmask) { *wrptr++ = wbyte; wmask = 0x80; wbyte = 0;}
			y_dda = 0.5;
			for (row = 0; row < m_cur_font->form_height; row++)
			{
				// Scale a single row.
				// Init the DDA accumulator
				double x_dda = old_xacc_dda;
				Uint8 rmask = 0x80, wmask = 0x80;
				Uint8 rbyte, wbyte, *rdptr, *wrptr;
				cx = *char_w;
				wbyte = 0;
		
				rdptr = siptr + (*buf_w) * row;
				wrptr = diptr;
		
				rbyte = *rdptr++;	
				for (cx = 0; cx < *char_w; ++cx)
				{
					if (rbyte & rmask)	/* Solid pixel */
					{
						x_dda += m_DDA_INC;
						if (x_dda > 1.0)
						{
		/* Write extra pixel */
							x_dda -= 1.0;
							wbyte |= wmask;
							INC_WMASK
							//putchar('+');
						}
		/* Write normal pixel */
						wbyte |= wmask;
						INC_WMASK
						//putchar('-');
					}
					else 	/* blank pixel */
					{
						x_dda += m_DDA_INC;
						if (x_dda > 1.0)
						{
							x_dda -= 1.0;
							INC_WMASK
							//putchar('_');
						}
						INC_WMASK
						//putchar('.');
					}
		/* Get next pixel */
					INC_RMASK
				}
		/* End of horizontal scaling loop */
				//putchar('\n');
		/* Flush any outstanding bits */
				if (wmask != 0x80) *wrptr++ = wbyte; 
				*wrptr++ = 0;
		
				y_dda += m_DDA_INC;
				if (y_dda >= 1.0)
				{
					y_dda -= 1.0;
					memcpy(diptr + line_wb, diptr, line_wb);
					diptr += 2*line_wb; // BL = form width, bytes
				}
				else
				{
					diptr += line_wb;
				}
			}
			*buf_w = line_wb;
			*char_w = ax;
		}
	}
	else	/* Scaling down */
	{
		m_buff_ptr += form_siz;
		y_dda = m_XACC_DDA;
		ax = 0;
		double old_xacc_dda = m_XACC_DDA;
		for (cx = 0; cx < m_lt_hoff; cx++)
		{
			y_dda += m_DDA_INC;
			if (y_dda > 1.0) y_dda -= 1.0; else ++ax; 
		}
		m_lt_hoff = ax;
		y_dda = m_XACC_DDA;
		cx = m_rt_hoff;
		for (cx = 0; cx < m_rt_hoff; cx++)
		{
			y_dda += m_DDA_INC;
			if (y_dda > 1.0) y_dda -= 1.0; else ++ax; 
		}
		m_rt_hoff = ax;
		y_dda = m_XACC_DDA;
		ax = 0;
		for (cx = 0; cx < *char_w; cx++)
		{
			y_dda += m_DDA_INC;
			if (y_dda > 1.0)  y_dda -= 1.0; else ++ax; 
		}
// AX = new char width
		m_XACC_DDA = y_dda;
		y_dda = old_xacc_dda;	
		siptr = m_buff_ptr - form_siz;	
		diptr = m_buff_ptr;
// Work out new bytes/line.
		Uint32 line_wb = ((ax + m_CHAR_DEL + 7) >> 3);
// Must be at least 1 byte/line.
		if (!line_wb) ++line_wb;	
		Uint32 row;

//
// Unlike scaling up, which knows it's going to write to a completely new 
// row each time, scaling down has to write to the same row multiple times.
// Therefore we clear a row when moving onto it, and then OR one or more
// scaled rows into it.
//
#undef INC_WMASK
#define INC_WMASK \
	wmask = wmask >> 1; if (!wmask) { *wrptr++ |= wbyte; wmask = 0x80; wbyte = *wrptr; ;}
			
		// Clear out the dest line.
		memset(diptr, 0, line_wb);
		y_dda = 0.5;
		for (row = 0; row < m_cur_font->form_height; row++)
		{
			// Scale a single row.
text_scale_down_no_inc_loop:
			// Init the DDA accumulator
			double x_dda = old_xacc_dda;
			Uint8 rmask = 0x80, wmask = 0x80;
			Uint8 rbyte, wbyte, *rdptr, *wrptr;
			cx = *char_w;
			wbyte = *diptr;
			rdptr = siptr + (*buf_w) * row;
			wrptr = diptr;
		
			rbyte = *rdptr++;	
			for (cx = 0; cx < *char_w; ++cx)
			{
				if (rbyte & rmask)	/* Solid pixel */
				{
					wbyte |= wmask;
					x_dda += m_DDA_INC;
					if (x_dda > 1.0) x_dda -= 1.0;
					else
					{
						INC_WMASK;
						//putchar('+');
					}
				}
				else 	/* blank pixel */
				{
					x_dda += m_DDA_INC;
					if (x_dda > 1.0) x_dda -= 1.0;
					else
					{
						INC_WMASK
						//putchar('.');
					}
				}
		/* Get next pixel */
				INC_RMASK
			}
		/* End of horizontal scaling loop */
			//putchar('\n');
		/* Flush any outstanding bits */
			if (wmask != 0x80) *wrptr++ = wbyte; 
			*wrptr++ = 0;
		
			y_dda += m_DDA_INC;
			if (y_dda >= 1.0) y_dda -= 1.0;
			else
			{
				diptr += line_wb;
				// Clear out the dest line.
				memset(diptr, 0, line_wb);
			}
		}
		*buf_w = line_wb;
		*char_w = ax;
	}
}

#ifdef byte_swap			// Save it off
# define SAVEWORD do { *wrptr++ = bx & 0xFF; *wrptr++ = (bx >> 8); } while(0)
#else
# define SAVEWORD do { *wrptr++ = (bx >> 8); *wrptr++ = bx & 0xFF; } while(0)
#endif

/****************************************************************
 * TEXT_ROTATE							*
 * Inputs:							*
 *	bl = character buffer width, in bytes			*
 *	cx = height of the character cell			*
 *	dx = offset to the next character			*
 *	si = character width -- all horizontal effects applied	*
 *	es:di -> temporary character buffer			*
 ****************************************************************/

//				cx		si
void SdSdl::text_rotate(Uint32 cell_h,  Sint32 *char_wp, 
//				bl		dx
		        Uint32 *char_wb, Uint32 offset)
{
	Uint32 SPECIAL_CS;
	Sint32 next_source, next_dest;
	Uint8 *si, *di, *wrptr, *rdptr;
	Sint32 cx1, cx2;
	Uint32 bp;
	Uint32 dx;
	Uint32 bx;

	dx = *char_wp;
	m_rot_height = cell_h;
// The source and destination pointers must be modified.  The source will
// be somewhere in the temporary buffer and the destination will be somewhere
// else in the second temporary buffer.  Exactly where depends on the rotation
// angle.
	si = m_buff_ptr;
	di = m_txbuf2;

// Text rotated 180 degrees is handled differently from text rotated either
// 90 or 270 degrees.
	if (m_SPECIAL & ROTODD)
	{
// Set up the increment values to the next source line and next destination
// line.
		m_buff_ptr = di;
		if (!dx) ++dx;	// If no width, give it a width.
		next_source = *char_wb;
		next_dest = (cell_h + 7) >> 3;

		if (!(m_SPECIAL & ROTHIGH))
		{	
// 90 degrees:  the starting source will be the beginning of the
// temporary character buffer and the starting destination will be the
// beginning of the last scan line in the second buffer.
			di += (dx - 1) * next_dest;
			next_dest = -next_dest;
		}
		else
		{
// 270 degrees:  the starting source will be the beginning of the last scan
// line in the temporary character buffer and the starting destination will
// be the beginning of the second buffer.

			si += (cell_h - 1) * (*char_wb); 
			next_source = -next_source;
		}
//		printf("text_rotate: next_source=%d next_dest=%d char_wp=%d offset=%d\n",
//				next_source, next_dest, *char_wp, offset);
		cx1 = dx;
#ifdef byte_swap
		dx = 0x8000;	// Source mask
#else
		dx = 0x0080;
#endif
		for (; cx1 > 0; cx1--)
		{
// Outer rotation loop
			bx = 0;
			rdptr = si;
			wrptr = di;
			bp = 0x8000;	// Dest mask
// Inner rotation loop 
			//putchar('\n');
			for (cx2 = m_rot_height; cx2 > 0; cx2--)
			{
// Weird that there should be a little-endian assumption just here.
				if ((*(Uint16 *)rdptr) & dx)
				{
					//putchar('*');
					bx |= bp;
				}
				//else	putchar(' ');
				bp = bp >> 1;
				if (bp)
				{
					rdptr += next_source;	
					continue;
				}
				bp = 0x8000;	
				SAVEWORD;
				bx = 0;	// start clean
				// jn 11-21-87
				if (cx2 == 1) goto rotate_odd_save_done; 
				rdptr += next_source;	
			}
// End of inner loop	
//			printf("\nEnd inner loop with next_dest=%d bx=%x\n",
//					next_dest, bx);
			if (next_dest & 1) *wrptr++ = (bx >> 8);
			else SAVEWORD;
rotate_odd_save_done:
			di += next_dest;
#ifndef byte_swap
			dx = ((dx << 8) & 0xFF00) | ((dx >> 8) & 0x00FF);
#endif
			dx = dx >> 1;
			if (!dx) 	// Next source pixel column 
			{
				si += 2;
				dx = 0x8000;
			}
#ifndef byte_swap
			dx = ((dx << 8) & 0xFF00) | ((dx >> 8) & 0x00FF);
#endif

		}	
	}
	else
	{
// 180 degrees:  the starting source will be the beginning of the temporary
// character buffer and the starting destination will be the last used byte
// in the second temporary buffer.
		m_buff_ptr = di;
		di += (cell_h * (*char_wb));
		--di;		// DI -> second buffer, last byte
		Uint8 ah, al, ch, cl;
	       
		cl = (*char_wp) & 7;
		if (!cl) ch = 0x80;
		else	 ch = 1 << (cl - 1);
		// CH = destination rotate mask
		cl = 1;	
		ah = 0;		// 'reflected' byte
		al = *si++;	// Prime with the first byte
		int inst_src = 0, inst_dest = 14;
		for (cx1 = (cell_h * (*char_wb)); cx1 > 0; )
		{
			// Rotate top bit of AL into the 'reflected' byte
// rol al,1 ! rcr ah,1
			ah = ah >> 1;
			if (al & 0x80) 
			{
				ah |= 0x80;
			}
			al = al << 1;
// rol cl,1
			cl = cl << 1;
			if (!cl)
			{
				cl = 1;
				al = *si++;
			}
// ror ch,1
			ch = ch >> 1;
			if (!ch)
			{
				*di-- = ah;
				ah = 0;
				ch = 0x80;
				--cx1;
			}
		}
// If I comment this out, italic works and bold doesn't.
// If I leave it in, bold works and italic doesn't.
// The problem is that bold is aligned to the right where its first byte 
// is entirely empty. Italic isn't.
//
// You know what - this is a real live GEM bug! Replicated on GEM/3 with 
// SDCGA8.CGA and SDPSM8.VGA, but not SDPSC9.VGA.
//
//		m_buff_ptr = di;
	}
	// Swap cell width & height
	if (m_SPECIAL & ROTODD)
	{
		m_tempdely = *char_wp;
		*char_wp = m_rot_height;
		*char_wb = ((*char_wp) + 7) >> 3;

//		printf("\nnew tempdely=%d\n", m_tempdely);
//		printf("new char_wb=%d\n", *char_wb);
//		printf("new char_wp=%d\n", *char_wp);

	}
/*

; 180 degrees:  the starting source will be the beginning of the temporary
; character buffer and the starting destination will be the last used byte
; in the second temporary buffer.
rotate_180:
		mov	al, bl			; width in bytes
		mul	cl			; width * height (bytes)
		add	di, ax
		dec	di			; es:di -> 2nd buff last byte
		mov	cl, dl			; cell width in pixels
		and	cl, 7			; what`s in the leftover byte?
		mov	ch, 80h
		rol	ch, cl			; ch = destination rotate mask
		mov	bx, ax			; bx = number of bytes to do
		lodsb				; prime with the first byte
		mov	cl, 1h			; cl = source rotate mask
		xor	ah, ah			; start clean

; Top of the 180 degree byte processing loop.
rotate_180_loop_top:
		rol	al, 1			; get high bit into carry
		rcr	ah, 1			; store the bit
		rol	cl, 1			; count the source bit
		jnc	rotate_180_rotate_dest
		lodsb				; get the next byte
rotate_180_rotate_dest:
		ror	ch, 1			; count the destination
		jnc	rotate_180_loop_top
		mov	es:[di], ah		; store the flipped byte
		xor	ah, ah			; start clean
		dec	di
		dec	bx			; another one bites the dust
		jnz	rotate_180_loop_top

; Restore input registers.
rotate_done:
		pop	ds
		pop	si
		pop	dx
		pop	bx

; Swap height and width information in the input registers if the rotation
; is either 90 or 270 degrees.
		test	SPECIAL, ROTODD
		jz	end_text_rotate
		mov	tempdely, si		; update "height" of the cell
		mov	si, rot_height		; si = "width" in pixels
		mov	bx, si
		add	bl, 7
		shr	bl, 1
		shr	bl, 1
		shr	bl, 1			; bl = "width" in bytes
end_text_rotate:
		ret
*/
}
#if 0

W_0	equ	word ptr 0
W_1	equ	word ptr 2
W_2	equ	word ptr 4
W_3	equ	word ptr 6
#endif

/*******************************************
*d_justif
*	justified text entry point
*	Entry
********************************************/

void SdSdl::d_justified(const GPoint *pt, const wchar_t *str,
			Uint32 word_space, Uint32 char_space)
{
	// Take a copy of the string so d_justif_calc() can fiddle with it
	wchar_t *buf = new wchar_t[1 + wcslen(str)];
	Uint32 *siz  = new Uint32 [1 + wcslen(str)];
	wcscpy(buf, str);
	chk_fnt();
	word_space |= 1;	// Make sure it does inter word
	if (d_justif_calc(pt, buf, word_space, char_space, siz))
	{
		graphicText(pt, buf, true, siz);
	}
	delete siz;
	delete buf;
}



/*******************************************
*d_justif_calc
*	justified text entry point
*
*	Entry
********************************************/

Sint32 SdSdl::d_justif_calc(const GPoint *pt, wchar_t *text,
		                        Uint32 word_space, Uint32 char_space,
					Uint32 *out)
{
	Sint32 ax, bx, width, spacew, minsp, sign;
	Uint32 dx, txtlen, spaces;
	wchar_t *si;
	Uint32 *di;
	Uint8 *di8;
	Uint32 glyph;
	Sint32 q, r;
	int n;

	word_space |= 1;	// Make sure it does inter word (again?)

	width = 0;	// Init the width counter
	dx = 0; // Init the spaces counter
	txtlen = wcslen(text);	// No characters?
	if (!txtlen) return 0;
	
	si = text;
	si[0] = set_fnthdr(si[0]);
	di = m_FONT_INF.off_table;
	spaces = 0;
	// Is this a width inquire only?
	if ((word_space & 0xFFFE) == 0x8000)
	{
		for (n = 0; n < txtlen; n++)
		{
			si[n] = set_fnthdr(si[n]);
			if (si[n] == ' ') ++spaces;	// Increase count of spaces
			glyph = si[n] - m_FONT_INF.first_ade;
			di = &m_FONT_INF.off_table[ glyph ];
			width += (di[1] - di[0]);	// Char width 
//11/6/86 DH
//added to horizontal offset calculation
			if (m_FONT_INF.flags & HORZ_OFF)
			{
				di8 = &m_FONT_INF.hor_table [ glyph ];
				width -= di8[0];
				width -= di8[1];
			}
		}
// Get the width of a space.
		glyph = set_fnthdr(' ') - m_FONT_INF.first_ade;			
		di = &m_FONT_INF.off_table[ glyph ];
		spacew = di[1] - di[0];
		if (m_FONT_INF.flags & HORZ_OFF)
		{
			di8 = &m_FONT_INF.hor_table [ glyph ];
			spacew -= di8[0];
			spacew -= di8[1];
		}
//
// width of string with no attributes
// width = width
// space width = spacew (di)
// space count = spaces
// 
		if (m_SPECIAL & SCALE)
		{
			if (m_DDA_INC == 1.0)
			{
				spacew *= 2;
				width *= 2;
			}
			else
			{
				width = ACT_SIZ(width);
				// Not present in GEM, but I think it
				// should be...
				spacew = ACT_SIZ(spacew);
			}
		}
		if (m_SPECIAL & THICKEN)
		{
			width += (wcslen(text) * m_WEIGHT);
		}
		m_width = pt[1].x;	// Desired width
		if (m_SPECIAL & ROTODD)
		{
			// If rotated, scale to aspect ratio
			m_width = SMUL_DIV(m_width, xsize, ysize);
		}
		sign = 0;
		bx = 1;
		ax = m_width - width;
		if (ax >= 0)
		{
			minsp = 1000;
		}
		else
		{
			minsp = (spacew / 2);
			bx = -1;
			ax = -ax;
			sign = 1;
		}
		m_JUST_NEG = sign;
		m_JUST_DEL_SIGN = bx;
		bx = 0;
		m_JUST_DEL_WORD = 0;
		m_JUST_DEL_WORD_REM = 0;
		m_JUST_DEL_CHAR = 0;
		m_JUST_DEL_CHAR_REM = 0;
// ax = width
// spaces = space count 
// Can we justify spaces between words?
		if (word_space && spaces)
		{
			q = (ax / spaces);	// Pels per space
			r = (ax % spaces);	// Remainder
			if (q >= minsp)
			{
				m_JUST_DEL_WORD = minsp;
			}
			else
			{
				m_JUST_DEL_WORD = q;
				m_JUST_DEL_WORD_REM = r;
				if (sign)
				{
					m_JUST_DEL_WORD = -m_JUST_DEL_WORD;
					m_JUST_DEL_CHAR = -m_JUST_DEL_CHAR;
				}
				return width;
			}
		}		
/* 
 * inter character only is allowed
 * width     = ax
 * dir       = sign
 * numspaces = ch
 * max space = minsp
 */
		if (char_space)
		{
			/* Subtract space taken by words */
			ax -= (minsp * spaces); 
			q = (ax / txtlen);
			r = (ax % txtlen);
			m_JUST_DEL_CHAR     = q;
			m_JUST_DEL_CHAR_REM = r;
		}
		if (sign)
		{
			m_JUST_DEL_WORD = -m_JUST_DEL_WORD;
			m_JUST_DEL_CHAR = -m_JUST_DEL_CHAR;
		}
		return width;
	}
/*
 * returns the width of each character in the string in dev coords
 * after having justified the string
 */
	Uint32 *pdx = out;	// Point at the intout array
	Sint32 just_width = 0;	// Init the width to 0
	if (!pdx) return 0;

	for (n = 0; n < txtlen; n++)
	{
		glyph = set_fnthdr(si[n]) - m_FONT_INF.first_ade;
		if (si[n] == ' ') ++spaces;	
		di = &m_FONT_INF.off_table[ glyph ];
		width = (di[1] - di[0]);	// Char width 
//11/6/86 DH
//added to horizontal offset calculation
		if (m_FONT_INF.flags & HORZ_OFF)
		{
			di8 = &m_FONT_INF.hor_table [ glyph ];
			width -= di8[0];
			width -= di8[1];
		}
		just_width += width;
		*pdx++ = width;
	}
	// Get the width of a space char.
	glyph = set_fnthdr(' ') - m_FONT_INF.first_ade;			
	di = &m_FONT_INF.off_table[ glyph ];
	spacew = di[1] - di[0];
	if (m_FONT_INF.flags & HORZ_OFF)
	{
		di8 = &m_FONT_INF.hor_table [ glyph ];
		spacew -= di8[0];
		spacew -= di8[1];
	}
d_justif_calc_spcinq_nohorz:
	width = just_width;
	if (m_SPECIAL & SCALE)
	{
		if (m_DDA_INC == 1.0)
		{
			width *= 2;
			spacew *= 2;
			for (n = 0; n < txtlen; n++)
			{
				out[n] *= 2;
			}
		}
		else
		{
			// This isn't the way GEM does it; it inlines ACT_SIZ.
			width = ACT_SIZ(width);
			spacew = ACT_SIZ(spacew);
			for (n = 0; n < txtlen; n++)
			{
				out[n] *= ACT_SIZ(out[n]);
			}
		}
	}
	if (m_SPECIAL & THICKEN)
	{
		width += m_WEIGHT * txtlen;
		for (n = 0; n < txtlen; n++)
		{
			out[n] += m_WEIGHT;
		}
	}
	m_width = pt[1].x;	/* I notice no X-Y scaling this time */
	sign = 0;
	bx = 1;
	ax = m_width - width;
	if (ax >= 0)
	{
		minsp = 1000;
	}
	else
	{
		sign = 1;
		bx = -1;
		ax = -ax;
		minsp = (spacew / 2);
	}
d_justif_calc_inq_calc_intword_pos:
	m_JUST_NEG = sign;
	m_JUST_DEL_SIGN = bx;
	dx = bx = 0;
	m_JUST_DEL_WORD = 0;
	m_JUST_DEL_WORD_REM = 0;
	m_JUST_DEL_CHAR = 0;
	m_JUST_DEL_CHAR_REM = 0;
	if ((word_space & 1) && spaces)
	{
		q = (ax / spaces);	// Pels per space
		r = (ax % spaces);	// Remainder
		if (q >= minsp)
		{
			m_JUST_DEL_WORD = minsp;
		}
		else
		{
			m_JUST_DEL_WORD = q;
			m_JUST_DEL_WORD_REM = r;
			goto d_justif_calc_inq_calc_exit;
		}
	}
	// inter character only is allowed
 // width     = ax
 // dir       = si
 // numspaces = cx
 // max space = di
	if (char_space & 1)
	{
		ax -= (minsp * spaces);
		m_JUST_DEL_CHAR     = ax / txtlen;
		m_JUST_DEL_CHAR_REM = ax % txtlen;
	}
d_justif_calc_inq_calc_exit:
	if (sign)
	{
		m_JUST_DEL_WORD = -m_JUST_DEL_WORD;
		m_JUST_DEL_CHAR = -m_JUST_DEL_CHAR;
	}
	wchar_t *pbx = text;
	dx = m_JUST_DEL_WORD;
	Sint32 rem = m_JUST_DEL_WORD_REM;
	for (n = 0; n < txtlen; n++)
	{
		if (text[n] == ' ')
		{
			out[n] += dx;
			if (rem) 
			{
				out[n] += m_JUST_DEL_SIGN;
				--rem;
			}
		}
		else if (m_JUST_DEL_CHAR || m_JUST_DEL_CHAR_REM)
		{
			out[n] += m_JUST_DEL_CHAR;
			if (m_JUST_DEL_CHAR_REM)
			{
				out[n] += m_JUST_DEL_SIGN;
				--m_JUST_DEL_CHAR_REM;
			}
		}
	}
	if ((char_space & 0xfffe) == 0x8000) return 0;
	return m_width;
}
#if 0
;****************************************************************
; dqt_justified - inquire offsets for justified text		*
;****************************************************************

dqt_just:
	push	bp
	call	chk_fnt			;make sure font is in memory
	call	dq_justif_calc		;set up data for justified text
	or	ax, ax
	jnz	dqt_justif_work
dqt_justif_exit:
	pop	bp
	ret

dqt_justif_work:
	mov	FLIP_Y, 1		;we return magnitudes
	mov	si, offset CONTRL	;saves code
	mov	cx, 6[si]		;get the intin count
	mov	4[si], cx		;this is the vertex count
	push	cx			;save for after clear
	shl	cx, 1			;make a word count
	mov	di, offset PTSOUT	;zero out the ptsout array
	push	ds
	pop	es
	push	di
	xor	ax, ax
	rep	stosw
	pop	di			;restore pointer to output array
	pop	cx			;restore character count

	mov	si, offset INTIN+4	;point at the string

	push ax				; jn 11-3-87
	mov ax, [si]
	call set_fnthdr
	mov [si], ax			; jn 11-21-87
	pop ax

	les	bp, OFF_TBL		;point at the offset table

	mov	dx, FIR_CHR		;bias pointer by low ade
	shl	dx, 1
	sub	bp, dx
	xor	dx, dx			;width starts at zero

	mov	ax, rot_case		;determine starting address
	test	ax, 1			;in ptsin array
	jz	dqt_extent_in_x
	inc	di			;start at PTSIN[1]
	inc	di

dqt_extent_in_x:
	or	ax, ax			;jump to correct math op
	jz	dqt_add_loop
	cmp	ax, 3
	jz	dqt_add_loop

dqt_sub_loop:
	call	dqt_calc
	sub	dx, bx
	mov	[di], dx
	add	di, 4
	loop	dqt_sub_loop

	jmps	dqt_justif_exit

dqt_add_loop:
	call	dqt_calc
	add	dx, bx
	mov	[di], dx
	add	di, 4
	loop	dqt_add_loop

	jmps	dqt_justif_exit

dqt_calc:
	push	bp
	lodsw				;get the char

	push dx				; jn 11-2-87
	call set_fnthdr			; jn 11-21-87, let ax change
	les bp, off_tbl
	mov dx, fir_chr
	shl dx, 1
	sub bp, dx
	pop dx

	mov	bx, ax			;index into offset table
	shl	bx, 1			;x values stored as words
	add	bp, bx
	mov	bx, es:W_1[bp]
	sub	bx, es:[bp]
	pop	bp


; jn 1-5-88
; Horz offset code added
;
; register state:
;	ax = character in question
;	bx = width of char

	push ax				; save all regs used
	push dx
	push si

	mov si, off_htbl		; es:si = horz offset table addr
	mov dx, fir_chr			; dx = ADE of first char in font seg
	shl dx, 1
	shl ax, 1
	sub ax, dx			; ax = byte offset chars offset info
	add si, ax			; si = offset of chars horz off info
	mov ax, es:[si]			; ah = right offset, al = left offset
	add al, ah			; al = total offset
	xor ah, ah			; ax = total offset
	sub bx, ax			; bx = horz offset compensated width
	
	pop si				; restore used regs
	pop dx
	pop ax



	test	SPECIAL, SCALE
	jz	dqt_nodouble
	push	ax
	push	cx
	push	dx
	push	bx
	call	ACT_SIZ
	pop	bx
	mov	bx, ax
	pop	dx
	pop	cx
	pop	ax

dqt_nodouble:
	test	SPECIAL, THICKEN
	jz	dqt_interword
	add	bx, WEIGHT

dqt_interword:
	cmp	al, 20h			;is this a space char?
	jne	dqt_interchar
	add	bx, JUST_DEL_WORD	; yes:  add in the word offset
	cmp	JUST_DEL_WORD_REM, 0	; need to add remainder?
	je	dqt_interchar
	add	bx, JUST_DEL_SIGN	; add in remainder
	dec	JUST_DEL_WORD_REM	; account for remainder
	jmps	dqt_calc_exit

dqt_interchar:
	add	bx, JUST_DEL_CHAR	; add in character offset
	cmp	JUST_DEL_CHAR_REM, 0	; need to add remainder?
	je	dqt_calc_exit
	add	bx, JUST_DEL_SIGN	; add in remainder
	dec	JUST_DEL_CHAR_REM	; account for remainder
dqt_calc_exit:
	ret
;****************************************************************
; dq_justif_calc - common calculator for justified text		*
;****************************************************************

dq_justif_calc:
	push	bp
	xor	bp, bp				;init the width counter
	mov	si, offset CONTRL		;saves code
	mov	4[si], bp			;set the intout to 0;
	mov	cx, 6[si]			;get the intin count
	dec	cx
	dec	cx				;are there any chars?
	mov	6[si], cx			;put back the correct width
	jz	dq_justif_skip_it
	mov	si, offset INTIN + 4		;point at the string

	push ax				; jn 11-3-87
	mov ax, [si]
	call set_fnthdr
	mov [si], ax			; jn 11-21-87
	pop ax

	les	di, OFF_TBL			;point at the offset table
	xor	ch, ch				;clear out the space counter

	mov	dx, FIR_CHR			;bias pointer by low ade
	shl	dx, 1
	sub	di, dx

dq_justif_width_loop:
	lodsw					;get the char

	push dx
	call set_fnthdr			; jn 11-21-87, let ax change
	les di, ss:off_tbl
	mov dx, ss:fir_chr
	shl dx, 1
	sub di, dx
	pop dx

	cmp	al, 20h				;is this a space char?
	jnz	dq_justif_width_notspace
	inc	ch				;found a space

dq_justif_width_notspace:
	shl	ax, 1				;index into offset table
	mov	bx, ax				;x values stored as words
	add	bp, es:W_1[bx+di]
	sub	bp, es:[bx+di]
	dec	cl
	jnz	dq_justif_width_loop

	push ax				; jn 11-3-87
	mov ax, 20h
	call set_fnthdr
	pop ax

	mov	bx, 20h				;get the width of a space char
	shl	bx, 1
	add	bx, di
	mov	di, es:W_1[bx]
	sub	di, es:[bx]

;width of string with no attributes
;width = bp
;space width = di
;space count = ch

	test	SPECIAL, SCALE
	jz	dq_justif_width_nodouble
	jmps	dq_justif_width_scale

dq_justif_skip_it:
	pop	bp
	xor	ax, ax				;return FALSE
	ret

dq_justif_width_scale:
	push	cx				;save the space count
	push	bp
	call	ACT_SIZ
	pop	bp
	mov	bp, ax
	push	di
	call	ACT_SIZ
	pop	di
	mov	di, ax
	pop	cx

dq_justif_width_nodouble:
	test	SPECIAL, THICKEN
	jz	dq_justif_calc_intword
	mov	ax, CONTRL+6
	mov	dx, WEIGHT
	mul	dx				;ax = the additional thicken
	add	bp, ax

dq_justif_calc_intword:
	mov	ax, PTSIN+4			;get width of desired string
	cmp	ax, 0				; must be positive
	jle	dq_justif_skip_it
	test	SPECIAL, ROTODD			; if 90 or 270 degrees,
	jz	dq_justif_save_width		;   must scale width
	mov	bx, xsize
	mul	bx
	mov	bx, ysize
	div	bx
	shl	dx, 1				; rounding
	cmp	dx, bx
	jl	dq_justif_save_width
	inc	ax

dq_justif_save_width:
	mov	width, ax
	xor	si, si
	mov	bx, 1
	sub	ax, bp				;get the delta to the width
	js	dq_justif_calc_intword_neg
	shl	di, 1				;if positive max space = 2X
	jmps	dq_justif_calc_intword_pos

dq_justif_calc_intword_neg:
	neg	bx
	neg	ax
	inc	si				;set the negative flag
	shr	di, 1				;set max space = 1/2

dq_justif_calc_intword_pos:
	mov	JUST_NEG, si
	mov	JUST_DEL_SIGN, bx
	mov	cl, ch
	xor	ch, ch
	xor	dx, dx
	mov	JUST_DEL_WORD, dx
	mov	JUST_DEL_WORD_REM, dx
	mov	JUST_DEL_CHAR, dx
	mov	JUST_DEL_CHAR_REM, dx
	test	INTIN, 0ffffh			;is inter word justify allowed
	jz	dq_justif_intchar_only
	and	cl, cl				;are there any spaces in string
	jz	dq_justif_calc_intchar
	mov	bx, ax				;save the width
	div	cx				;get the pels per space and
						;remainder of pels per space
	cmp	ax, di
	jle	dq_justif_calc_intword_done
	mov	ax, bx				;set the extra space = max
	mov	JUST_DEL_WORD, di
	jmps	dq_justif_calc_intchar

dq_justif_calc_intword_done:
	mov	JUST_DEL_WORD, ax
	mov	JUST_DEL_WORD_REM, dx
	jmps	dq_justif_calc_exit

;width     = ax
;dir       = si
;numspaces = cx
;max space = di

dq_justif_intchar_only:				;inter character only
	xor	cl, cl				;no contribution from words

dq_justif_calc_intchar:				;inter character adjustment
	test	INTIN+2, 0ffffh			;is inter char allowed?
	jz	dq_justif_calc_exit
	xchg	ax, di
	mul	cx				;get space taken up by words
	sub	di, ax

dq_justif_calc_intchar_rem:
	mov	ax, di
	mov	bx, CONTRL+6
	div	bx				;get the additional char space
	mov	JUST_DEL_CHAR, ax
	mov	JUST_DEL_CHAR_REM, dx

dq_justif_calc_exit:
	and	si, si
	jz	dq_justif_calc_exit_pos
	neg	JUST_DEL_CHAR
	neg	JUST_DEL_WORD

dq_justif_calc_exit_pos:
	pop	bp
	mov	ax, 0FFFFh
	ret

#endif

/****************************************************************
* CLR_SKEW							*
*****************************************************************/

void SdSdl::clr_skew()
{
	Uint32 pushed_fgbp   = m_FG_BP_1;
	Uint32 pushed_lnmask = m_LN_MASK;

	if (!m_WRT_MODE) m_FG_BP_1 = m_globals->MAP_COL(0);

// Set the rotation case index:  0 for 0 degrees, 2 for 90 degrees, 4 for
// 180 degrees, and 6 for 270 degrees.  Prepare for the output loop.

	m_LN_MASK = 0xFFFFFFFF;
	
	Uint32 rci = (m_SPECIAL & ROTATE) >> 6;
	Uint32 slant  = 0x55555555;

// Top of the line output loop.
	for (Sint32 cx = 0; cx < m_ACTDELY; cx++)
	{
		// Save (X1,Y1) and (X2,Y2) which are used by this call.
		Uint32 save_x1 = m_X1, save_x2 = m_X2, save_y1 = m_Y1, save_y2 = m_Y2;
		RECTFILL();
		m_X1 = save_x1; m_X2 = save_x2; m_Y1 = save_y1; m_Y2 = save_y2;

// Location of the next line depends on the rotation angle.

		switch(rci)
		{
			case 3: ++m_X1; ++m_X2; break;	// Right 
			case 2: ++m_Y1; ++m_Y2; break;	// Down
			case 1: --m_X1; --m_X2; break;	// Left
			case 0: --m_Y1; --m_Y2; break;  // Up
		}
		if (slant & 0x80000000L)
		{
			slant <<= 1;
			slant |= 1;
// And move it sideways a bit
			switch(rci)
			{
				case 3: ++m_Y1; ++m_Y2; break;
				case 2: --m_X1; --m_X2; break;
				case 1: --m_Y1; --m_Y2; break;
				case 0: ++m_X1; ++m_X2; break;
			}
		} 
		else slant <<= 1;
	} // end for
	m_LN_MASK = pushed_lnmask;
	m_FG_BP_1 = pushed_fgbp;
}
