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

typedef struct lls_packet
{
	unsigned char type;
	union 
	{
		unsigned char  buf[4];
		unsigned char  i1;
		unsigned short i2;
		unsigned long  i4;
	} payload;
	unsigned char trailer[1];
} LLS_PACKET;

#define PT_RESULT    0
#define PT_CONNECT   1
#define PT_HANGUP    2
#define PT_ABORT     3
#define PT_SETDATA   8
#define PT_SETCTRL   9
#define PT_SETCABLE 10
