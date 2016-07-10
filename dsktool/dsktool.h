/************************************************************************

    Dsktool v1.0.2 - LibDsk front end

    Copyright (C) 2005, 2007  John Elliott <jce@seasip.demon.co.uk>

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


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

class wxConfigBase;

class DsktoolApp : public wxApp
{
public:
	DsktoolApp(void);
    bool OnInit(void);

	wxConfigBase *m_config;

private:
	DECLARE_EVENT_TABLE()
};


// Command IDs
#define ID_APP_ABOUT 200
#define ID_IDENTIFY 201
#define ID_FORMAT 202
#define ID_TRANSFORM 203

#include "libdsk.h"	

