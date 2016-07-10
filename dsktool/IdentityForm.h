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

#if !defined(AFX_IDENTITYFORM_H__4BD06B38_8C56_48F2_9DA0_15D12C23A01A__INCLUDED_)
#define AFX_IDENTITYFORM_H__4BD06B38_8C56_48F2_9DA0_15D12C23A01A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class IdentityForm : public wxDialog  
{
	DSK_GEOMETRY m_dg;
public:
	IdentityForm(wxWindow* parent, const wxString &title, DSK_PDRIVER dsk, DSK_GEOMETRY *dg);
	virtual ~IdentityForm();

};

#endif // !defined(AFX_IDENTITYFORM_H__4BD06B38_8C56_48F2_9DA0_15D12C23A01A__INCLUDED_)
