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
// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
    #pragma implementation "image_spc.cxx"
    #pragma interface "image_spc.hxx"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "wx/image.h"

// Our own header
#include "image_spc.hxx"
#include "image_utils.hxx"

SpcImageHandler::SpcImageHandler()
{
	m_name = "Stop Press Canvas";
	m_extension = "spc";
	m_type = wxBITMAP_TYPE_SPC;
	m_mime = "image/x-stop-press-canvas";
}

SpcImageHandler::~SpcImageHandler()
{
}

#if wxUSE_STREAMS
bool SpcImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index )
{
	UNUSED(index)
	return LoadFile(image, stream, verbose);
}

bool SpcImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose )
{
	int cx, cy, px, py, m, o;
	unsigned char *data, dbyte[8];

   UNUSED(verbose)
	/* Empty the image */
	image->Destroy();
	image->Create(720, 256);
	data = image->GetData();
	if (!data)
        {
        	wxLogError( wxT("Cannot allocate memory to load SPC.") );
		return FALSE;
	}

	memset(data, 0, 720*256*3);

	for (cy = 0; cy < 32; cy++)	/* 32*90 cells of 8x8 pixels */
	{
		for (cx = 0; cx < 90; cx++)
		{
			stream.Read(dbyte, 8);
			for (py = 0; py < 8; py++)
			{
				m = 0x80;
                                o = ((cy * 5760) + (cx*8) + (py*720))*3;

				for (px = 0; px < 8; px++)
				{
					if (dbyte[py] & m)
					{
						data[o  ] = 0xFF;
						data[o+1] = 0xFF;
						data[o+2] = 0xFF;
					}
					o += 3;
					m = m >> 1;
				}
			}
		}
	}
	image->SetMask( FALSE );

	return TRUE;

}



bool SpcImageHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
	int x, y, w, h, px, mask;
	unsigned char *data, spcfile[23040];
	int offset, doffset;

 	UNUSED(verbose)

	w = image->GetWidth();
	h = image->GetHeight();

	data = image->GetData();

	if (!data) return FALSE;
	memset(spcfile, 0, 23040);

	offset = 0;
	for (y = 0; y < h; y++)
	{
		if (y >= 256) break;
		for (x = 0; x < w; x++)
		{
			if (x >= 720) 
			{
				offset += 3; 
				continue; 
			}
			px = mono(x,y,data[offset], 
                                  data[offset+1],
                                  data[offset+2]);
			offset += 3;

			doffset = ((y / 8) * 720) + ((x / 8) * 8) + (y % 8);

			mask = 0x80;
			if (x % 8) mask = mask >> (x % 8);
			if (px) spcfile[doffset] |= mask;
		}
	}
	stream.Write(spcfile, 23040);
	return TRUE;
}


bool SpcImageHandler::DoCanRead( wxInputStream& stream )
{
	UNUSED(stream)
	/* SPC files have no magic number, unfortunately; so we have to make
         * assumptions */
	return TRUE;
}


#endif


