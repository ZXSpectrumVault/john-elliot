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

  PARTEST performs diagnostic testing on the parallel port components of 
LibLink. It allows individual bits in the port to be set and read.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


/* Print false or true, depending on b. This is just to replicate the 
 * style of LLTEST.EXE */
char *ft(int b) { return b ? "true" : "false"; }


/* Show what data are being sent (o = previous output byte, n = new output 
 * byte). */
void show_data(unsigned char o, unsigned char n)
{
	if ((o ^ n) &   1) printf("OUT - Data 0 %s\n", ft(n & 1));
	if ((o ^ n) &   2) printf("OUT - Data 1 %s\n", ft(n & 2));
	if ((o ^ n) &   4) printf("OUT - Data 2 %s\n", ft(n & 4));
	if ((o ^ n) &   8) printf("OUT - Data 3 %s\n", ft(n & 8));
	if ((o ^ n) &  16) printf("OUT - Data 4 %s\n", ft(n & 16));
	if ((o ^ n) &  32) printf("OUT - Data 5 %s\n", ft(n & 32));
	if ((o ^ n) &  64) printf("OUT - Data 6 %s\n", ft(n & 64));
	if ((o ^ n) & 128) printf("OUT - Data 7 %s\n", ft(n & 128));
}

/* Show what data are being received. */
void show_octrl(unsigned o, unsigned n)
{
	if ((o ^ n) & LL_CTL_STROBE)  printf("OUT - STROBE  %s\n", ft(n & LL_CTL_STROBE));
	if ((o ^ n) & LL_CTL_AUFEED)  printf("OUT - AUFEED  %s\n", ft(n & LL_CTL_AUFEED));
	if ((o ^ n) & LL_CTL_OSELECT) printf("OUT - OSELECT %s\n", ft(n & LL_CTL_OSELECT));
	if ((o ^ n) & LL_CTL_INIT)    printf("OUT - INIT    %s\n", ft(n & LL_CTL_INIT));
}

void show_ictrl(unsigned o, unsigned n)
{
	if ((o ^ n) & LL_CTL_BUSY)    printf("IN - BUSY    %s\n", ft(n & LL_CTL_BUSY));
	if ((o ^ n) & LL_CTL_ACK)     printf("IN - ACK     %s\n", ft(n & LL_CTL_ACK));
	if ((o ^ n) & LL_CTL_NOPAPER) printf("IN - NOPAPER %s\n", ft(n & LL_CTL_NOPAPER));
	if ((o ^ n) & LL_CTL_ISELECT) printf("IN - ISELECT %s\n", ft(n & LL_CTL_ISELECT));
	if ((o ^ n) & LL_CTL_ERROR)   printf("IN - ERROR   %s\n", ft(n & LL_CTL_ERROR));

}

/* Main loop. */
void diagnose(LL_PDEV link)
{
	unsigned inctl, inctl2, outctl = 0, outctl2;
	unsigned char outdata = 0, outdata2;
	ll_error_t e;

/* Print a brief explanation of what is going on */
	printf("Press 0-7 to toggle data lines; S,A,O,I for control; Q to quit\n");
/* Kick off the process by setting data lines */
	e = ll_send(link, outdata);
	e = ll_sctl(link, outctl);
/* Initialise incoming data lines */
	e = ll_rctl_poll(link, &inctl);

/* Show initial status */
	show_data (~outdata, outdata);
	show_octrl(~outctl,  outctl);
	show_ictrl(~inctl,   inctl);

	while (1)
	{
		e = ll_rctl_poll(link, &inctl2);
/* If input lines change, show the new values */
		if (inctl2 != inctl)
		{
			show_ictrl(inctl, inctl2);
			inctl = inctl2;
		}
		outdata2 = outdata;
		outctl2  = outctl;
		switch (pollch())
		{
/* ^C or q to quit */
			case 3: case 'q': case 'Q': return;
/* "0"/"1": Change outputs */
			case '0': outdata2 ^= 1; break;
			case '1': outdata2 ^= 2; break;
			case '2': outdata2 ^= 4; break;
			case '3': outdata2 ^= 8; break;
			case '4': outdata2 ^= 16; break;
			case '5': outdata2 ^= 32; break;
			case '6': outdata2 ^= 64; break;
			case '7': outdata2 ^= 128; break;
			case 'S': case 's': outctl2 ^= LL_CTL_STROBE; break;
			case 'A': case 'a': outctl2 ^= LL_CTL_AUFEED; break;
			case 'O': case 'o': outctl2 ^= LL_CTL_OSELECT; break;
			case 'I': case 'i': outctl2 ^= LL_CTL_INIT; break;
		}
/* Output byte changed. Set control lines accordingly. */
		if (outdata2 != outdata)
		{
			show_data(outdata, outdata2);		
			ll_send(link, outdata2);
			outdata = outdata2;
		}
		if (outctl2 != outctl)
		{
			show_octrl(outctl, outctl2);		
			ll_sctl(link, outctl2);
			outctl = outctl2;
		}
	}
}

int help(char **argv)
{
	fprintf(stderr, "Syntax: \n");
	fprintf(stderr, "    %s { -device <dev> } { -driver <drv> }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s -device /dev/parport0 -driver parport\n", argv[0]);
	fprintf(stderr, "       - Exercise the first parallel port.\n\n");

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
//	char *port   = "unix:S:/tmp/locolink.socket";
//	char *driver = "parsocket";
	char *port   = "/dev/parport0";
	char *driver = "parport";
	int n;
/* Find out what device to open. */
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help")) return help(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);

		if (n == (argc - 1)) continue;	
		if (!strcmp(argv[n], "--driver")) { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "-driver"))  { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "--device")) { ++n; port   = argv[n]; }
		if (!strcmp(argv[n], "-device"))  { ++n; port   = argv[n]; }
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
	diagnose(link);

/* Close the link */
	e = ll_close(&link);
	if (e) fprintf(stderr, "ll_close: %s\n", ll_strerror(e));

	return 0;
}
