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
    #pragma implementation "image_mda.cxx"
    #pragma interface "image_mda.hxx"
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
#include "image_mda.hxx"
#include "image_utils.hxx"

MdaImageHandler::MdaImageHandler()
{
	m_name = "MicroDesign Area";
	m_extension = "mda";
	m_type = wxBITMAP_TYPE_MDA;
	m_mime = "image/x-microdesign-area";
}

MdaImageHandler::~MdaImageHandler()
{
}

void MdaImageHandler::PlotByte(unsigned char *data, int offset, unsigned char b)
{
	int m, n;

	m = 0x80;
	for (n = 0; n < 8; n++)
	{
		if (b & m) 
		{
			data[offset  ] = 0xFF;
			data[offset+1] = 0xFF;
			data[offset+2] = 0xFF;	
		}
		offset += 3; 
		m = m >> 1;
	}
}

#if wxUSE_STREAMS
bool MdaImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index )
{
	UNUSED(index);
	return LoadFile(image, stream, verbose);
}
bool MdaImageHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose )
{
	int x, y, w, h, n, count, mdaver;
	unsigned char *data, *mdrow, chr, chr2;
	unsigned char mda_header[132];

	UNUSED(verbose);
	image->Destroy();

	stream.Read(mda_header, 132);

	if (memcmp(mda_header, ".MDA", 4)) return FALSE;
	mdaver = 0;
        if (!memcmp(mda_header+18, "v1.00\r\n", 7)) mdaver = 2;
        if (!memcmp(mda_header+18, "v1.30\r\n", 7)) mdaver = 3;

	if (!mdaver) return FALSE;

        h =     mda_header[128] + 256 * mda_header[129];
        w = 8 * mda_header[130] + 256 * mda_header[131];

	image->Create(w, h);
	data = image->GetData();
	mdrow = new unsigned char[w/8];
	if (!data || !mdrow)
        {
		if (mdrow) delete mdrow;
        	wxLogError( wxT("Cannot allocate memory to load MDA.") );
		return FALSE;
	}
	memset(data, 0, 3*w*h);
	memset(mdrow, 0, w/8);

	if (mdaver == 2)	/* Load a 2.x MDA. This is an RLE-encoded */
	{			/* byte stream; RLE groups may cover */
		for (x = y = 0; y < h; )	/* multiple lines */
		{
			stream.Read(&chr, 1);
			if (chr == 0 || chr == 0xFF)	/* Repeat seq. */
			{
				stream.Read(&chr2, 1);
				count = chr2;
				if (!count) count = 256;

				while (count)
				{
					PlotByte(data, (y*w+x)*3, chr);
					--count;
					x += 8;
					if (x >= w) { x = 0; ++y; }
				}
			}
			else 	/* Not a repeat sequence */
			{
				PlotByte (data, (y*w+x)*3, chr);
				x += 8;
				if (x >= w) { x = 0; ++y; }
			}
		}	
	}
	if (mdaver == 3)
	{
		for (y = 0; y < h; y++)
		{
			/* Row type */
			stream.Read(&chr, 1);
 			switch(chr)
			{
				case 0: /* All the same byte */
				stream.Read(&chr2, 1);
				for (n = 0; n < (w/8); n++)
				{
					mdrow[n] = chr2;
				}
				break;

				case 1:	/* RLE Encoded */
				case 2:	/* RLE Encoded & XOR with previous */
				for (x = 0; x < (w/8); )
				{
					stream.Read(&chr2, 1);
					if (chr2 >= 129) /* RLE sequence */
					{
						count = 257-chr2;
						stream.Read(&chr2, 1);
						for (n = 0; n < count; n++)
						{
							assert(x < (w/8));
							if (chr == 1)  mdrow[x++] = chr2;
							else           mdrow[x++] ^= chr2;
						}
					}
					else /* Not RLE sequence */
					{
						count = 1 + chr2;
						for (n = 0; n < count; n++)
						{
                                                        assert(x < (w/8));
							stream.Read(&chr2, 1);	
							if (chr == 1)  mdrow[x++] = chr2;
                                                        else           mdrow[x++] ^= chr2;

						}
					}	/* End RLE check */
				}		/* End for */
				break;
			}			/* End switch */
			for (x = 0; x < w/8; x++)
			{
				PlotByte(data, (y*w+x*8)*3, mdrow[x]);
			}
		}
	}
	image->SetMask( FALSE );

	return TRUE;

}



bool MdaImageHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
	int x1, x, y, w, w1, h1, h, px, mask;
	unsigned char *data, header[132], *mdrow, b;
	int offset = 0;

	UNUSED(verbose);

	w = image->GetWidth();
	h = image->GetHeight();

	data = image->GetData();

	if (!data) return FALSE;

	/* Generate a header. We write out in MD2 format, for compatibility */
	/* For ease of implementation, don't let RLE run over the ends of lines */

	h1 = ((h + 3) /4) * 4;	/* No. of rows to write; MDA wants this to be a */
				/* multiple of 4 */
	w1 = ((w + 7) /8);      /* No. of 8-pixel columns */

        mdrow = new unsigned char[w1];
        if (!mdrow) return FALSE;

	/* Create the header */
	memset(&header, 0, sizeof(header));
	strcpy((char *)header, ".MDA wxWindows Appv1.00\r\n  GPL  \r\n"); 

	header[128] =   h1       & 0xFF;
	header[129] =  (h1 >> 8) & 0xFF;
	header[130] =   w1       & 0xFF;
	header[131] =  (w1 >> 8) & 0xFF;

	stream.Write(&header, 132);
	for (y = 0; y < h1; y++)
	{
		memset(mdrow, 0, w1);
		if (y < h) for (x = 0; x < w; x++)
		{
			px = mono(x,y,data[offset], 
                                  data[offset+1],
                                  data[offset+2]);
			offset += 3;

			mask = 0x80;
			if (x % 8) mask = mask >> (x % 8);
			if (px) mdrow[x/8] |= mask;
		}
		/* Uncompressed row generated. RLE it. */
                for (x = 0; x < w1; )
                {
                        b = mdrow[x];

                        if (b != 0xFF && b != 0) /* Normal byte */
                        {
				stream.Write(&b, 1);
                                ++x;
                        }
                        else    /* RLE a run of 0s or 0xFFs */
                        {
                                for (x1 = x; x1 < w1; x1++)
                                {
                                        if (mdrow[x1] != b) break;
                                        if (x1 - x > 256) break;
                                }
                                x1 -= x;        /* x1 = no. of repeats */
                                if (x1 == 256) x1 = 0;
                                stream.Write(&b,1);
				b = x1;
				stream.Write(&b, 1);
                                x += x1;
                        }
		}
	
	}
	delete mdrow;
	return TRUE;
}


bool MdaImageHandler::DoCanRead( wxInputStream& stream )
{
        char buf[25];

        stream.Read(&buf, 25);
        stream.SeekI(-25, wxFromCurrent);

	if (memcmp(buf, ".MDA", 4)) return FALSE;
	if (!memcmp(buf+18, "v1.00\r\n", 7)) return TRUE;
	if (!memcmp(buf+18, "v1.30\r\n", 7)) return TRUE;
	return FALSE;	
}


#endif


