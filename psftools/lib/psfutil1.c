/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000, 2007  John Elliott

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <stdio.h>
#include "psflib.h"


psf_byte crush(psf_byte b)
{
        int n;
        psf_byte r = 0, lmask, rmask;
        /* XXX Write Z80 asm version for speed in Z80 version */
        for (rmask=0x80, lmask=0x40, n = 0; n < 4; n++)
        {
                if (b & lmask) r |= rmask;
                lmask = lmask >> 2;
                rmask = rmask >> 1;
        }
        return r;
}


psf_byte flip(psf_byte b)
{
        int n;
        psf_byte r = 0, lmask, rmask;
        /* XXX Write Z80 asm version for speed in Z80 version */
        for (rmask=1, lmask=0x80, n = 0; n < 8; n++)
        {
                if (b & lmask) r |= rmask;
                lmask = lmask >> 1;
                rmask = rmask << 1;
        }
        return r;
}

