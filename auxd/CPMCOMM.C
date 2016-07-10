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

/* Naive BDOS-based comms implementation. With custom code this could be
 * much better. */


/*
#define readch()   bdos(3,0)
#define writech(c) bdos(4,c)

int readch()
{
	int c = bdos(3,0);
	printf("%02x ", c & 0xFF);
	fflush(stdout);
	return c;
}



void writech(char c)
{
	printf(">%02x<", c & 0xFF);
	fflush(stdout);
	bdos(4,c);
}
*/
static unsigned char pkt_h[2];
static unsigned char pkt_t[2];
static unsigned char iscpm3;

int read_bytes(unsigned char *c, unsigned count);
int write_bytes(unsigned char *c, unsigned count);

#asm
;
; Written in Z80 assembly; this should improve speed.
;
	psect data
	global	_iscpm3


	psect text
	global	_read_bytes
	global	_write_bytes

poll:	ld	a,(_iscpm3)
	and	a
	ret	z	;Not CP/M 3 -- return carry reset.
	call	5	;Get status
	and	a
	ret	nz	;Return carry reset if device ready
	scf		;Carry set if device busy
	ret
;
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
	ld	a,b
	or	c
	jr	z,rbnone
rblp:	push	hl
	push	bc
rbwait:	
;;;	ld	c,6		;Print a Dot when polling
;;;	ld	e,'.'
;;;	call	5
	ld	c,6
	ld	e,0FFh
	call	5
	cp	3
	jr	z,rbbrk

	ld	c,7		;Get aux input status
	call	poll
	jr	c,rbwait
	ld	c,3
	call	5
	pop	bc
	pop	hl
	ld	(hl),a
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jr	nz,rblp
rbnone:	ld	hl,0
	ret
;
rbbrk:	ld	hl,-28
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
	ld	a,b
	or	c
	jr	z,wbnone
wblp:	push	hl
	push	bc
	ld	e,(hl)
	push	de
wbwait:	
;;;	ld	c,6		;Print a Dot when polling
;;;	ld	e,'.'
;;;	call	5
	ld	c,6
	ld	e,0FFh
	call	5
	cp	3
	jr	z,wbbrk
	ld	c,8	;Get aux output status
	call	poll
	jr	c,wbwait
	pop	de
	ld	c,4
	call	5
	pop	bc
	pop	hl
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jr	nz,wblp
wbnone:	ld	hl,0
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

	iscpm3 = ((bdos(0x0C) & 0xFF) >= 0x30);
/*	printf("iscpm3=%d\n", iscpm3); */
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

	/* Must never return a 0-byte packet; all packets must be 
	 * at least 2 bytes long. */
	if (!len) return DSK_ERR_RPC;
	iscpm3 = ((bdos(0x0C) & 0xFF) >= 0x30);
/*	printf("iscpm3=%d\n", iscpm3); */
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





