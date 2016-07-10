/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2005  John Elliott <jce@seasip.demon.co.uk>       *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cpm.h>
#include "crc16.h"
#include "auxd.h"

#define SOH 1
#define STX 2
#define ACK 6
#define NAK 21

/* PCW comms (acting, as far as possible, on the raw serial-port hardware) */

static unsigned char pkt_h[2];
static unsigned char pkt_t[2];

int read_bytes(unsigned char *c, unsigned count);
int write_bytes(unsigned char *c, unsigned count);
int sio_init();

unsigned char sio_errs;
unsigned char sio_reg5;

#asm
	psect	data
	global _sio_errs
	global _sio_reg5

	psect	code
	global	userf
	global	_sio_init
_sio_init:
	call	userf
	defw	00E3h	;CD VERSION
	ld	hl,1
	cp	1
	ret	nz	;Needs a PCW -- this is bit-banging stuff.

	call	userf
	defw	00BCh	;CD SA PARAMS
	xor	a	;No interrupt, no handshake
	ld	hl,808h	;8-bits TX and RX
	call	userf
	defw	00B6h	;CD SA INIT
	ld	a,68h	;TX enable | 8 TX bits
	ld	(_sio_reg5),a
	ld	a,7Eh	;Drop RTS
	call	userf
	defw	00B6h	;CD SA INIT
	xor	a
	ld	(_sio_errs),a
	ld	hl,0
	ret
;
	psect text

rts_up:	di
	push	af
	ld	a,5
	out	(c),a
	ld	a,(_sio_reg5)
	or	2
	ld	(_sio_reg5),a
	out	(c),a
	pop	af
	ei
	ret
;
rts_down:
	di
	push	af
	ld	a,5
	out	(c),a
	ld	a,(_sio_reg5)
	and	0FDh	
	ld	(_sio_reg5),a
	out	(c),a
	pop	af
	ei
	ret
;
; Read a byte from the Z80-DART.
;
sio_read:
	ld	a,1
	out	(c),a		;DART register 1
	in	a,(c)
	and	70h
	inc	a
	ld	hl,_sio_errs
	or	(hl)
	ld	(hl),a
	dec	c		;DART data
	in	a,(c)
	inc	c
	ret
;
sio_wstat:
	ld	c,0E1h
	ld	a,10h
	out	(c),a	;Command
	in	a,(c)
	and	20h		;Get CTS
	ret	z		;CTS is not set
	ld	a,1
	di
	out	(c),a
	in	a,(c)
	ei
	rra
	ret
;

	global	_read_bytes
	global	_write_bytes

_read_bytes:
	pop	de	;Return address
	pop	hl	;Address of bytes
	pop	bc	;Count
	push	bc
	push	hl
	push	de
;
; HL = address to load bytes
; BC = count of bytes to read
;
rblp:	push	hl
	push	bc
rbwait:	
	ld	c,6
	ld	e,0FFh
	call	5
	cp	3
	jr	z,rbbrk
;
; OK. We are now ready. 
;
	ld	c,0E1h		;First: Have we a character waiting already?
	in	a,(c)	
	rra
	jr	c,rdrdy
	call	rts_up
	in	a,(c)
	rra
	jr	nc,rbwait
rdrdy:	call	sio_read
	call	rts_down
	pop	bc
	pop	hl
	ld	(hl),a
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jr	nz,rblp
	ld	hl,0
	ret
;
rbbrk:	ld	c,0E1h
	call	rts_down
	ld	hl,-28
	pop	de
	pop	de
	ret

_write_bytes:
	pop	de	;Return address
	pop	hl	;Address of bytes
	pop	bc	;Count
	push	bc
	push	hl
	push	de
;
; HL = address of bytes
; BC = number of bytes to write
;
wblp:	push	hl
	push	bc
	ld	e,(hl)
	push	de
wbwait:	
	ld	c,6
	ld	e,0FFh
	call	5
	cp	3
	jr	z,wbbrk
	call	sio_wstat
	jr	nc,wbwait
	pop	de
	dec	c
	out	(c),e	;Write character
	pop	bc
	pop	hl
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jr	nz,wblp
	ld	hl,0
	ret
;
wbbrk:	ld	hl,-28
	pop	de
	pop	de
	pop	de
	ret
#endasm

int debug_read_bytes(unsigned char *c, unsigned count)
{
	int n;
	printf("read_bytes %x,%u\n", c, count); fflush(stdout);
	n = read_bytes(c, count);
	if (n) 
	{
		printf("err\n"); fflush(stdout);
		return n;
	}
	for (n = 0; n < count; n++)
	{
		printf("%02x ", c[n]);
	}
	putchar('\n');
	fflush(stdout);
	return 0;
}


int debug_write_bytes(unsigned char *c, unsigned count)
{
	int n;
	printf("write_bytes %x,%u\n", c, count); fflush(stdout);
	for (n = 0; n < count; n++)
	{
		printf(":%02x: ", c[n]);
	}
	fflush(stdout);
	n = write_bytes(c, count);
	if (n) 
	{
		printf("err\n"); fflush(stdout);
		return n;
	}
	printf(". Done.\n");
	fflush(stdout);
	return 0;
}
/* 
#define read_bytes debug_read_bytes 
#define write_bytes debug_write_bytes
*/

/* Read an incoming packet */
dsk_err_t read_packet(byte  *pkt, int *length)
{
	byte ch;
	unsigned len, n, crc;

	while(1)
	{
		CRC_Clear();
		do
		{
			if (read_bytes(&ch, 1)) return DSK_ERR_ABORT;
		}
		while (ch != SOH);
		if (read_bytes(pkt_h, 2)) return DSK_ERR_ABORT;
		len = pkt_h[0];
		len = (len << 8) | pkt_h[1];
/*		printf("\nlen=%d\n", len); fflush(stdout); */
		if (read_bytes(pkt, len)) return DSK_ERR_ABORT;
		if (read_bytes(pkt_t, 2)) return DSK_ERR_ABORT;
		crc = pkt_t[0];
		crc = (crc << 8) | pkt_t[1];
		for (n = 0; n < len; n++)
		{
			CRC_Update(pkt[n]);
		}
/*		printf("\ncrc=%x\n", crc); fflush(stdout); */
		if (crc == CRC_Done())
		{
			ch = ACK;
			*length = len;
			return write_bytes(&ch, 1);
		}
		ch = NAK;
		if (write_bytes(&ch, 1)) return DSK_ERR_ABORT;
	}
}

/* Write a packet */
dsk_err_t write_packet(byte *pkt, int len)
{
	byte ch;
	unsigned n, crc;
	while(1)
	{
		CRC_Clear();
		pkt_h[0] = (len >> 8);
		pkt_h[1] = (len & 0xFF);
		for (n = 0; n < len; n++)
		{
			CRC_Update(pkt[n]);
		}
		crc = CRC_Done();
		pkt_t[0] = crc >> 8;
		pkt_t[1] = crc & 0xFF;

		ch = STX;	/* Start of return packet */
		if (write_bytes(&ch,      1) ||
		    write_bytes(pkt_h,    2) ||
		    write_bytes(pkt,    len) ||
		    write_bytes(pkt_t,    2) ||
		    read_bytes (&ch,      1)) return DSK_ERR_ABORT;

		if (ch == ACK) 
		{
			return DSK_ERR_OK;
		}
		/* If ch isn't NAK, we're in trouble... */
	}
}





