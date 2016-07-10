/***************************************************************************
 *                                                                         *
 *    HHEDIT: Hungry Horace editor                                         *
 *    Copyright (C) 2009 John Elliott <jce@seasip.demon.co.uk>             *
 *                                                                         *
 *    This program is free software; you can redistribute it and/or modify *
 *    it under the terms of the GNU General Public License as published by *
 *    the Free Software Foundation; either version 2 of the License, or    *
 *    (at your option) any later version.                                  *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *    GNU General Public License for more details.                         *
 *                                                                         *
 *    You should have received a copy of the GNU General Public License    *
 *    along with this program; if not, write to the Free Software          *
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                         *
 ***************************************************************************/
typedef enum { VGA, CGA, MDA, UNKNOWN } GRAPHICS;
typedef unsigned char byte;

#define K_MOUSE (-32)

extern GRAPHICS gl_graphics;
extern byte font8x8[];
extern int gl_mbutton, gl_xmouse, gl_ymouse;

int getmode(void);
int setmode(int m);
int vga_init(void);
int ega_init(void);
int cga_init(void);
int mda_init(void);

void put_4x4(unsigned r, unsigned c, char pattern);
void putstr(unsigned r, unsigned c, const char *s, int rv);
void putstatus(const char *s);
int getkey(void);
int getevent(void);
void debounce_mouse(void);
void drawcell(int r, int c, unsigned type, int attr);
void drawsprite(int r, int c, unsigned bitmap, int fg);
void savescr(void);
void restorescr(void);
void cls(void);
void rectangle(unsigned x, unsigned y, unsigned w, unsigned h, unsigned fill);
void popup(const char *s);
void draw_displacement(unsigned r1, unsigned c1, unsigned r2, unsigned c2, int active);
