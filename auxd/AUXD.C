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
#include <string.h>
#include <cpm.h>
#include "crc16.h"
#include "auxd.h"


byte inbuf[BUFSIZE];
byte outbuf[BUFSIZE];
static byte crcbuf[512];

byte gl_verbose;

/* auxd is a server, running under CP/M, which receives LibDsk requests
 * via its serial port and executes them. 
 *
 * Whereas a proper libdskd would have all of libdsk as a backend here,
 * libdsk is too big and complicated to compile on CP/M (at least with
 * the Hi-Tech 3.09 compiler) and so the backend is custom-made. 
 */ 

int main(int argc, char **argv)
{
	dsk_err_t err;
	int inlen, outlen;
	int refcount = 0;
	int n;

	if (dsk_init()) return 1;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "-V")) gl_verbose = 1;
	}

	CRC_Init(crcbuf);
	printf("Idle.        \r");
	fflush(stdout);

	while (1)
	{
		if (bdos(6, 0xFF) == 3) 
		{
			printf("Terminated.  \n");
			return 0;
		}
		err = read_packet(inbuf, &inlen);
		if (!err) 
		{
			printf("Processing.  \r");
/*{
	int n;
	printf("Input packet: ");
	for (n = 0; n < inlen; n++)
	{
		printf("%02x ", inbuf[n]);
	}
	printf("\n");
}*/
			err = dsk_rpc_server(inbuf, inlen, outbuf, &outlen,
						&refcount);
/*{
	int n;
	printf("Output packet: ");
	for (n = 0; n < BUFSIZE - outlen; n++)
	{
		printf("%02x ", outbuf[n]);
	}
	printf("\n");
}*/
			write_packet(outbuf, BUFSIZE - outlen);
			if (refcount == 0) printf("Idle.        \r");
		}
		if (err == DSK_ERR_ABORT)
		{
			bdos(0x0D, 0);
			printf("Aborted.     \n");
			return 1;
		}
	}
	return 0;
}
