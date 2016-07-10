/* liblink: Simulate parallel / serial hardware for emulators
 
    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*****************************************************************************

  LLTEST is designed to behave like LLTEST.EXE, the diagnostic program 
supplied with LocoLink. Unlike the original LLTEST.EXE (which could only act
as a master), LLTEST can be a master or a slave.

  What LLTEST actually does is sit in a loop polling the keyboard and the 
parallel port. Changes on the incoming LocoLink data lines are displayed; 
while keypresses can be used to toggle the outgoing data lines. 

  A short discussion of LocoLink is perhaps appropriate at this point. The 
original LocoLink hardware attached to an Amstrad PCW 8000/9000/10 series
computer at one end (the "slave"), and to a standard parallel port at the 
other (the "master"). Four data lines were used; BUSY and ACK were set by the 
slave and read by the master, while DATA 0 and DATA 1 went the other way. 

  LibLink doesn't run on a PCW; however, it does run on computers that have
parallel ports. In order for such a computer to be a LocoLink slave, it 
needs to be connected to the master using a crossover cable. 
  The ideal LocoLink crossover cable would map BUSY to DATA 0 and ACK to 
DATA 1, in both directions. This would mean that the same bits could be set/
read on the master and the slave. However, there is no such cable (and even 
if you made one, LibLink wouldn't support it).
  The standard parallel crossover cable uses the LapLink pinout. This uses
the following mappings:

DATA 0 <--> ERROR
DATA 1 <--> SELECT
BUSY   <--> DATA 4
ACK    <--> DATA 3
  
  The wires used by the master can't change (since it has to maintain 
compatibility with a true LocoLink system) so the slave has to use the pins
that the LapLink cable demands. To be precise, it has to read data from 
ERROR and SELECT, and write to DATA 3 and DATA 4. 
  Because the master and slave use different data/control lines, the program
has to alter its behaviour depending whether it's acting as a master or a 
slave. Hence the "--master" and "--slave" options.

  Note also that the "master/slave" setting is completely independent of the 
"server/client" setting used by the "parsocket" driver. 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


/* Print false or true, depending on b. This is just to replicate the 
 * style of LLTEST.EXE */
char *ft(int b) { return b ? "true" : "false"; }


/* Show what data are being sent (o = previous output byte, n = new output 
 * byte). The names of the data lines are shown from the point of view of
 * the master; so a line saying "OUT - Data 0 true" on a master will be
 * matched by a line saying "IN - Data 0 true" on the slave. */
void show_out(int master, unsigned char o, unsigned char n)
{
	if (master)
	{
		if ((o ^ n) & 1) printf("OUT - Data 0 %s\n", ft(n & 1));
		if ((o ^ n) & 2) printf("OUT - Data 1 %s\n", ft(n & 2));
	}
	else
	{
		if ((o ^ n) & 1) printf("OUT - Busy %s\n", ft(n & 1));
		if ((o ^ n) & 2) printf("OUT - Ack %s\n", ft(n & 2));
	}
}

/* Show what data are being received. Again, the control line names are as
 * the master sees them. */
void show_in(int master, unsigned char o, unsigned char n)
{
	if (master)
	{
		if ((o ^ n) & 1) printf("IN - Busy %s\n", ft(n & 1));
		if ((o ^ n) & 2) printf("IN - Ack %s\n", ft(n & 2));
	}
	else
	{
		if ((o ^ n) & 1) printf("IN - Data 0 %s\n", ft(n & 1));
		if ((o ^ n) & 2) printf("IN - Data 1 %s\n", ft(n & 2));
	}
}

/* Main loop. */
void diagnose(LL_PDEV link, int master)
{
	unsigned char data, data2;
	unsigned char outdata = 0, outdata2;
	ll_error_t e;

/* Print a brief explanation of what is going on */
	if (master)
		printf("Master mode: Press 0, 1 to toggle data lines; Q to quit\n");
	else	printf("Slave mode:  Press A, B to toggle data lines; Q to quit\n");
/* Kick off the process by setting data lines */
	e = ll_lsend(link, master, outdata);
/* Initialise incoming data lines */
	e = ll_lrecv_poll(link, master, &data);

/* Show initial status */
	show_out(master, ~outdata, outdata);
	show_in (master, ~data,    data);
	while (1)
	{
		e = ll_lrecv_poll(link, master, &data2);
/* If input lines change, show the new values */
		if (data2 != data) 
		{
			show_in(master, data, data2);
			data = data2;
		}
		outdata2 = outdata;
		switch (pollch())
		{
/* ^C or q to quit */
			case 3: case 'q': case 'Q': return;
/* "0"/"1": Change outputs for a master */
			case '0': outdata2 ^= 1; break;
			case '1': outdata2 ^= 2; break;
/* "A"/"B": Change outputs for a slave */
			case 'B': case 'b': outdata2 ^= 1; break;
			case 'A': case 'a': outdata2 ^= 2; break;
		}
/* Output byte changed. Set control lines accordingly. */
		if (outdata2 != outdata)
		{
			show_out(master, outdata, outdata2);		
			ll_lsend(link, master, outdata2);
			outdata = outdata2;
		}
	}
}

int help(char **argv)
{
	fprintf(stderr, "Syntax: \n");
	fprintf(stderr, "    %s -master { -device <dev> } { -driver <drv> }\n", argv[0]);
	fprintf(stderr, "    %s -slave  { -device <dev> } { -driver <drv> }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s -master -device /dev/parport0 -driver parport\n", argv[0]);
	fprintf(stderr, "       - Be a LocoLink master on the first parallel port.\n\n");
	fprintf(stderr, "    %s -slave -device unix:S:/tmp/locolink.socket -driver parsocket\n", argv[0]);
	fprintf(stderr, "       - Be a LocoLink slave.\n");
	fprintf(stderr, "       - Listen for UNIX-domain connections on the socket /tmp/locolink.socket\n\n");
	fprintf(stderr, "    %s -master -device unix:C:/tmp/locolink.socket -driver parsocket\n", argv[0]);
	fprintf(stderr, "       - Be a LocoLink master.\n");
	fprintf(stderr, "       - Connect to the slave listening on /tmp/locolink.socket\n");

// XXX enumerate drivers
	return 0;
}

int version(char **argv)
{
	fprintf(stderr, "liblink version %s\n", VERSION);
	return 0;
}


int main(int argc, char **argv)
{
	LL_PDEV link;
	ll_error_t e;
	char *port   = "unix:S:/tmp/locolink.socket";
	char *driver = "parsocket";
//	char *port   = "/dev/parport0";
//	char *driver = "parport";
	int master = -1;
	int n;
/* Find out what device to open. */
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "-slave"  )) master = 0;
		if (!strcmp(argv[n], "--slave" )) master = 0;
		if (!strcmp(argv[n], "-master" )) master = 1;
		if (!strcmp(argv[n], "--master")) master = 1;
		if (!strcmp(argv[n], "--help")) return help(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);

		if (n == (argc - 1)) continue;	
		if (!strcmp(argv[n], "--driver")) { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "-driver"))  { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "--device")) { ++n; port   = argv[n]; }
		if (!strcmp(argv[n], "-device"))  { ++n; port   = argv[n]; }
	}

	if (master == -1)
	{
		fprintf(stderr, "You must specify --master or --slave\n");
		return 1;
	}
	printf("Driver to use = %s\n", driver);
	printf("Device to use = %s\n", port);

/* Open the link */
	e = ll_open(&link, port, driver);
	
	if (e)
	{
		fprintf(stderr, "ll_open: %s\n", ll_strerror(e));	
		return 1;
	}
/* Enter main loop */
	diagnose(link, master);

/* Close the link */
	e = ll_close(&link);
	if (e) fprintf(stderr, "ll_close: %s\n", ll_strerror(e));

	return 0;
}
