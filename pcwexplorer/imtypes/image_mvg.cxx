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
    #pragma implementation "image_mvg.cxx"
    #pragma interface "image_mvg.hxx"
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
#include "image_mvg.hxx"
#include "image_utils.hxx"

MvgImageHandler::MvgImageHandler()
{
	m_name = "PCW16 screen (mono VGA)";
	m_extension = "mvg";
	m_type = wxBITMAP_TYPE_MVG;
	m_mime = "image/x-mono-vga";
}

MvgImageHandler::~MvgImageHandler()
{
}

#if wxUSE_STREAMS
bool MvgImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index )
{
	return LoadFile(image, stream, verbose);
}

bool MvgImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose )
{
	int sx, x, y, m, o;
	unsigned char *data, dbyte[80];

	stream.SeekI(128, wxFromCurrent);

	/* Empty the image */
	image->Destroy();
	image->Create(640, 480);
	data = image->GetData();
	if (!data)
        {
        	wxLogError( wxT("Cannot allocate memory to load MVG.") );
		return FALSE;
	}

	memset(data, 0, 640*480*3);

	o = 0;
	for (y = 0; y < 480; y++)
	{
		m = 0x80;
		sx = 0;
		stream.Read(dbyte, 80);		
		for (x = 0; x < 640; x++)
		{
			if (dbyte[sx] & m)
			{
				data[o  ] = 0xFF;
		 		data[o+1] = 0xFF;
				data[o+2] = 0xFF;
			}
			o += 3;
			m = m >> 1;
			if (!m) { sx++; m = 0x80; }
		}			
	}
	image->SetMask( FALSE );

	return TRUE;

}


bool MvgImageHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
        int x, y, w, h, px, mask;
        unsigned char *data, mvgfile[38400], header[128];
        int offset, doffset;

	memset(&header, 0, sizeof(header));
	memcpy(&header, "Mono VGA image\033\000MVG1", 20);

        w = image->GetWidth();
        h = image->GetHeight();

        data = image->GetData();

        if (!data) return FALSE;
        memset(mvgfile, 0, 38400);

        offset = 0;
        for (y = 0; y < h; y++)
        {
                if (y >= 480) break;
                for (x = 0; x < w; x++)
                {
                        if (x >= 640)
                        {
                                offset += 3;
                                continue;
                        }
                        px = mono(x,y,data[offset],
                                  data[offset+1],
                                  data[offset+2]);
                        offset += 3;

                        doffset = (y * 80) + (x / 8);

                        mask = 0x80;
                        if (x % 8) mask = mask >> (x % 8);
                        if (px) mvgfile[doffset] |= mask;
                }
        }
	stream.Write(header, 128);
        stream.Write(mvgfile, 38400);
        return TRUE;

}


bool MvgImageHandler::DoCanRead( wxInputStream& stream )
{
	char buf[20];

	stream.Read(&buf, 20);
	stream.SeekI(-20, wxFromCurrent);

	return !memcmp(buf, "Mono VGA image\033\000MVG1", 20);	
}


#endif


