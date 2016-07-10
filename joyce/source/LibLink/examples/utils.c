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

*****************************************************************************

  PARTEST performs diagnostic testing on the parallel port components of 
LibLink. It allows individual bits in the port to be set and read.

*/

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#ifdef HAVE_CONIO_H
char pollch()
{
	if (kbhit()) return getchar();
	Sleep(10);
	return 0;
}
#endif

#ifdef HAVE_TERMIOS_H
/* Wait 10ms for a keypress. If there isn't one, return 0. */
char pollch()
{
        struct termios ts, ots;
        char c;
        int i;
	fd_set rfds;
	struct timeval tv;

        int filen = fileno(stdin);
/* Switch the terminal to raw mode */
        tcgetattr(filen, &ts);          
        tcgetattr(filen, &ots);
        cfmakeraw(&ts);
        tcsetattr(filen, TCSANOW, &ts);
        
        fflush(stdout);
        fflush(stdin);
/* Use select() to see if there's a key ready */	
	FD_ZERO(&rfds);
	FD_SET(filen, &rfds);	
	tv.tv_sec = 0;
	tv.tv_usec = 10;
	if (!select(1, &rfds, NULL, NULL, &tv)) i = -1; /* timeout */
	else i = read(filen, &c, 1);
        
        tcsetattr(filen, TCSANOW, &ots);

        fflush(stdin);
	if (i < 0) return 0;
        return c;
}

#endif

