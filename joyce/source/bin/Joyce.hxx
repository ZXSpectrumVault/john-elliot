/*************************************************************************

    JOYCE v2.1.5 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2004  John Elliott <seasip.webmaster@gmail.com>

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

#ifndef JOYCE_H 

#define JOYCE_H 1
#include "JoyceArgs.hxx"

#include "JoyceZ80.hxx"		/* The CPU */
#include "JoyceAsic.hxx"	/* Ports 0xF4 and 0xF8 */
#include "JoyceFdc.hxx"		/* Floppy controller */
#include "JoyceMemory.hxx"	/* The RAM */
#include "JoycePcwTerm.hxx"	/* The screen */
#include "JoyceLocoLink.hxx"	/* LocoLink interface */
#include "JoyceMenuTerm.hxx"	/* The menu system */
#include "JoyceMenuTermPcw.hxx"	/* The code that draws menus in PCW style */
/* Input */
#include "JoycePcwKeyboard.hxx" /* Generic PCW keyboard          */
#include "JoycePcwMouse.hxx"    /* PCW mice                      */
#include "JoyceJoystick.hxx"	/* Joysticks (other than keyboard sticks) */

/* Printers */
#include "JoyceMatrix.hxx"	/* 8256 matrix printer           */
#include "JoyceDaisy.hxx"	/* 9512 daisywheel printer       */
#include "JoyceCenPrinters.hxx" /* CEN: Centronics printer       */
#include "JoyceCPS.hxx"		/* CPS8256: Centronics + serial  */

/* The complete system */
#include "JoyceSystem.hxx"

/* file names */
#define BOOT_SYS        "bootfile.emj"  /* boot image of CP/M system    */
#define LOGFILE         "joyce.log"     /* Log file */

#endif /* ndef JOYCE_H */
