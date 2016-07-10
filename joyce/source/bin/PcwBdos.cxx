/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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

/** A vaguely BDOS-like interface - C=function and          **/
/** DE=parameter. It can also take values in B and HL.      **/
/** Returns HL=0 on failure, other values on success.       **/

/* [2.2.7] Rewritten to allow selective blocking of file access */

#include "Pcw.hxx"
#include "PcwBdos.hxx"
#include "inline.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"


#define HANDLES_MAX 16

static FILE *handles[HANDLES_MAX];/* Can open up to 15 files; handles[0] is 
				 * reserved */
static char pathname[PATH_MAX];		/* DOS pathname passed by Z80 program */
static char lastpathname[PATH_MAX];	/* DOS pathname last time round */
static SDLKey lastoption = SDLK_SPACE;

#define BCDIFY(x) (((x % 10)+16*(x/10)) & 0xFF)


void jdos_time(Z80 *R, int secflag)	/* Read the time into buffer at DE */
{
	time_t secs;			/* If secflag=1, include seconds */
	struct tm *ltm;
	int day, hour, min, sec, lyear;

	time(&secs);
					/* << v1.36: Use local time, 
					 *    rather than UTC. */
	ltm  = localtime(&secs);

	/* Work out the day count. */
	lyear = (ltm->tm_year - 76);	/* Years since 1976 */

	day = (lyear / 4) * 1461;	/* No. of days to start of 
					 * last leapyear */
	lyear = (lyear % 4);
	if (lyear)    { day += 366; --lyear; }
	while (lyear) { day += 365; --lyear; }
	/* Day = day count from 1976 to start of this year */
	day -= (365 + 366 - 1);	
	/* The extra 1 is because the CP/M day count is 1 based */
	day += ltm->tm_yday;

	hour = ltm->tm_hour;
	min  = ltm->tm_min;
	sec  = ltm->tm_sec;	
				/* >> v1.36 */
	if (!R->getDE()) return;
	R->storeB(R->getDE(), day & 0xFF);
	R->storeB(R->getDE() + 1, (day >> 8) & 0xFF);
	R->storeB(R->getDE() + 2, BCDIFY(hour));
	R->storeB(R->getDE() + 3, BCDIFY(min));
	if (secflag) R->storeB(R->getDE() + 4, BCDIFY(sec));
	R->a = BCDIFY(sec);

}
#undef BCDIFY


void txt2fcb(Z80 *R, word addr, char *s)	/* Parse ASCII filename to FCB */
{
	char curch;
	int fcnt = 0;

	while(fcnt < 11) switch(*s)
	{
		case 0:	while (fcnt < 11)
			{
				R->storeB(addr + fcnt, ' ');
				fcnt++;
			}
			return;

		case '.': while (fcnt < 8)
			{
				R->storeB(addr + fcnt, ' ');
				fcnt++;			
			}
			++s;
			break;

		case ':': 
		case '/':	
		case '\\':
			fcnt = 0;
			++s;
			break;

		default:
			/* v1.11: Deal with W95 or Unix filenames which
			 * may have lowercase letters in them */
			curch = (*s) & 0x7F; 
			if (islower(curch)) curch = toupper(curch);
			R->storeB(addr + fcnt, curch);
			++s;
			++fcnt;
			break;

	}
}


static int get_pathname(Z80 *R)
{
	int m = 0;
	char c;

	do {
		c = R->fetchB(R->getDE() + m);	/* Extract pathname */
		pathname[m] = c;
		++m;
		if (m + 1 >= PATH_MAX)
		{
			return 0;	/* [2.2.7] Pathname too long */
		}
	} while (c);

	return 1;
}



word jdos_creat(Z80 *R, JDOSMode mode)	/* Create a file, DE->name */
{
	int n, m;
	char c;

	if (mode == JM_NEVER) return 0;	/* No file access */

	for (n = 1; n < HANDLES_MAX; n++)	/* Find a free handle */
	{
		if (!handles[n]) break;
	}
	if (n >= HANDLES_MAX) return 0;	/* No free handles */
	
	handles[n] = fopen(pathname, "w+b");	/* Create the file */

	if (!handles[n]) return 0;

	return n;
}

word jdos_open(Z80 *R, JDOSMode mode)	/* Open a file, DE->name */
{
	int n, m;
	char c;

	if (mode == JM_NEVER) return 0;	/* No file access */

	for (n = 1; n < HANDLES_MAX; n++)	/* Find a free handle */
	{
		if (!handles[n]) break;
	}
	if (n >= HANDLES_MAX) return 0;	/* No free handles */
	
	handles[n] = fopen(pathname, "r+b");	/* Open the file */

	if (!handles[n]) /* Read/write failed. Try read-only */
		handles[n] = fopen(pathname, "rb");
	if (!handles[n]) return 0;

	return n;
}



word jdos_putc(Z80 *R, JDOSMode mode)	/* Write byte to file */
{
	int n = R->getDE();

	/* [2.2.7] Range-check handle number */
	if (mode == JM_NEVER || n < 0 || n >= HANDLES_MAX || !handles[n]) 
	{
		return 0;	/* Not open */
	}
	if (fputc(R->b, handles[n]) == EOF) return 0;

	return 1;
}


word jdos_getc(Z80 *R, JDOSMode mode)	/* Read a byte from a file into B */
{
	int n = R->getDE();
	int m;

	/* [2.2.7] Range-check handle number */
	if (mode == JM_NEVER || n < 0 || n >= HANDLES_MAX || !handles[n]) 
	{
		return 0;
	}

	m = fgetc(handles[n]);
	if (m == EOF) return 0;		/* EOF */
	R->b = (m & 0xFF);
	return 1;
}

word jdos_close(Z80 *R, JDOSMode mode)	/* Close file */
{
	int n = R->getDE();
	int m;

	if (mode == JM_NEVER || n < 0 || n >= HANDLES_MAX || !handles[n]) 
	{
		return 0;
	}

	m = fclose(handles[n]);

	handles[n] = NULL;

	return(!m);	
}


int jdos(Z80 *R, PcwSystem *sys)
{
	JDOSMode mode = sys->getJDOSMode();
	UiDrawer *d = sys->m_termMenu;

	/* These two functions take a pathname */
	if (R->c == 0x0F || R->c == 0x16)
	{
		/* If all file access is disallowed, don't bother trying to
		 * get the pathname */
		if (mode == JM_NEVER)
		{
			R->setHL(0);
			return 0;
		}
		/* Get the pathname, and fail if it's invalid */
		if (!get_pathname(R))
		{
			R->setHL(0);
			return 0;
		}
		/* Prompt for file access */
		if (mode == JM_PROMPT)
		{
			int sel = -1;
			char filename[51];
			int prompting = 1;
			UiEvent uie;

			if (!strcmp(pathname, lastpathname))
			{
				switch (lastoption)
				{
					case SDLK_y: prompting = 0; break;
					case SDLK_n:
					case SDLK_v:
						R->setHL(0);
						return 0;
				}
			}
			strcpy(lastpathname, pathname);

			if (strlen(pathname) <= 40) 
			     sprintf(filename, "  %-40.40s  ", pathname);
			else sprintf(filename, "  %-20.20s...%-17.17s  ",
					pathname,
					pathname + strlen(pathname) - 17);
			while (prompting)
			{

				UiMenu uim(d);
				uim.add(new UiLabel("  Confirm host file access  ", d));
				uim.add(new UiSeparator(d));
				uim.add(new UiLabel("  A CP/M program is attempting to access the file:  ", d));
				uim.add(new UiLabel(filename, d));
				uim.add(new UiLabel("  Do you want to allow access to this file?  ", d));
				uim.add(new UiSeparator(d));
				uim.add(new UiCommand(SDLK_n, "  No  ", d));
				uim.add(new UiCommand(SDLK_v, "  No, and don't ask again  ", d));
				uim.add(new UiCommand(SDLK_y, "  Yes  ", d));
				d->centreContainer(uim);
				uim.setSelected(sel);

				uie = uim.eventLoop();
				if (uie == UIE_QUIT) return -1;

				sel = uim.getSelected();
				lastoption = uim.getKey(sel);
				switch (lastoption)
				{
					case SDLK_v:
						sys->setJDOSMode(JM_NEVER);
						// FALL THROUGH
					case SDLK_n: 
						R->setHL(0);
						return 0;
					case SDLK_y:
						prompting = 0;
						break;
				}
			}		
		}
	}

	switch(R->c)
	{
		case 0x0F:	/* Open */
		R->setHL( jdos_open(R, mode) );
		break;

		case 0x10:	/* Close */
		R->setHL( jdos_close(R, mode) );
		break;

		case 0x14:	/* Get byte from stream */
		R->setHL( jdos_getc(R, mode) );
		break;

		case 0x15:	/* Write byte to stream */
		R->setHL( jdos_putc(R, mode) );
		break;

		case 0x16:
		R->setHL( jdos_creat(R, mode) ); /* Create file */
		break;

		case 105:
		jdos_time(R, 0);	/* Get the time */
		break;


		case 0x91:		/* Wildcard expansion */
		R->setHL( jdos_wildex(R, mode) );
		break;

		default:
		joyce_dprintf("Unknown call to API function 10: "
			"C=%02x DE=%04x PC=%04x\n",
			R->c, R->getDE(), R->pc);
		break;
	}
	return 0;
}

