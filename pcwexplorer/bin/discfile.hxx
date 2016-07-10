/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000  John Elliott

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
// Wrap file I/O in a class

class PcwDiscFile: public cpmFile
{
public:
	int Open(const cpmInode *root, int group, const char *name, int mode);
	int Open(cpmInode *ino, int mode);
	int Read(void *buf, int len);
	int Write(void *buf, int len);
	int Close(void);
protected:
	int EWrap(int error);
	struct cpmInode m_ino;
};
