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

#if 0
		cseg
		public	set_fnthdr
		public	chk_ade		; jn 10-27-87
		public	chk_fnt
		public	inc_lfu
		public	CLC_DDA
		public	ACT_SIZ
		public	in_rot
		public	in_doub
		public	cpy_head
		public	txtblt_rep_rr_0	
		public	txtblt_rep_rr_s	
		public	txtblt_tran_rr_0
		public	txtblt_tran_rr_s
		public	txtblt_xor_rr_s
		public	txtblt_itran_rr_0
		public	txtblt_itran_rr_s
		public	txtblt_rep_rl_0
		public	txtblt_rep_rl_s
		public	txtblt_tran_rl_0
		public	txtblt_tran_rl_s
		public	txtblt_xor_rl_s
		public	txtblt_itran_rl_0
		public	txtblt_itran_rl_s

if mono_multisegs
		extrn	graph_seg_high:word
endif
if mono_port
		extrn	next_seg_pgdown:near
endif
set_flag	rb	1

;
;these two tables are initialized to rotate and mask the data 
;
;
		dseg
if mono_mem
		extrn	current_bank:byte
endif
		extrn	first:dword
		extrn	cur_font:dword
		extrn	act_font:dword
		extrn	cur_head:dword
		extrn	T_SCLSTS:word
		extrn	DDA_INC:word
		extrn	fi_flags:word
		extrn	FOFF:word
		extrn	poff_tbl:word
		extrn	seg_htbl:word
if ( num_planes gt 1 ) and ( not segment_access )
		extrn	plane_port_tbl:byte
		extrn	plane_read_tbl:byte
ndif
ftmgradd	rd	0	
ftmgroff	dw	0		;offset of font mgr call
ftmgrseg	dw	0		;segment of font mgr call
		public	ftmgroff
		public	ftmgrseg
		cseg
if wy700
		extrn	current_port:byte	;wy700 control port value
endif
;*******************************************
;txtblt_rep_rr_s
;	replace mode rotate right color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
txtblt_rep_rr_s_doright:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
endif
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dh				;apply the right mask
	xor	al, ah				;xor in the source data
	stosb
	add	si, bp				;add the souce width in
if wy700
	call	dest_add2
else
	add	di, next_line-2			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rr_s_nomid_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rr_s_nomid_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rr_s_nomid_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rr_s_nomid_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_nomid_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rr_s_nomid_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rr_s_nomid_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rr_s_nomid_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_nomid_end
	add	di, move_to_first
endif
txtblt_rep_rr_s_nomid_end:
	loop	txtblt_rep_rr_s_nomid_loop
	ret
;
;this code draws a character with left + right mask or left mask only
;
txtblt_rep_rr_s_nomid:
	cmp	dh, 0ffh
	jz	txtblt_rep_rr_s_nomid_loop
	dec	bp				;make the add -1 for speed
txtblt_rep_rr_s_nomid_loop:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dl				;apply the left mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte

	cmp	dh, 0ffh			; left only?
	jne	txtblt_rep_rr_s_doright
	add	si, bp				;add the souce width in
if wy700
	call	dest_add1
else
	add	di, next_line-1			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rr_s_noright_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rr_s_noright_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rr_s_noright_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rr_s_noright_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_noright_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rr_s_noright_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rr_s_noright_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rr_s_noright_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_noright_end
	add	di, move_to_first
endif
txtblt_rep_rr_s_noright_end:
	loop	txtblt_rep_rr_s_nomid_loop
	ret
;
;this code is for middle + left + right
;
txtblt_rep_rr_s:
	mov	bx, ax
	and	ch, ch				;is this a no middle one
if mono_mem
	jnz	txtblt_rep_rr_s_wide
	jmp	txtblt_rep_rr_s_nomid
else
	jz	txtblt_rep_rr_s_nomid
endif
txtblt_rep_rr_s_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dl				;apply the left mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	mov	ah, ch				;ah will be the middle count
txtblt_rep_rr_s_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
endif
	stosb	
	dec	ah
	jnz	txtblt_rep_rr_s_wide_bloop
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
endif
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dh				;apply the right mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rr_s_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rr_s_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rr_s_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rr_s_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rr_s_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rr_s_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rr_s_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rr_s_wide_end
	add	di, move_to_first
endif
txtblt_rep_rr_s_wide_end:
	dec	cl
	jnz	txtblt_rep_rr_s_wide
	ret
;*******************************************
;txtblt_rep_rl_s
;	replace mode rotate left color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;this code is for left + right or left only
txtblt_rep_rl_s_doright:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
endif
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dh				;apply the right mask
	xor	al, ah				;xor in the source data
	mov	es:[di], al			;apply the byte
	add	si, bp				;add the souce width in
if wy700
	call	dest_add1
else
	add	di, next_line-1			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rl_s_nomid_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rl_s_nomid_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rl_s_nomid_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rl_s_nomid_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_nomid_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rl_s_nomid_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rl_s_nomid_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rl_s_nomid_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_nomid_end
	add	di, move_to_first
endif
txtblt_rep_rl_s_nomid_end:
	loop	txtblt_rep_rl_s_nomid_loop
	ret
txtblt_rep_rl_nomid:
	dec	bp
	cmp	dh, 0ffh
	je	txtblt_rep_rl_s_nomid_loop
	dec	bp				;set up the next line counter
txtblt_rep_rl_s_nomid_loop:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
else
	or	al, cs:txtblt_rrot_table_1[bx]
endif	
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dl				;apply the left mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte

	cmp	dh, 0ffh
	jne	txtblt_rep_rl_s_doright
	add	si, bp				;add the souce width in
if wy700
	call	dest_add1
else
	add	di, next_line-1			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rl_s_noright_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rl_s_noright_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rl_s_noright_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rl_s_noright_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_noright_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rl_s_noright_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rl_s_noright_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rl_s_noright_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_noright_end
	add	di, move_to_first
endif
txtblt_rep_rl_s_noright_end:
	loop	txtblt_rep_rl_s_nomid_loop
	ret
;this code is for left + middle + right
txtblt_rep_rl_s:
	mov	bx, ax
	and	ch, ch
if mono_mem
	jnz	txtblt_rep_rl_s_wide
	jmp	txtblt_rep_rl_nomid
else
	jz	txtblt_rep_rl_nomid
endif
txtblt_rep_rl_s_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
else
	or	al, cs:txtblt_rrot_table_1[bx]
endif	
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dl				;apply the left mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	mov	ah, ch				;ah will be the middle count
	and	ah, ah
	jz	txtblt_rep_rl_s_wide_right
txtblt_rep_rl_s_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
endif
	stosb	
	dec	ah
	jnz	txtblt_rep_rl_s_wide_bloop
txtblt_rep_rl_s_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
endif
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dh				;apply the right mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rl_s_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rl_s_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rl_s_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rl_s_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rl_s_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rl_s_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_rep_rl_s_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_rep_rl_s_wide_end
	add	di, move_to_first
endif
txtblt_rep_rl_s_wide_end:
	dec	cl
	jnz	txtblt_rep_rl_s_wide
	ret
;*******************************************
;txtblt_rep_rl_0
;txtblt_rep_rr_0
;	replace mode color ix = 0
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
txtblt_rep_rl_0:
txtblt_rep_rr_0:
	mov	bx, ax
	xor	bl, bl
	mov	bp, next_line
if rev_vid
	not	bl
endif
if multiseg or mono_port or mono_mem
	mov	si, plane_size
endif
txtblt_rep_rr_0_wide:
	push	di
	mov	al, bl
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dl				;apply the left mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	push	cx
	mov	cl, ch
	xor	ch, ch
	mov	al, bl
	jcxz	txtblt_rep_rr_0_right
	rep	stosb				;move out all the bytes
txtblt_rep_rr_0_right:
	pop	cx
	mov	ah, es:[di]			;get the dest byte
	xor	ah, al				;xor in the source data
	and	ah, dh				;apply the right mask
	xor	al, ah				;xor in the source data
	stosb					;apply the byte
	pop	di
if wy700
	call	dest_add
else
	add	di, bp				;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_rep_rr_0_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_rep_rr_0_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_rep_rr_0_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_rep_rr_0_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, si
	jc	txtblt_rep_rr_0_end
	sub	di, si
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_rep_rr_0_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_rep_rr_0_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, si				;have we wrapped past the end?
	jc	txtblt_rep_rr_0_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, si
	jc	txtblt_rep_rr_0_end
	add	di, move_to_first
endif
txtblt_rep_rr_0_end:
	dec	cl
	jnz	txtblt_rep_rr_0_wide
	ret
;
;*******************************************
;txtblt_tran_rr_s
;	transparent mode rotate right color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;
;
;this code draws a character with left fringe only
;
txtblt_tran_rr_s_noright:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
if rev_vid
	or	al, dl				;mask off bits
	and	es:[di], al
else
	and	al, dl
	or	es:[di], al
endif
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rr_s_noright_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rr_s_noright_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rr_s_noright_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rr_s_noright_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_noright_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rr_s_noright_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rr_s_noright_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rr_s_noright_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_noright_end
	add	di, move_to_first
endif
txtblt_tran_rr_s_noright_end:
	loop	txtblt_tran_rr_s_noright
	ret
;
;this code draws a character with left + right mask only
;
txtblt_tran_rr_s_nomid:
	xor	ch, ch
	cmp	dh, 0ffh
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif
	jz	txtblt_tran_rr_s_noright
	dec	bp				;make the add -1 for speed
txtblt_tran_rr_s_nomid_loop:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
if rev_vid
	or	al, dl				;mask off bits
	and	es:[di], al
else
	and	al, dl
	or	es:[di], al
endif
	inc	di
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	or	al,dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	and	al,dh
	or	es:[di], al
endif
	add	si, bp				;add the souce width in
if wy700
	call	dest_add1
else
	add	di, next_line-1			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rr_s_nomid_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rr_s_nomid_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rr_s_nomid_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rr_s_nomid_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_nomid_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rr_s_nomid_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rr_s_nomid_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rr_s_nomid_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_nomid_end
	add	di, move_to_first
endif
txtblt_tran_rr_s_nomid_end:
	loop	txtblt_tran_rr_s_nomid_loop
	ret
;
txtblt_tran_rr_s:
	mov	bx, ax
	and	ch, ch				;is there any middle
	jz	txtblt_tran_rr_s_nomid
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif
txtblt_tran_rr_s_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
if rev_vid
	or	al, dl				;mask off bits
	and	es:[di], al
else
	and	al, dl
	or	es:[di], al
endif
	inc	di
	mov	ah, ch				;ah will be the middle count
txtblt_tran_rr_s_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	or	es:[di], al
endif
	inc	di	
	dec	ah
	jnz	txtblt_tran_rr_s_wide_bloop
txtblt_tran_rr_s_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	or	al, dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	and	al, dh
	or	es:[di], al
endif
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rr_s_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rr_s_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rr_s_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rr_s_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rr_s_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rr_s_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rr_s_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rr_s_wide_end
	add	di, move_to_first
endif
txtblt_tran_rr_s_wide_end:
	dec	cl
	jnz	txtblt_tran_rr_s_wide
	ret
;*******************************************
;txtblt_tran_rl_s
;	transparent mode rotate left color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
txtblt_tran_rl_s_noright:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	or	al, dl
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	and	al, dl
	or	es:[di], al
endif	
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rl_s_noright_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rl_s_noright_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rl_s_noright_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rl_s_noright_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_noright_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rl_s_noright_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rl_s_noright_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rl_s_noright_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_noright_end
	add	di, move_to_first
endif
txtblt_tran_rl_s_noright_end:
	loop	txtblt_tran_rl_s_noright
	ret
;this code is for left + right
txtblt_tran_rl_s_nomid:
	dec	bp
	xor	ch, ch
	cmp	dh, 0ffh
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif
	jz	txtblt_tran_rl_s_noright
	dec	bp				;set up the next line counter
txtblt_tran_rl_s_nomid_loop:
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	or	al, dl
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	and	al, dl
	or	es:[di], al
endif	
	inc	di
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	or	al, dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	and	al, dh
	or	es:[di], al
endif	
	add	si, bp				;add the source width in
if wy700
	call	dest_add1
else
	add	di, next_line-1			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rl_s_nomid_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rl_s_nomid_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rl_s_nomid_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rl_s_nomid_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_nomid_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rl_s_nomid_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rl_s_nomid_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rl_s_nomid_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_nomid_end
	add	di, move_to_first
endif
txtblt_tran_rl_s_nomid_end:
	loop	txtblt_tran_rl_s_nomid_loop
	ret

;
txtblt_tran_rl_s:
	mov	bx, ax
	and	ch, ch
	jz	txtblt_tran_rl_s_nomid
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif
txtblt_tran_rl_s_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	or	al, dl
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	and	al, dl
	or	es:[di], al
endif	
	inc	di				;apply the byte
	mov	ah, ch				;ah will be the middle count
txtblt_tran_rl_s_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	or	es:[di], al
endif
	inc	di
	dec	ah
	jnz	txtblt_tran_rl_s_wide_bloop
txtblt_tran_rl_s_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	or	al, dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	and	al, dh
	or	es:[di], al
endif
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rl_s_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rl_s_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rl_s_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rl_s_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rl_s_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rl_s_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rl_s_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rl_s_wide_end
	add	di, move_to_first
endif
txtblt_tran_rl_s_wide_end:
	dec	cl
	jnz	txtblt_tran_rl_s_wide
	ret
;
;*******************************************
;txtblt_tran_rr_0
;	transparent mode rotate right color ix = 0
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;
txtblt_tran_rr_0:
	mov	bx, ax
if rev_vid 
	not	dx				;invert the mask if not revvid
endif
txtblt_tran_rr_0_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
if rev_vid
	and	al, dl				;mask off bits
	or	es:[di], al
else
	or	al, dl
	and	es:[di], al
endif
	inc	di
	and	ch, ch
	jz	txtblt_tran_rr_0_wide_right
	mov	ah, ch				;ah will be the middle count
txtblt_tran_rr_0_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch the first byte from nxt src
	not	al
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch the first byte from nxt src
	not	al
	and	es:[di], al
endif
	inc	di	
	dec	ah
	jnz	txtblt_tran_rr_0_wide_bloop
txtblt_tran_rr_0_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	and	al, dh
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	or	al, dh
	and	es:[di], al
endif
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rr_0_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rr_0_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rr_0_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rr_0_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rr_0_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rr_0_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rr_0_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rr_0_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rr_0_wide_end
	add	di, move_to_first
endif
txtblt_tran_rr_0_wide_end:
	dec	cl
	jnz	txtblt_tran_rr_0_wide
	ret
;*******************************************
;txtblt_tran_rl_0
;	transparent mode rotate left color ix = 0
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;
txtblt_tran_rl_0:
	mov	bx, ax
if rev_vid 
	not	dx				;invert the mask if not revvid
endif
txtblt_tran_rl_0_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	not	al
	and	al, dl
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	not	al
	or	al, dl
	and	es:[di], al
endif	
	inc	di				;apply the byte
	and	ch, ch
	jz	txtblt_tran_rl_0_wide_right
	mov	ah, ch				;ah will be the middle count
txtblt_tran_rl_0_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch the second byte from src
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	and	es:[di], al
endif
	inc	di
	dec	ah
	jnz	txtblt_tran_rl_0_wide_bloop
txtblt_tran_rl_0_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	and	al, dh
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	or	al, dh
	and	es:[di], al
endif
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_tran_rl_0_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_tran_rl_0_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_tran_rl_0_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_tran_rl_0_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_tran_rl_0_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_tran_rl_0_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_tran_rl_0_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_tran_rl_0_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_tran_rl_0_wide_end
	add	di, move_to_first
endif
txtblt_tran_rl_0_wide_end:
	dec	cl
	jnz	txtblt_tran_rl_0_wide
	ret
;
#endif
/*******************************************
*txtblt_xor_rr_s
*txtblt_xor_rr_0
*	xor mode rotate right color ix = 0/1
*
*	Entry
*		es:di = dest pointer ( screen )
*		ds:si = source pointer ( memory )
*		ah    = rotate count
*		dl    = left mask
*		dh    = right mask
*		ch    = middle byte count
*		cl    = vertical scan count
*		bp    = source form width
*********************************************/
//				cl                 ah           ch
void SdSdl::txtblt_xor_rr_s(int vertical_scan, int rcount, int bytes_w,
//				bp                 dl           dh
			    int source_w, Uint8 lmask, Uint8 rmask,
//				si		   di
                            Uint8 **src_bits, Uint8 *dest_bits) 
{
	Uint32 ah, cl;
	Uint32 bh = (rcount << 8);
	Uint8 al, bl;

	lmask = ~lmask;
	rmask = ~rmask;
txtblt_xor_rr_s_wide:
	for (cl = 0; cl < vertical_scan; ++cl)
	{
		Uint8 *psi = *src_bits;
		Uint8 *pdi = dest_bits;

		bl = *psi;
		al = m_globals->m_txtblt_rrot_table_1[bh | bl];
		al &= lmask;
		*pdi ^= al;
		++pdi;
		for (ah = 0; ah < bytes_w; ah++)
		{
			al = m_globals->m_txtblt_rrot_table_2[bh | bl];
			++psi;
			bl = *psi;
			al |= m_globals->m_txtblt_rrot_table_1[bh | bl];
			*pdi ^= al;
			++pdi;
		}
		al = m_globals->m_txtblt_rrot_table_2[bh | bl];
		++psi;
		bl = *psi;
		al |= m_globals->m_txtblt_rrot_table_1[bh | bl];
		al &= rmask;
		*pdi ^= al;
		*src_bits += source_w;
		dest_bits += m_globals->m_bytes_line;
	}
}


/*******************************************
*txtblt_xor_rl_s
*txtblt_xor_rl_0
*	xor mode rotate left color ix = 0/1
*
*	Entry
*		es:di = dest pointer ( screen )
*		ds:si = source pointer ( memory )
*		ah    = rotate count
*		dl    = left mask
*		dh    = right mask
*		ch    = middle byte count
*		cl    = vertical scan count
*		bp    = source form width
*********************************************/
//				cl		    ah         ch 
void SdSdl::txtblt_xor_rl_s(int vertical_scan, int rcount, int bytes_w,
			    int source_w, Uint8 lmask, Uint8 rmask,
                            Uint8 **src_bits, Uint8 *dest_bits) 
{
	Uint32 ah, cl;
	Uint32 bh = (rcount << 8);
	Uint8 al, bl;

	lmask = ~lmask;
	rmask = ~rmask;
txtblt_xor_rl_s_wide:
	for (cl = 0; cl < vertical_scan; ++cl)
	{
		Uint8 *psi = *src_bits;
		Uint8 *pdi = dest_bits;

		bl = bh | *psi;
		al = m_globals->m_txtblt_rrot_table_2[bh | bl];
		++psi;
		bl = *psi;
		al |= m_globals->m_txtblt_rrot_table_1[bh | bl];
		al &= lmask;
		*pdi ^= al;
		++pdi;
		for (ah = 0; ah < bytes_w; ah++)
		{
			al = m_globals->m_txtblt_rrot_table_2[bh | bl];
			++psi;
			bl = *psi;
			al |= m_globals->m_txtblt_rrot_table_1[bh | bl];
			*pdi ^= al;
			++pdi;
		}
		al = m_globals->m_txtblt_rrot_table_2[bh | bl];
		++psi;
		bl = *psi;
		al |= m_globals->m_txtblt_rrot_table_1[bh | bl];
		al &= rmask;
		*pdi ^= al;
		*src_bits += source_w;
		dest_bits += m_globals->m_bytes_line;
	}
}

#if 0
;
;*******************************************
;txtblt_itran_rr_0
;	inverse transparent mode rotate right color ix = 0
;txtblt_itran_rr_s
;	inverse transparent mode rotate right color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;
txtblt_itran_rr_0:
	mov	set_flag, 0
if rev_vid 
	not	dx				;invert the mask if not revvid
endif
	jmps	txtblt_itran_rr_common

txtblt_itran_rr_s:
	mov	set_flag, 1
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif

txtblt_itran_rr_common:
	mov	bx, ax

txtblt_itran_rr_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	cmp	set_flag, 0
	jnz	txtblt_itran_rr_set_1
if rev_vid
	and	al, dl				;mask off bits
	or	es:[di], al
else
	or	al, dl
	and	es:[di], al
endif
	jmps	txtblt_itran_rr_end_1
txtblt_itran_rr_set_1:
	not	al
if rev_vid
	or	al, dl				;mask off bits
	and	es:[di], al
else
	and	al, dl
	or	es:[di], al
endif
txtblt_itran_rr_end_1:
	inc	di
	and	ch, ch
	jz	txtblt_itran_rr_wide_right
	mov	ah, ch				;ah will be the middle count
txtblt_itran_rr_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch second byte from src
	inc	si
	mov	bl, [si]
	cmp	set_flag, 0
	jnz	txtblt_itran_rr_set_2
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	and	es:[di], al
endif
	jmps	txtblt_itran_rr_end_2
txtblt_itran_rr_set_2:
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	or	es:[di], al
endif
txtblt_itran_rr_end_2:
	inc	di	
	dec	ah
	jnz	txtblt_itran_rr_wide_bloop
txtblt_itran_rr_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
	cmp	set_flag, 0
	jnz	txtblt_itran_rr_set_3
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	and	al, dh
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	or	al, dh
	and	es:[di], al
endif
	jmps	txtblt_itran_rr_end_3
txtblt_itran_rr_set_3:
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	or	al, dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	and	al, dh
	or	es:[di], al
endif
txtblt_itran_rr_end_3:
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_itran_rr_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_itran_rr_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_itran_rr_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_itran_rr_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_itran_rr_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_itran_rr_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_itran_rr_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_itran_rr_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_itran_rr_wide_end
	add	di, move_to_first
endif
txtblt_itran_rr_wide_end:
	dec	cl
	jz	end_txtblt_itran_rr
	jmp	txtblt_itran_rr_wide
end_txtblt_itran_rr:
	ret
;*******************************************
;txtblt_itran_rl_0
;	inverse transparent mode rotate left color ix = 0
;txtblt_itran_rl_s
;	inverse transparent mode rotate left color ix = 1
;
;	Entry
;		es:di = dest pointer ( screen )
;		ds:si = source pointer ( memory )
;		ah    = rotate count
;		dl    = left mask
;		dh    = right mask
;		ch    = middle byte count
;		cl    = vertical scan count
;		bp    = source form width
;********************************************	
;
txtblt_itran_rl_0:
	mov	set_flag, 0
if rev_vid 
	not	dx				;invert the mask if not revvid
endif
	jmps	txtblt_itran_rl_common

txtblt_itran_rl_s:
	mov	set_flag, 1
if rev_vid eq FALSE
	not	dx				;invert the mask if not revvid
endif

txtblt_itran_rl_common:
	mov	bx, ax
txtblt_itran_rl_wide:
	push	si
	push	di
	mov	bl, [si]			;get the source data
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
	cmp	set_flag, 0
	jnz	txtblt_itran_rl_set_1
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	and	al, dl
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	or	al, dl
	and	es:[di], al
endif	
	jmps	txtblt_itran_rl_end_1
txtblt_itran_rl_set_1:
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]
	not	al
	or	al, dl
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]
	not	al
	and	al, dl
	or	es:[di], al
endif	
txtblt_itran_rl_end_1:
	inc	di				;apply the byte
	and	ch, ch
	jz	txtblt_itran_rl_wide_right
	mov	ah, ch				;ah will be the middle count
txtblt_itran_rl_wide_bloop:
	mov	al, cs:txtblt_rrot_table_2[bx]	;fetch second byte from src
	inc	si
	mov	bl, [si]
	cmp	set_flag, 0
	jnz	txtblt_itran_rl_set_2
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	and	es:[di], al
endif
	jmps	txtblt_itran_rl_end_2
txtblt_itran_rl_set_2:
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;fetch first byte from nxt src
	not	al
	or	es:[di], al
endif
txtblt_itran_rl_end_2:
	inc	di
	dec	ah
	jnz	txtblt_itran_rl_wide_bloop
txtblt_itran_rl_wide_right:
	mov	al, cs:txtblt_rrot_table_2[bx]	;rotate and mask the source 
	inc	si
	mov	bl, [si]
	cmp	set_flag, 0
	jnz	txtblt_itran_rl_set_3
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	and	al, dh
	or	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	or	al, dh
	and	es:[di], al
endif
	jmps	txtblt_itran_rl_end_3
txtblt_itran_rl_set_3:
if rev_vid
	and	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	or	al, dh
	and	es:[di], al
else
	or	al, cs:txtblt_rrot_table_1[bx]	;rotate and mask the source 
	not	al
	and	al, dh
	or	es:[di], al
endif
txtblt_itran_rl_end_3:
	pop	di
	pop	si
	add	si, bp				;add the souce width in
if wy700
	call	dest_add
else
	add	di, next_line			;move to the next screen line
endif
if mono_xrxfp
	jnc	txtblt_itran_rl_wide_end
	mov	ax, es
	cmp	ax, graph_plane
	mov	ax, graph_plane_high
	jz	txtblt_itran_rl_wide_end1
	mov	ax, graph_plane
	add	di, bytes_line
txtblt_itran_rl_wide_end1:
	mov	es, ax
endif
if mono_multisegs
	jnc	txtblt_itran_rl_wide_end
	mov	es, graph_seg_high		;get the data from cs:	
endif
if mono_mem
	cmp	di, plane_size
	jc	txtblt_itran_rl_wide_end
	sub	di, plane_size
	mov	al, ss:current_bank
	inc	al
	cmp	al, 0c7h			;past last bank?
	jnz	txtblt_itran_rl_wide_end_mono
	mov	al, 0c0h
	add	di, bytes_line
txtblt_itran_rl_wide_end_mono:
	mov	ss:current_bank, al
	mov	es:.mono_mem_off, al
endif
if mono_port
	cmp	di, plane_size			;have we wrapped past the end?
	jc	txtblt_itran_rl_wide_end
	call	next_seg_pgdown
endif
if multiseg
	cmp	di, plane_size
	jc	txtblt_itran_rl_wide_end
	add	di, move_to_first
endif
txtblt_itran_rl_wide_end:
	dec	cl
	jz	end_txtblt_itran_rl
	jmp	txtblt_itran_rl_wide
end_txtblt_itran_rl:
	ret

#endif

/*************************
* in_doub
*	
*	initializes the double table
**************************/

void SdSdl::in_doub()
{
	Uint16 *di = m_globals->m_double_table;
	Uint16 ch16;
	Uint16 ch;
	Uint16 mask8, mask16;

	for (ch = 0; ch < 256; ++ch)
	{
		ch16 = 0;
		mask8 = 0x80;
		mask16 = 0xC000;
		while (mask8)
		{
			if (ch & mask8) ch16 |= mask16;	
			mask8 >>= 1;
			mask16 >>= 2;
		}
		*di++ = ch16;
	}
}


/*************************
* in_rot
*	initialize the two rotation tables
*	
**************************/

void SdSdl::in_rot()
{
	Uint8 *di = m_globals->m_txtblt_rrot_table_1;
        Uint8 *si = m_globals->m_txtblt_rrot_table_2;
	Uint8 ah = 0, al = 0;
	Uint8 bl;
	Uint8 ch = 8, cl = 0;
	Uint8 dh = 0, dl = 0xFF;	
	Uint16 bp;

	for (ch = 8; ch > 0; ch--)
	{
		for (bp = 0; bp < 256; bp++)
		{
			al = (ah >> cl);
			bl = (ah << (8 - cl));
			al &= dl;
			bl &= dh;
			*di++ = al;
			*si++ = bl;
			++ah;
		}	
		ah = 0;
		dl = dl >> 1;
		dh = ~dl;
		++cl;
	}
}

/*************************
; CLC_DDA
; entry
;	4[bp] = actual size
;	6[bp] = requested size
;
; exit
;	ax = dda_inc
;**************************/ 
double SdSdl::CLC_DDA(double actual, double requested)
{	
	if (requested <= actual)
	{
		m_T_SCLSTS = 0;
		if (!actual) actual = 1;
		return requested / actual;
	}
	m_T_SCLSTS = 1;
	requested -= actual;
	if (requested >= actual) return 1.0;
	
	return requested / actual;
}

/**************************
 ACT_SIZ
 entry
	4[bp] = size to scale
 exit
	ax = actual size
**************************/

Uint32 SdSdl::ACT_SIZ(Uint32 size)
{
	double bx;	
	Uint32 ax;

	if (m_DDA_INC == 1.0)  return 2 * size;
	if (!size) return 0;
	bx = 0.5;
	ax = 0;
	if (m_T_SCLSTS & 1)	/* Getting bigger */
	{
		while (size--)
		{
			bx += m_DDA_INC; 
			if (bx > 1.0) 
			{
				++ax;
				bx -= 1.0;
			}
			++ax;
		}
		return ax;
	}
	while (size--)
	{
		bx += m_DDA_INC;
		if (bx > 1.0) 	// Compare ACT_SIZ_SMALL_LOOP in opttdraw.a86
		{	
			++ax;
			bx -= 1.0;
		}
	}
	if (!ax) ++ax;
	return ax;
}


/*************************
*  chk_fnt
**************************/
void SdSdl::chk_fnt(void)
{	
	// Page in a font that's paged out (the header is loaded, but
	// not the data). Not currently used because under Unix, that's
	// what virtual memory is for :-)
}

wchar_t SdSdl::set_fnthdr(wchar_t ch)
/*
 *	Set the current font header.
 *
 *	Entry:	ax = char
 *
 *	Exit:	act_font = font header for this character
 *		cur_font = act_font
 *		font_inf = *cur_font
 */
{
	int r = chk_ade(ch);
	if (!r) return ch;
	if (r == 2)
	{
		r = chk_ade(' ');
		if (!r) return ' ';
	}
	chk_fnt();
	return ch;
}

int SdSdl::chk_ade(wchar_t ch)
/*
;
; jn 10-27-87
; This is the support for a segmented screen font.
;
; This routine must be called for every character.
;
; If the ADE value of the character passed in falls within
; the range of the characters in the currently selected font
; then we just return.
;
; If the ADE value of the character passed in not within the
; the range of the characters in the currently selected font
; the following is done:
;
;	1) The font chain traversed, starting with first, until
;	   the first font header segment for the currently selected
;	   font is found.
;
;	2) The font segments for the currently selected font are
;	   traversed until the font segment containing 
;	   the required ADE value is found.
;
;	3) act_font is set to the new font header.
;
;	Entry:	char passed in on stack
;
;	Exit:	act_font = the proper font header for the entry character 
;		cur_font = act_font
;		the new font header is copied into the current virtual
;		work station font info structure.
;		ax = 0 - if no change to the font_inf header
;		   = 1 - if there was a change.
;			 or if the font in the font header is not loaded.
;		   = 2 - if the character was out of range.
;
;	Note:	This routine preserves all registers used
;		except ax.  This is because is was added onto an
;		existing system.
;
;		act_font is relative to the current virtual work 
;		station
*/
{
	FontHead *si;

        // Does the ADE value of the character fall
        // within the range of the current font header ADE values?

	if (ch >= m_act_font->first_ade && ch <= m_act_font->last_ade)
	{
		// Skip the check for the font being paged out. It isn't.
		return 0;	
	} 

	// the character does not fall within the bounds of the
	// current font header.  find the correct header.

	si = find_font_seg0(m_act_font);
	si = find_ade_seg(ch, si);

	if (!si) return 2;	// Character not found 

	m_act_font = si;
	m_cur_font = si;
	cpy_head();
	return 1;	
}

FontHead *SdSdl::find_ade_seg(wchar_t ch, FontHead *si)
/*
;
;	Find the font header segment that contains the
;	ADE segment range
;
;	Entry:	ax = char
;		es:si = first header for this font style and point size
;
;	Exit:	es:si = header we want, or NULL if the character is not found.
*/
{
	while (si)
	{
		if (ch < si->first_ade) return NULL;
		if (ch <= si->last_ade) return si;

		si = si->next_sect;
	}
	return NULL;
}

FontHead *SdSdl::find_font_seg0(FontHead *si)
/*
;	Find the first segment of this font.
;
;	Entry:	es:si = act_font
;
;	Exit:	es:si = addr of first font header segment of the
;			desired font face and point size.
*/
{
	Uint32 id = si->font_id;    // ax
	Sint32 pt = si->point;  // bx

	si = &fntFirst;
	while (si->font_id != id && si->point != pt)
	{
		if (!si->next_font) return si;	// ERROR. We can't find the
						// font, but we know it was
						// there a moment ago.
		si = si->next_font;
	}
	return si;
}
