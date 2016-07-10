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
#include "image_mvg.hxx"

class PcwMvgFactory : public PcwGraphicFactory
{
protected:
	MvgImageHandler *m_handler;
public:
	PcwMvgFactory();
	virtual ~PcwMvgFactory();

	virtual wxIcon GetBigIcon();	/* 32x32 icon for this class */
	virtual wxIcon GetSmallIcon();	/* 16x16 icon for this class */

        virtual wxSize GetDimensions(const char *); // For image property page 
	virtual int GetImageType(void);
        // What is this type of file called?
        virtual char *GetTypeName();

	virtual bool Identify(const char *filename, const unsigned char *first128 = NULL);
};

PcwFileFactory *GetMvgFileClass();



