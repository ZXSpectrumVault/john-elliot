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

int pc_mouse_init(void);
void pc_show_pointer(void);
void pc_hide_pointer(void);
int pc_poll_mouse(int *x, int *y, int *button);
void idle(void);
void pc_idle_init(void);
