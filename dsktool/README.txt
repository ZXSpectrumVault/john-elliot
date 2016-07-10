DSKTOOL v1.0.5                                    3 January 2016, John Elliott
==============================================================================

  DSKTOOL is a front-end to the LibDsk library, replicating its three main
sample programs with a graphical interface. The three functions DSKTOOL can 
perform are:

1. Identify a disc or disc image file.

2. Format a disc / create a blank disc image file.

3. Copy a disc or image file to another disc or image file.

Installation under Windows
==========================
  Extract dsktool.exe and libdsk.dll to a folder on your hard drive (assuming
you haven't done so already). Then set up a shortcut to it from your desktop,
start menu or wherever else takes your fancy.
  If you're using Windows 2000 or XP, I recommend you install FDRAWCMD
<http://simonowen.com/fdrawcmd>.
  The two '.tar.gz' files are the source code of dsktool.exe and libdsk.dll.
You don't need to do anything with them in order to use DSKTOOL, but if you
distribute DSKTOOL you should include them. See the file COPYING for more 
details.

Installation under Linux
========================
* Install wxWidgets (I used v2.8.10).
* Install LibDsk (you should be able to get it from wherever you found 
Dsktool). 
* It should be sufficient to type 'make' in the Dsktool directory; to 
install, su to root and type 'make install'. If you need to set up special
compiler flags, edit the Makefile.

In Use
======
  Select the function you want from the "Disc" menu - Identify, Format or
Copy. Each function starts out with a wizard in which you choose the file or
drive to operate on.

Drive Chooser Screen
====================

    +------------------------------------------------------------------+
    |              Select drive / disc image:                          |
    |                                                                  |
    |                 ________________________[v]  [Browse]            |
    |                                              [Serial]            |
    |              Drive / file type:                                  |
    |                                                                  |
    |                 _Auto_detect_____________[v]                     |
    |                                                                  |
    |              Compression:                                        |
    |                                                                  |
    |                 _Auto_detect_____________[v]                     |
    |                                                                  |
    |              Override format:                                    |
    |                                                                  |
    |                 _Auto_detect_____________[v]                     |
    |                                                                  |
    |              +-Floppy drive options---------------------------+  |
    |              |                                                |  |
    |              | _Auto_Sides_[v]_  [ ] Double-step  _1_ retries |  |
    |              |                                                |  |
    |              +------------------------------------------------+  |
    +------------------------------------------------------------------+

  On this screen:

  "Select drive / disc image": You can either browse for an image file 
                          (using the 'Browse' button), type its name in this
                          field, or select a floppy drive from the drop list.
                          Click the 'Serial' button to set up a connection 
                          to another computer via a serial line (the other
                          computer should be running the example LibDsk 
                          program 'serslave' or similar).

  "Drive / file type": When identifying or copying from a file, Dsktool can 
                      usually auto-detect what type of file it is. When 
                      formatting or copying to a file, you must select a 
                      type. The types supported are:
                        * rcpmfs: Reverse CP/M filesystem; a directory on
                         the PC which behaves like a CP/M diskette. 
                        * floppy: The floppy drive under Linux or Windows.
			* remote: A computer at the other end of a serial 
                         connection, running the LibDsk example program 
                         'serslave' or a similar program.
                        * ntwdm: Simon Owen's alternative floppy driver; 
                         if you have this installed, use it in preference
                         to the standard Windows one.
                        * dsk: CPCEMU 'DSK' format, used by various Amstrad 
                         and Sinclair emulators.
                        * edsk: Extended CPCEMU 'DSK' format.
                        * apridisk: Apridisk format, used to archive 
                         floppies from the Apricot PC series.
                        * qm: CopyQM file (read only).
                        * raw: Flat file containing all sectors; also the 
                         floppy drive under systems other than Windows and 
                         Linux.
                        * myz80: Hard drive image from the MYZ80 emulator.
                        * nanowasp: Floppy image from the Nanowasp emulator.
                        * cfi: A compressed disc image format used to 
                         distribute some old Amstrad system discs.

  "Compression":         Deals with transparently compressed disc images.
                        As with the file type above, this is normally 
                        auto-detected when reading or identifying, and 
                        should be set explicitly when writing. Compression
                        methods are:
                        * No compression: File is not compressed.
                        * sq: Huffman coded, aka squeezed.
                        * gz: GZipped (not available on Windows).
                        * bz2: BZip2 compressed (read only, not available 
                              on Windows).

  "Override format":     As with all the other options, Dsktool will try to
                        detect the disc format used by the disc or image file
                        it's reading. If you want to override it, choose
                        one of the formats from this drop list. When you are
                        formatting a disc, it's compulsory to select a 
                        format here.

  "Floppy drive options": These options don't make much sense on disc images,
                        but do if you're reading a floppy disc. They are:
                        > "Auto Sides" / "Side 1" / "Side 2": Use this to 
                          force a particular side of a disc to be read. This
                          is mainly used if the disc in question was written
                          on a machine with a side switch.
                        > "Double-step": Tick this if you're reading a 360k
                          5.25" diskette in a 1.2Mb drive, or a 180k 3" 
                          diskette in a 720k 3" drive. Note that it isn't
                          a very good idea to write double-stepped discs.
                        > "Retries": The number of times Dsktool should try
                          if it encounters an error. Sometimes repeated reads
                          can get you past an error.

Operations
==========

If you are identifying or copying a diskette, you will be shown the drive
chooser screen once; when you click 'Finish', the action will be performed.

If you are copying from one drive/file to another, you will see this screen
twice -- once for the drive/file to read, and once for the drive/file to
write. There will then be a third screen with options; unless you want to
do something special, leave these with their default settings.

* Format while copying: This should normally be ticked. You may optionally 
 untick it if you are writing to a real floppy disc that is already formatted 
 in the correct format.

* MicroDesign 3 copy protection: Copy a MicroDesign 3 disc, bypassing its
 copy protection scheme. Note that this does not make Dsktool a circumvention
 device, since the authors of MicroDesign have placed it in the public domain 
 and given permission for the copy-protection to be reverse engineered; I
 posted their original press release to USENET as
 <1008359853.26849.0.nnrp-13.c2de7091@news.demon.co.uk>.

* Reorder logical sectors: For use with disc formats such as Personal CP/M,
 in which the order of sectors on disc does not match the one commonly used
 by the PC. This should be used in conjunction with the 'raw' output format,
 to obtain a file in which the sectors are arranged the way their original 
 host computer saw them.

* Rewrite Apricot boot sector: Given a floppy in Apricot MS-DOS format, 
 convert its boot sector to standard PC-DOS format. This should be used
 in conjunction with the 'raw' output format to produce a file which can 
 be loopback-mounted under Linux.

* Partial copy: Copies only a range of cylinders. This can be used to 
 extract data from discs with hard errors, or with different formats in
 different parts of the disc. 

The Legal Bits
==============
    Copyright (C) 2005, 2007, 2010  John Elliott <jce@seasip.demon.co.uk>

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

