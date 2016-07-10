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

/* This is a cut-down version of the LibDsk RPC server, lib/rpcserv.c. 
 * It has been abbreviated in various ways, including:
 * > Less use of malloc and big buffers on the stack
 * > Use of integer handles rather than DSK_PDRIVER pointers
 * > Not all functions implemented
 */
#include <stdio.h>
#include <stdlib.h>
#include <cpm.h>
#include "auxd.h"

typedef unsigned char byte;
typedef unsigned short int16;
typedef unsigned long  int32;

#include "rpcpack.h"

#define V_PRINTF  if (gl_verbose) printf

/* The big switch in rpcserv.c is too big for the Hi-Tech CP/M compiler. 
 * Break it into 2. */
static dsk_err_t dsk_rpc_server2(int16 function,
			unsigned char *input, int inp_len, 
		         unsigned char *output, int *out_len,
			 int *ref_count)
{
	dsk_err_t err, err2;
	char *str1, *str2, *str3;
	DSK_PDRIVER hdriver;
	int32 nd, int1; /*, int2, int3, int4, int5, int6, int7; */
	int16 *props;
	int16 buflen;
/*	DSK_FORMAT fmt;
	DSK_GEOMETRY geom; */
	int n;
	short short1;

	switch (function)
	{
		case RPC_DSK_OPEN:
		case RPC_DSK_CREAT:
			err = dsk_unpack_string(&input, &inp_len, &str1);
			if (err) return err;
			err = dsk_unpack_string(&input, &inp_len, &str2);
			if (err) return err;
			err = dsk_unpack_string(&input, &inp_len, &str3);
			if (err) return err;
			if (gl_verbose)
			{
				printf("Opening drive %s\n", str1);
				fflush(stdout);
			}
			err2 = dsk_open(&hdriver, str1, str2, str3);
			err = dsk_pack_err(&output, out_len, err2); 
			if (err) return err;
			err = dsk_pack_i32(&output, out_len, hdriver); 
			if (err) return err;
			if (ref_count && err2 == DSK_ERR_OK) ++(*ref_count);	
			break;
		case RPC_DSK_CLOSE:
			err = dsk_unpack_i32(&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			if (gl_verbose)
			{
				printf("Closing drive %c:\n", hdriver);
				fflush(stdout);
			}
			err2 = dsk_close(hdriver);
			err = dsk_pack_err(&output,  out_len, err2);
			if (ref_count) --(*ref_count);
			return DSK_ERR_OK;
		case RPC_DSK_OPTION_ENUM:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_i32 (&input, &inp_len, &int1);
			if (err) return err;
			V_PRINTF("Enumerate option: %c: %ld\n", hdriver, int1);
			err2 = dsk_option_enum(hdriver, int1, &str1);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_string(&output, out_len, str1);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_OPTION_SET:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_string(&input, &inp_len, &str1);
			if (err) return err;
			err = dsk_unpack_i32 (&input, &inp_len, &int1);
			if (err) return err;
			V_PRINTF("Set option: %c: %s=%ld\n", hdriver, str1, int1);
			err2 = dsk_set_option(hdriver, str1, int1);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_OPTION_GET:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_string(&input, &inp_len, &str1);
			if (err) return err;
			V_PRINTF("Get option: %c: %s\n", hdriver, str1);
			err2 = dsk_get_option(hdriver, str1, &short1);
/*			V_PRINTF("Value=%d\n", short1); */
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i32(&output, out_len, short1);
			if (err) return err;
			return DSK_ERR_OK;

		case RPC_DSK_PROPERTIES:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			buflen = 0;
			str1 = "";
			V_PRINTF("Get drive properties: %c:\n", hdriver);
			err2 = dsk_properties(hdriver, &props, &buflen, &str1);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i16(&output, out_len, buflen);
			if (err) return err;
			for (n = 0; n < buflen; n++)
			{
				err = dsk_pack_i16(&output, out_len, props[n]);
				if (err) return err;
			}
			err = dsk_pack_string(&output, out_len, str1);
			if (err) return err;
			return DSK_ERR_OK;	
		case RPC_DSK_GETCOMMENT:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			V_PRINTF("Get comment: %c:\n", hdriver);
			err2= dsk_get_comment(hdriver, &str1);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_string(&output, out_len, str1);
			if (err) return err;
			return DSK_ERR_OK;

		case RPC_DSK_SETCOMMENT:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_string(&input, &inp_len, &str1); 
			if (err) return err;
			V_PRINTF("Set comment: %c: '%s'\n", hdriver, str1);
			err2= dsk_set_comment(hdriver, str1);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			return DSK_ERR_OK;
	}
}

dsk_err_t dsk_rpc_server(unsigned char *input, int inp_len, 
		         unsigned char *output, int *out_len,
			 int *ref_count)
{
	dsk_err_t err, err2;
	unsigned char *buf1, status;
	int16 function, buflen, filler;
	DSK_PDRIVER hdriver;
	int32 nd, int1, int2, int3, int4, int5, int6, int7;
	DSK_GEOMETRY geom;
	DSK_FORMAT fmt;
	int n;
	unsigned short rcount;
	static DSK_FORMAT fmtbuf[256];

	*out_len = BUFSIZE;

	err = dsk_unpack_i16(&input, &inp_len, &function); if (err) return err;

	fflush(stdout);
	switch(function)
	{
		case RPC_DSK_OPEN:
		case RPC_DSK_CREAT:
		case RPC_DSK_CLOSE:
		case RPC_DSK_OPTION_GET:
		case RPC_DSK_OPTION_SET:
		case RPC_DSK_OPTION_ENUM:
		case RPC_DSK_PROPERTIES:
		case RPC_DSK_GETCOMMENT:
		case RPC_DSK_SETCOMMENT:
			return dsk_rpc_server2(function,
				input, inp_len, output, out_len, ref_count);
		case RPC_DSK_DRIVE_STATUS:
			err = dsk_unpack_i32(&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err2= dsk_drive_status(hdriver, &geom, int1, &status);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i16(&output, out_len, status);
			if (err) return err;
			return DSK_ERR_OK;

		case RPC_DSK_PREAD:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			V_PRINTF("Read sector: %c: cylinder %ld head %ld "
				"sector %ld\n", hdriver, int1, int2, int3);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_pread(hdriver, &geom, output + 4,
					int1, int2, int3);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i16(&output, out_len, geom.dg_secsize);
			if (err) return err;
			(*out_len) -= geom.dg_secsize;
			return DSK_ERR_OK;
		case RPC_DSK_PWRITE:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
/* Rather than unpack the sector buffer, just remember where it is (buf1)
 * and skip over the data */
			err = dsk_unpack_i16(&input, &inp_len, &buflen);
			if (err) return err;
			buf1 = input;
			if (inp_len < buflen) return DSK_ERR_RPC;
			input += buflen;
			inp_len -= buflen;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			V_PRINTF("Write sector: %c: cylinder %ld head %ld "
				"sector %ld\n", hdriver, int1, int2, int3);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_pwrite(hdriver, &geom, buf1,
					int1, int2, int3);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_XREAD:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int4);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int5);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int6);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int7);
			if (err) return err;
			V_PRINTF("Ext read sector: %c: cylinder %ld head %ld "
				"sector (%ld, %ld, %ld) size=%ld deleted=%ld\n",
			       	hdriver, int1, int2, int3, int4, int5, 
				int6, int7);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_xread(hdriver, &geom, output + 4,
					int1, int2, int3, int4, int5,
					int6, &int7);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i16(&output, out_len, int6);
			if (err) return err;
			(*out_len) -= int6;
			output     += int6;
			err = dsk_pack_i32(&output, out_len, int7);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_XWRITE:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
/* Rather than unpack the sector buffer, just remember where it is (buf1)
 * and skip over the data */
			err = dsk_unpack_i16(&input, &inp_len, &buflen);
			if (err) return err;
			buf1 = input;
			if (inp_len < buflen) return DSK_ERR_RPC;
			input += buflen;
			inp_len -= buflen;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int4);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int5);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int6);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int7);
			if (err) return err;
			V_PRINTF("Ext write sector: %c: cylinder %ld head %ld "
				"sector (%ld, %ld, %ld) size=%ld deleted=%ld\n",
			       	hdriver, int1, int2, int3, int4, int5, 
				int6, int7);
			err2 = dsk_xwrite(hdriver, &geom, buf1,
					int1, int2, int3, int4, int5, 
					int6, int7);
			err= dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_PTREAD:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			V_PRINTF("Read track: %c: cylinder %ld head %ld\n",
				hdriver, int1, int2);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_ptread(hdriver, &geom, output + 4,
					int1, int2);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			int3 = geom.dg_secsize * geom.dg_sectors;
			err = dsk_pack_i16(&output, out_len, int3);
			if (err) return err;
			(*out_len) -= int3;
			return DSK_ERR_OK;
		case RPC_DSK_XTREAD:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int4);
			if (err) return err;
			V_PRINTF("Ext read track: %c: cylinder %ld/%ld "
				"head %ld/%ld\n", hdriver, int1, int3,
				int2, int4);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_xtread(hdriver, &geom, output + 4,
					int1, int2, int3, int4);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			int5 = geom.dg_sectors * geom.dg_secsize;
			err = dsk_pack_i16(&output, out_len, int5);
			if (err) return err;
			(*out_len) -= int5;
			return DSK_ERR_OK;
		case RPC_DSK_RTREAD:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int3);
			if (err) return err;
			V_PRINTF("Read raw track: %c: cylinder %ld head %ld mode=%ld\n",
				hdriver, int1, int2, int3);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and (repeated) buffer length */
			err2 = dsk_rtread(hdriver, &geom, output + 8,
					int1, int2, int3, &buflen);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i32(&output, out_len, buflen);
			if (err) return err;
			err = dsk_pack_i16(&output, out_len, buflen);
			if (err) return err;
			(*out_len) -= int3;
			return DSK_ERR_OK;

		case RPC_DSK_PFORMAT:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			/* Get cylinder and head */
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			for (n = 0; n < geom.dg_sectors; n++)
			{
				err = dsk_unpack_format(&input, &inp_len,
						&fmtbuf[n]);
				if (err) return err;
			}
			err = dsk_unpack_i16(&input, &inp_len, &filler);
			V_PRINTF("Format track: %c: cylinder %ld head %ld "
				 "filler=%02x\n", hdriver, int1, int2, filler);
			fflush(stdout);
			err2= dsk_pformat(hdriver, &geom, int1, int2, 
					fmtbuf, filler);
			err= dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err= dsk_pack_geom(&output, out_len, &geom);
			if (err) return err;
			return DSK_ERR_OK;

		case RPC_DSK_GETGEOM:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			V_PRINTF("Probe geometry: %c:\n", hdriver);
			err2 = dsk_getgeom(hdriver, &geom);	
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_geom(&output, out_len, &geom);
			if (err) return err;
			return DSK_ERR_OK;

		case RPC_DSK_PSECID:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			V_PRINTF("Sector ID: %c: cylinder %d head %d\n", 
					hdriver, int1, int2);
			fflush(stdout);
			err2 = dsk_psecid(hdriver, &geom, int1, int2, &fmt);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_format(&output, out_len, &fmt);
			return err;
		case RPC_DSK_PSEEK:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			V_PRINTF("Seek to track: %c: cylinder %ld head %ld\n",
				hdriver, int1, int2);
/* To avoid additional buffering, read straight into the output buffer.
 * Add 4 bytes for error code and buffer length */
			err2 = dsk_pseek(hdriver, &geom, int1, int2);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			return DSK_ERR_OK;
		case RPC_DSK_TRACKIDS:
			err = dsk_unpack_i32 (&input, &inp_len, &nd);
			if (err) return err;
			hdriver = (unsigned int)nd;
			err = dsk_unpack_geom(&input, &inp_len, &geom);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int1);
			if (err) return err;
			err = dsk_unpack_i32(&input, &inp_len, &int2);
			if (err) return err;
			V_PRINTF("Identify all sectors: %c: cylinder %d head %d\n", hdriver, int1, int2);
			err2 = dsk_ptrackids(hdriver, &geom, int1,
				int2, &rcount, fmtbuf);
			err = dsk_pack_err(&output, out_len, err2);
			if (err) return err;
			err = dsk_pack_i32(&output, out_len, rcount);
			if (err) return err;
			for (n = 0; n < rcount; n++)
			{
				err = dsk_pack_format(&output, out_len, &fmtbuf[n]);
				if (err) return err;
			}
			return DSK_ERR_OK;
		default:
			err2 = DSK_ERR_UNKRPC;
			fprintf(stderr, 
				"\nRPC Function %d unimplemented\n", function);
			err = dsk_pack_err(&output, out_len, err2);
			break;
	}

	return DSK_ERR_OK;
}


