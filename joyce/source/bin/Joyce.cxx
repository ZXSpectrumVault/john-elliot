/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

/*
 * Note: JOYCE may well contain assumptions that an int is 32 bits or more.
 * I wouldn't recommend porting it to a 16-bit system - the ability
 * to do a malloc(2megs) is quite important :-)
 *
 */
#define MAIN
#include "Joyce.hxx"
#include "PcwFid.hxx"
#include "PcwBdos.hxx"
#include "inline.hxx"

void deinitJoyce(void);

/* Argument processing */






/******************* main() *******************/

void initJoyce(void)
{
	fplog = NULL;

        fplog = openPcwFile(FT_NORMAL, LOGFILE, "w");
	if (!fplog) fplog = fopen(LOGFILE, "w");
	if (!fplog) fplog = stderr;
}


static void error_function(int debuglevel, char *fmt, va_list ap)
{
        /* Let's say default action is level 1; showing all messages
         * would be just too horribly disturbing. */

        if (debuglevel > 1) return;
        vfprintf( fplog, fmt, ap );
}




int
main(int argc,char *argv[])
{
	JoyceSystem *sys;
	JoyceArgs args(argc, argv);

        printf("JOYCE PCW Emulator v%d.%d.%d\n", BCDVERS >> 8, 
				(BCDVERS & 0xF0) >> 4, BCDVERS & 0x0F);
	fflush(stdout);
	
	if (!args.parse()) return false;

	// Trap signal to exit gracefully
	signal(SIGINT, goodbye);

	/* kick off CP/M */
	XLOG("Creating JoyceSystem");
	gl_sys = sys = new JoyceSystem(&args);
	initJoyce();		
        lib765_register_error_function(error_function);
	atexit(deinitJoyce);

	XLOG("Calling sysInit()");
	if (sys->sysInit())
	{
		// Set the Z80 going
		sys->m_cpu->mainloop();
	}

	/* we should exit through the goodbye() function
	 * to restore terminal modes and close disk drives
	 */
	goodbye(99);

	/* The logic will never get here */
	return 0;
}




/* signal trap - reset tty modes and leave program
 * 99 is used if the program wants to exit gracefully,
 */
void goodbye(int sig)
{
	if (fplog)
	{
		if (sig != 99) fprintf(fplog, "Stopped by signal: %d\n", sig);
		if (fplog != stderr) fclose(fplog);
		fplog = NULL;
	}
	exit(sig);
}


/* As goodbye(), but prints a farewell message */

void diewith(const char *s, int sig)
{
	alert(s);
	if (fplog)
	{
		fprintf(fplog, "Fatal error %d: %s", sig, s);
		if (fplog != stderr) fclose(fplog);
		fplog = NULL;
	}
	exit(sig);
}



void deinitJoyce(void)	/* v1.21 The very last things done */
{
        if (gl_sys) 
        {
                gl_sys->deinitVideo();
                delete gl_sys;
        }
}





