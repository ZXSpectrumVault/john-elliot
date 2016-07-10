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

#include "Pcw.hxx"

PcwArgs::PcwArgs(int argc, char **argv)
{
	m_fullScreen      = false;
	m_hardwareSurface = false;
	m_hardwarePalette = false;
	m_debug		  = false;
	m_aDrive          = "";
	m_bDrive          = "";
	m_bootRom         = ""; /* Set by derived class */
	gl_homeRoot	  = "Joyce";
	m_argc            = argc;
	m_argv            = argv;
	m_memSize         = 0;
#ifdef _WIN32
        static char buf[PATH_MAX];

        GetModuleFileName(NULL, buf, PATH_MAX);
        m_argv[0] = buf;

#endif
}

bool PcwArgs::stricmp(const char *a, const char *b)
{
	char c,d;

	while ((c = *a) && (d = *b))
	{
		if (islower(c)) c = toupper(c);
		if (islower(d)) d = toupper(d);
		if (c != d) return true;
		++a, ++b;	
	}	
	return false;
}


bool PcwArgs::parse()
{
	for (m_arg = 1; m_arg < m_argc; ++m_arg) if (m_argv[m_arg][0] == '-') 
	{
		switch(m_argv[m_arg][1])
		{
			case 'a': case 'A':
				if (!parseDrive(m_aDrive)) return false;
				break;
			case 'b': case 'B':
				if (!parseDrive(m_bDrive)) return false;
				break;
			case 'e': case 'E':
				if (!parseDrive(m_bootRom)) return false;
				break;
			case 'h': case 'H':
				if (!parseDrive(gl_homeRoot)) return false;
				break;
			case 'm': case 'M':
				if (!parseMem(m_memSize)) return false;
				break;
			default:
				if      (!stricmp(m_argv[m_arg], "-debug"))      m_debug = true;
				else if (!stricmp(m_argv[m_arg], "--debug")) 	 m_debug = true;
				else if (!stricmp(m_argv[m_arg], "-fullscreen")) m_fullScreen = true;
				else if (!stricmp(m_argv[m_arg], "--fullscreen")) m_fullScreen = true;
				else if (!stricmp(m_argv[m_arg], "-f"))          m_fullScreen = true;
				else if (!stricmp(m_argv[m_arg], "--f"))          m_fullScreen = true;
				else if (!stricmp(m_argv[m_arg], "-hwsurface"))  m_hardwareSurface = true;
				else if (!stricmp(m_argv[m_arg], "--hwsurface"))  m_hardwareSurface = true;
				else if (!stricmp(m_argv[m_arg], "-hw"))         m_hardwareSurface = true;
				else if (!stricmp(m_argv[m_arg], "--hw"))         m_hardwareSurface = true;
				else if (!stricmp(m_argv[m_arg], "--hwpalette"))  m_hardwarePalette = true;
				else if (!stricmp(m_argv[m_arg], "-hwpalette"))  m_hardwarePalette = true;
				else if (!stricmp(m_argv[m_arg], "--hwp"))  	 m_hardwarePalette = true;
				else if (!stricmp(m_argv[m_arg], "-hwp"))  	 m_hardwarePalette = true;
				else if (!stricmp(m_argv[m_arg], "--help"))      help();
				else if (!stricmp(m_argv[m_arg], "-help"))       help();
				else if (!stricmp(m_argv[m_arg], "--version"))   version();
				else if (!stricmp(m_argv[m_arg], "-version"))    version();
				else if (!processArg(m_argv[m_arg]))
				{
					fprintf(stderr, "%s: Unknown option: %s\n", m_argv[0], m_argv[m_arg]);
					return false;
				}
				break;

		}		

	}
	return true;
}

bool PcwArgs::parseDrive(string &drvStr)
{
	char *s = strchr(m_argv[m_arg], '=');
	if (s)
	{
		drvStr = s + 1;
		return true;
	}
	if (m_argv[m_arg][2])
	{
		drvStr = m_argv[m_arg] + 2;
		return true;
	}
	++m_arg;
	if (m_arg >= m_argc)
	{
		--m_arg;
		fprintf(stderr, "%s: Incorrect syntax. The %s option must be\n"
				"followed by a filename.\n",
			m_argv[0], m_argv[m_arg]);
		return false;
	}
	drvStr = m_argv[m_arg];
	return true;
}


void PcwArgs::help(void)
{
	printf("Command-line options: \n"
	       "-a <dskfile> : Boot from specified .DSK file\n"
	       "-b <dskfile> : Boot with specified .DSK file in drive B:\n"
               "-debug       : Boot with debug console visible\n"
	       "-e <emsfile> : Boot from specified .EMS / .EMT file\n"
	       "-h <homedir> : Use a different home directory (default is 'Joyce')\n"
	       "-m <memory>  : Specify memory size\n"
	       "-fullscreen  : Run in full-screen (not windowed) mode\n"
	       "-hwsurface   : Use a hardware surface for video\n");
	exit(0);
}



bool PcwArgs::parseMem(int &memSize)
{
	char *s = strchr(m_argv[m_arg], '=');
	if (s)
	{
		memSize = 1024 * atoi(s + 1);
		return true;
	}
	if (m_argv[m_arg][2])
	{
		memSize = 1024 * atoi(m_argv[m_arg] + 2);
		return true;
	}
	++m_arg;
	if (m_arg >= m_argc)
	{
		--m_arg;
		fprintf(stderr, "%s: Incorrect syntax. The %s option must be\n"
				"followed by a number.\n",
			m_argv[0], m_argv[m_arg]);
		return false;
	}
	memSize = 1024 * atoi(m_argv[m_arg]);
	return true;
}


void PcwArgs::version(void)
{
	printf("%s v%d.%d.%d\n", appname(),
			(BCDVERS >> 8) & 0xFF,
			(BCDVERS >> 4) & 0x0F,
			BCDVERS & 0x0F);
	exit(0);
}


bool PcwArgs::processArg(const char *a)
{
	return false;
}
