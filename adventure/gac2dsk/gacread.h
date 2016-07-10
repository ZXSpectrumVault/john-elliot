/************************************************************************

    GAC2DSK 1.0.0 - Convert GAC tape files to +3DOS

    Copyright (C) 2007, 2009  John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/
#ifdef __PACIFIC__
#define BUFSIZE 0xC000
#else
#define BUFSIZE 0x10000
#endif

typedef unsigned char byte;

#ifdef INSTANTIATE
#define EXT 
#else
#define EXT extern
#endif

EXT char progname[11];
EXT byte datablock[BUFSIZE];
EXT unsigned datalen;
EXT unsigned gac_sp, gac_pc, gac_de, gac_ix;

int load_gac(const char *filename);
