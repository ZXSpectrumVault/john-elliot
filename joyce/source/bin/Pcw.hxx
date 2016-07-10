/*************************************************************************

    JOYCE v2.2.9 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2007,2012-14,2016 John Elliott <seasip.webmaster@gmail.com>

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

#ifndef PCW_H		    /* Protect against multiple inclusion */

/* Standard libraries */

#include "config.h"	/* Autoconf header */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

/* STL includes */

#include <vector>
#include <string>

using namespace std;

// The XLOG macro allows me to do printf()-style debugging of the 
// PcwSystem::PcwSystem construct phase.
#define XLOG(s)

// This simply appends messages to a file
// #define XLOG(s) do { FILE *fp = fopen("xlog", "a"); fprintf(fp, "%s\n", s); fclose(fp); } while(0)
//
// This version of XLOG is provided to try and smoke out memory allocation
// bugs, by allocating & freeing memory.
//
//#define XLOG(s) do { FILE *fp = fopen("xlog", "a"); \
//		    fprintf(fp, "%s...", s); fclose(fp); \
//		    std::string *str[8000]; int n; \
//		    for (n = 0; n < 8000; n++) str[n] = new std::string(); \
//		    for (n = 0; n < 8000; n++) delete str[n]; \
//		    fp = fopen("xlog", "a"); \
//		    fprintf(fp, "OK\n", s); fclose(fp); } while(0)
//
#ifdef HAVE_WINDOWS_H
#include "PcwW32.hxx"
#else
#include "PcwUnix.hxx"
#endif

// XML parser for config files
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

// LibPng for graphical output
#include <png.h>

// Code to launch processes asynchronously in Win32 and Unix
#include "Executor.hxx"

#include "UiTypes.hxx"		/* Enums for user interface events */
#include "PcwArgs.hxx"	/* Argument parser */

// PCW model types. There were in fact 6 PCWs, but JOYCE treats the 
// (8256,8512) and (9256,10) pairings as identical; the differences
// between them are configured manually.
//
enum PcwModel
{
	PCW8000,	// PCW8256. PCW8512
	PCW9000,	// PCW9512
	PCW9000P,	// PcW9512+
	PCW10,		// PcW9256, PcW10
	PCW16		// PcW16
};

class PcwSystem;

#include "Z80.hxx"              /* Z80 emulation declarations    */
#include "libdsk.h"		/* Low-level drive access        */
#include "765.h"	        /* FDC emulation declarations    */
#include "liblink.h"		/* Parport/LocoLink emulation 	 */
#include "PcwComms.hxx"		/* Wrapper for comms code        */
// New device model
#include "PcwDevice.hxx"        /* Generic device class          */

#include "PcwMemory.hxx"	/* The memory sybsystem 	 */
#include "PcwAsic.hxx"	
#include "PcwFdc.hxx"		/* The PCW's disc subsystem      */
#include "PcwHdc.hxx"		/* Emulated hard drives          */
#include "PcwBeeper.hxx"	/* The beeper                    */
// Input
#include "PcwInput.hxx"		/* Generic input interface       */
#include "PcwJoystick.hxx"	/* Joystick input                */

// Screen stuff
#include "PcwSdlContext.hxx"	/* Our output window             */
#include "PcwTerminal.hxx"      /* Generic terminal              */
#include "PcwSdlTerm.hxx"	/* Generic SDL text terminal     */
#include "PcwVgaTerm.hxx"	/* VGA: SDL-based console        */
#include "PcwLogoTerm.hxx"	/* Logo: DR Logo driver          */
#include "PcwDebugTerm.hxx"	/* Debugger console              */
#include "PcwMenuTerm.hxx"	/* Menu system                   */
#include "PcwGSX.hxx"		/* GSX support (-lSDL_Gfx)       */


// Printers
#include "PcwLibLink.hxx"       /* LibLink I/O and UI            */
#include "PcwPrinter.hxx"       /* Basic printer emulation       */
#include "PcwSystem.hxx"	/* The PCW itself                */

#define BCDVERS 0x0229	/* v2.2.9 */


//
// Enum describing the different directories JOYCE uses:
//
enum PcwFileType
{
	FT_NORMAL,	// JOYCE root
	FT_BOOT,	// boot discs
	FT_DISC,	// non-boot discs
	FT_PNGS,	// PNG output files
	FT_OLDBOOT,	// JOYCE 1.3x boot discs
	FT_OLDDISC,	// JOYCE 1.3x other discs
	FT_PS,		// Postscript output files
	FT_ANNEFSROOT	// Root of emulated PCW16 'floppy' storage area
			// (see AnneAccel.cxx for the application of this)
};


/////////////////////////////////////////////////////////////////////////////
//
// JOYCE routines: 
//

//
// Locate a file. Mode is an fopen() mode. 
//
std::string findPcwFile(PcwFileType ft, const std::string name, const std::string mode);
FILE  *openPcwFile(PcwFileType ft, const std::string name, const std::string mode);
std::string findPcwDir (PcwFileType ft, bool global);

//
// Shorten a filename for display
//
std::string displayName (std::string filename, int len);
//
// Open a temporary file
//
FILE *openTemp(char *name, const std::string type, const std::string mode);
//
// Check a directory exists
//
void checkExistsDir(const std::string path);

// Get user's home directory ($HOME in Unix, "My Documents" in Windows)
std::string getHomeDir(void);
// Set user's home directory (Windows only)
void setHomeDir(std::string s);

void joyce_dprintf(const char *, ...); /* Printf() to debugger output */
void joyce_vdprintf(const char *, va_list);	/* vprintf() to debugger output */
void dgets(char *S, int maxc);	/* Get string for debugger */
char dgetch(void);		/* Get character */
void dcls(void);		/* Clear debug console screen */

void goodbye(int);		/* Exit tidily */
void diewith(const char *, int);	/* Exit tidily with an alert */
void alert(const std::string s);	/* On UNIX, warn to stderr. Under Windows,
				 * do a MessageBox */

void fid_com(Z80 *R);		/* (v1.21) COM port handler */
void set_com(void);		/* (v1.22) COM port reinitialise */
void com_init(void);		/* (v1.31) COM port startup */
void com_deinit(void);		/* (v1.31) COM port shutdown */

void boot_id(Z80 *R);		/* (v1.22) parse BOOT.CFG */

char *name_from_path(char *s);

void UpdateArea(int x, int y, int w, int h);

/* Interface to CPMREDIR library */
void cpmredir_intercept(Z80 *R);

#ifdef MAIN	/* Data storage classes - extern unless in main program */
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN PcwSystem *gl_sys;
EXTERN FILE *fplog;		/* Log file */
/* (v1.31) Requested by external agency to stop */
EXTERN int extern_stop;

#include "colours.hxx"

#define PCW_H 1



#endif /* ndef PCW_H */
