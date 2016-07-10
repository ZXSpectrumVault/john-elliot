/************************************************************************

    JOYCE v2.2.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2014  John Elliott <seasip.webmaster@gmail.com>

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

#include "Pcw.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"

PcwAsic::PcwAsic(PcwSystem *s) : PcwDevice("general")
{
	m_sys = s;
}


/* [2.2.7] Add UI to enable / disable the JDOS API (which allows CP/M programs
 * to access files on the host system) */
UiEvent PcwAsic::setFileAccess(UiDrawer *d)
{
	int sel = -1;
	UiEvent uie;
	JDOSMode m = m_sys->getJDOSMode();

	while (1)
	{
		UiMenu uim(d);
	
		uim.add(new UiLabel("  Host File Access  ", d));
		uim.add(new UiSeparator(d));

		uim.add(new UiLabel("  Should programs running under CP/M be "
				    "allowed to access files on this  ", d));
		uim.add(new UiLabel("  computer? This is used by the IMPORT "
				    "and EXPORT utilities, but would also  ", d));
		uim.add(new UiLabel("  allow a malicious CP/M program to delete "
				    "or overwrite important files.  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_a, (m == JM_ALWAYS), "  Allow full access", d));
		uim.add(new UiSetting(SDLK_p, (m == JM_PROMPT), "  Prompt for each file", d));
		uim.add(new UiSetting(SDLK_n, (m == JM_NEVER), "  No file access", d));
                uim.add(new UiSeparator(d));

                uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);

                uie =  uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
			case SDLK_a: 
				m = JM_ALWAYS;
				break;
			case SDLK_n:
				m = JM_NEVER;
				break;
			case SDLK_p:
				m = JM_PROMPT;
				break;
			case SDLK_ESCAPE:
				m_sys->setJDOSMode(m);
				return UIE_OK;
			default: break;
		}
	}



}

