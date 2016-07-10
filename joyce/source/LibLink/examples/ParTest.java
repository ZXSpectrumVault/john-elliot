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

import uk.co.demon.seasip.liblink.*;


class ParTest
{
	static char pollch()
	{
		try {
			if (System.in.available() > 0) return (char)System.in.read();
			}
		catch (java.io.IOException e) {}	
		return 0;
	}

/* Print false or true, depending on b. This is just to replicate the 
 * style of LLTEST.EXE */
	static String ft(int b) { return (b != 0) ? "true" : "false"; }


/* Show what data are being sent (o = previous output byte, n = new output 
 * byte). */
	static void show_data(byte o, byte n)
	{
		if (0 != ((o ^ n) &   1)) System.out.println("OUT - Data 0 " + ft(n & 1));
		if (0 != ((o ^ n) &   2)) System.out.println("OUT - Data 1 " + ft(n & 2));
		if (0 != ((o ^ n) &   4)) System.out.println("OUT - Data 2 " + ft(n & 4));
		if (0 != ((o ^ n) &   8)) System.out.println("OUT - Data 3 " + ft(n & 8));
		if (0 != ((o ^ n) &  16)) System.out.println("OUT - Data 4 " + ft(n & 16));
		if (0 != ((o ^ n) &  32)) System.out.println("OUT - Data 5 " + ft(n & 32));
		if (0 != ((o ^ n) &  64)) System.out.println("OUT - Data 6 " + ft(n & 64));
		if (0 != ((o ^ n) & 128)) System.out.println("OUT - Data 7 " + ft(n & 128));
	}

/* Show what data are being received. */
	static void show_octrl(int o, int n)
	{
		if (0 != ((o ^ n) & LibLink.LL_CTL_STROBE))  System.out.println("OUT - STROBE  %s" + ft(n & LibLink.LL_CTL_STROBE));
		if (0 != ((o ^ n) & LibLink.LL_CTL_AUFEED))  System.out.println("OUT - AUFEED  %s" + ft(n & LibLink.LL_CTL_AUFEED));
		if (0 != ((o ^ n) & LibLink.LL_CTL_OSELECT)) System.out.println("OUT - OSELECT %s" + ft(n & LibLink.LL_CTL_OSELECT));
		if (0 != ((o ^ n) & LibLink.LL_CTL_INIT))    System.out.println("OUT - INIT    %s" + ft(n & LibLink.LL_CTL_INIT));
	}

	static void show_ictrl(int o, int n)
	{
		if (0 != ((o ^ n) & LibLink.LL_CTL_BUSY))    System.out.println("IN - BUSY    %s" + ft(n & LibLink.LL_CTL_BUSY));
		if (0 != ((o ^ n) & LibLink.LL_CTL_ACK))     System.out.println("IN - ACK     %s" + ft(n & LibLink.LL_CTL_ACK));
		if (0 != ((o ^ n) & LibLink.LL_CTL_NOPAPER)) System.out.println("IN - NOPAPER %s" + ft(n & LibLink.LL_CTL_NOPAPER));
		if (0 != ((o ^ n) & LibLink.LL_CTL_ISELECT)) System.out.println("IN - ISELECT %s" + ft(n & LibLink.LL_CTL_ISELECT));
		if (0 != ((o ^ n) & LibLink.LL_CTL_ERROR))   System.out.println("IN - ERROR   %s" + ft(n & LibLink.LL_CTL_ERROR));
	}

	/* Main loop. */
	static void diagnose(LibLink link) throws LibLinkException
	{
		int inctl, inctl2, outctl = 0, outctl2;
		byte outdata = 0, outdata2;

/* Print a brief explanation of what is going on */
		System.out.println("Press 0-7 to toggle data lines; S,A,O,I for control; Q to quit");
/* Kick off the process by setting data lines */
		link.send(outdata);
		link.setControl(outctl);
/* Initialise incoming data lines */
		inctl = link.readControlPoll();

/* Show initial status */
		show_data ((byte)(~outdata), outdata);
		show_octrl(~outctl,  outctl);
		show_ictrl(~inctl,   inctl);

		while (true)
		{
			inctl2 = link.readControlPoll();
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
				case 'S': case 's': outctl2 ^= LibLink.LL_CTL_STROBE; break;
				case 'A': case 'a': outctl2 ^= LibLink.LL_CTL_AUFEED; break;
				case 'O': case 'o': outctl2 ^= LibLink.LL_CTL_OSELECT; break;
				case 'I': case 'i': outctl2 ^= LibLink.LL_CTL_INIT; break;
			}
/* Output byte changed. Set control lines accordingly. */
			if (outdata2 != outdata)
			{
				show_data(outdata, outdata2);		
				link.send(outdata2);
				outdata = outdata2;
			}
			if (outctl2 != outctl)
			{
				show_octrl(outctl, outctl2);		
				link.setControl(outctl2);
				outctl = outctl2;
			}
		}
	}

	static int help()
	{
		System.err.println( "Syntax: \n");
		System.err.println( "   java ParTest { -device <dev> } { -driver <drv> }");
		System.err.println();
		System.err.println( "For example:");
		System.err.println( "    java ParTest -device /dev/parport0 -driver parport");
		System.err.println( "       - Exercise the first parallel port.");

// XXX enumerate drivers
		return 0;
	}

	static int version()
	{
		System.err.println( "liblink version" + LibLink.getVersion());
		return 0;
	}

	public static void main(String args[])
	{
		LibLink link;
		String port   = "/dev/parport0";
		String driver = "parport";
		int n;
/* Find out what device to open. */
		for (n = 0; n < args.length; n++)
		{
			if (args[n].equals("--help"))    { help(); return; }
			if (args[n].equals("--version")) { version(); return; }

			if (n == (args.length - 1)) continue;	
			if (args[n].equals("--driver")) { ++n; driver = args[n]; }
			if (args[n].equals("-driver"))  { ++n; driver = args[n]; }
			if (args[n].equals("--device")) { ++n; port   = args[n]; }
			if (args[n].equals("-device"))  { ++n; port   = args[n]; }
		}

		System.out.println("Driver to use = " + driver);
		System.out.println("Device to use = " + port);

/* Open the link */
		try
		{
			link = LibLink.open(port, driver);
/* Enter main loop */
			diagnose(link);
/* Close the link */
			link.close();
		}	
		catch (LibLinkException e)	
		{
			System.err.println(e.getMessage() );	
		}
	}
}
