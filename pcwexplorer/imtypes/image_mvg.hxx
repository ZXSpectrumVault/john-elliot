/*
    PCW image handlers
    Copyright (C) 2000, 2006  John Elliott <jce@seasip.demon.co.uk>

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
//
// Encode/decode a PCW image: MVG (PCW16 screen) file
//

#include "imtypes.hxx"

class MvgImageHandler : public wxImageHandler
{
public:
	MvgImageHandler();
	~MvgImageHandler();

#if wxUSE_STREAMS
  /* Overloaded; some WX libraries have the 3-parameter version, others the 4-parameter one */
  virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index );
  virtual bool LoadFile( wxImage *image, wxInputStream& stream, bool verbose=TRUE );
  virtual bool SaveFile( wxImage *image, wxOutputStream& stream, bool verbose=TRUE );
  virtual bool DoCanRead( wxInputStream& stream );
#endif
};

