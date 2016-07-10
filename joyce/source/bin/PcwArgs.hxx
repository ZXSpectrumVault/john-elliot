/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-5  John Elliott <seasip.webmaster@gmail.com>

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

extern std::string gl_homeRoot;

class PcwArgs
{
public:
	bool	m_fullScreen;
	bool	m_hardwareSurface;
	bool	m_hardwarePalette;
	bool	m_debug;
	std::string  m_aDrive;
	std::string  m_bDrive;
	std::string  m_bootRom;
	int	m_memSize;

	PcwArgs(int argc, char **argv);
	bool parse();	
	virtual const char *appname(void) = 0;
protected:
	int m_argc, m_arg;
	char **m_argv;
	bool parseDrive(std::string &drive);
	bool parseMem(int &memSize);
	bool stricmp(const char *a, const char *b);
	virtual bool processArg(const char *a);
	void help(void);
	void version(void);
};

