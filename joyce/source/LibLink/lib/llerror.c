/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ll.h"

#include <string.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

const char *ll_strerror(int err)
{
	switch(err)
	{
		case LL_E_OK:		return "No error.";
		case LL_E_UNKNOWN:	return "Unknown error.";
		case LL_E_NOMEM:	return "Out of memory.";
		case LL_E_BADPTR:	return "Bad pointer passed to liblocolink.";
		case LL_E_SYSERR:	return strerror(errno);
		case LL_E_EXIST:	return "Shared memory file already exists.";
		case LL_E_NOMAGIC:	return "File is not a liblink connector file.";
		case LL_E_CLOSED:	return "The link is not open.";
		case LL_E_OPEN:		return "The link is already open.";
		case LL_E_INUSE:	return "The slave system is already connected to a different master.";
		case LL_E_TIMEOUT:	return "The operation timed out.";
		case LL_E_BADPKT:	return "Packet length is invalid.";
		case LL_E_CRC:		return "Received packet has incorrect CRC.";
		case LL_E_UNEXPECT:	return "Remote computer sent an unexpected value.";
		case LL_E_NODRIVER:	return "Specified driver does not exist.";
		case LL_E_NOTIMPL:	return "Unimplemented function.";
		case LL_E_NOTDEV:	return "Not a suitable device file.";
#ifdef HAVE_WINSOCK_H
		case LL_E_SOCKERR:	
					{
					static char buf[50];
					sprintf(buf, "Windows socket error 0x%x\n", WSAGetLastError());
					return buf;
					}
#else
		case LL_E_SOCKERR:	return hstrerror(h_errno);
#endif
 	}
	return "Unknown error.";
}


