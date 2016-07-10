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
    #pragma implementation "image_cut.cxx"
    #pragma interface "image_cut.hxx"
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
#include "image_cut.hxx"
#include "image_utils.hxx"

CutImageHandler::CutImageHandler()
{
	m_name = "Stop Press Cutout";
	m_extension = "cut";
	m_type = wxBITMAP_TYPE_CUT;
	m_mime = "image/x-stop-press-cutout";
}

CutImageHandler::~CutImageHandler()
{
}

#if wxUSE_STREAMS
bool CutImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index )
{
	return LoadFile(image, stream, verbose);
}

bool CutImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose )
{
	int x, y, w, h, sx, wb, m, o;
	unsigned char *data, *buf, header[4];

	/* Empty the image */
	image->Destroy();

	/* Get dimensions */
	stream.Read(header, 4);	

	h = (header[0] + 256*header[1]) / 2;
	w = header[2] + 256*header[3];

	wb = (w+10)/8;
	
	image->Create(w, h);
	data = image->GetData();
	if (!data)
        {
        	wxLogError( wxT("Cannot allocate memory to load CUT.") );
		return FALSE;
	}
	buf = new unsigned char[wb];
	if (!buf)
	{
		image->Destroy();
                wxLogError( wxT("Cannot allocate memory to load CUT.") );
                return FALSE;
	}
	memset(data, 0, w*h*3);

	for (y = 0; y < h; y++)
	{
		o = (y * w * 3);
		stream.Read(buf, wb);	
		m = 0x80;
		sx = 0;
		m = 0x80;
		for (x = 0; x < w; x++)
		{
			if (buf[sx] & m)
			{	
				data[o  ] = 0xFF;
				data[o+1] = 0xFF;
				data[o+2] = 0xFF;
			}
			o += 3;
			m = m >> 1;
			if (!m) { m = 0x80; ++sx; }
		}			
	}
	image->SetMask( FALSE );

	return TRUE;

}


bool CutImageHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
        int x, y, w, h, wb, px, mask;
        unsigned char *data, *cutfile;
        int offset, doffset;

        w = image->GetWidth();
        h = image->GetHeight();

	wb = (w + 10) / 8;

        data = image->GetData();
        if (!data) return FALSE;

	cutfile = new unsigned char[4 + wb*h];
	if (!cutfile) return FALSE;

        memset(cutfile, 0, 4+wb*h);

	cutfile[0] =  (2*h)       & 0xFF;
	cutfile[1] = ((2*h) >> 8) & 0xFF;
	cutfile[2] =     w        & 0xFF;
	cutfile[3] =    (w  >> 8) & 0xFF;

        offset = 0;
        for (y = 0; y < h; y++)
        {
                for (x = 0; x < w; x++)
                {
                        px = mono(x,y,data[offset],
                                  data[offset+1],
                                  data[offset+2]);
                        offset += 3;

                        doffset = (y * wb) + (x / 8);

                        mask = 0x80;
                        if (x % 8) mask = mask >> (x % 8);
                        if (px) cutfile[doffset+4] |= mask;
                }
        }
        stream.Write(cutfile, 4+wb*h);
	delete cutfile;
	return TRUE;
}


bool CutImageHandler::DoCanRead( wxInputStream& stream )
{
	/* CUT files have no magic number, unfortunately; so we have to make
         * assumptions */
	return TRUE;
}


#endif


