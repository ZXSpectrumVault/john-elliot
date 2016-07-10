LibDsk and CP/M
---------------

  AUXD has been provided as a remote LibDsk server. The current version of
AUXD has these limitations:

* Can only communicate through the CP/M AUX: device (PUN: / RDR: under CP/M 2).
* Requires 56k or greater TPA.

How to use AUXD:

  AUXD has been tested on the Amstrad PCW version of CP/M Plus, and under
CP/M 2 running in the MYZ80 emulator. 

  To set the PCW up for AUXD:

1. Set up the hardware connection.

    The PCW will need a CPS8256 serial interface, or compatible. 

    You can connect it to the PC either with a serial LapLink (null modem) 
  cable or with a custom cable. If you are using a custom cable, you should 
  set up the connections as follows:

	PCW Pin			PC Pin (25 way)		PC Pin (9 way) 
	2 TxD			3 RxD			2 RxD
	3 RxD			2 TxD			3 TxD
	5 CTS			4 RTS			7 RTS
	7 Ground		7 Ground		5 Ground
	20 DTR			5 CTS			8 CTS 

    Doing this means that the PCW can use DTR and CTS as the handshake lines,
  and the PC can use RTS and CTS. Note that this is NOT the same pinout as 
  a normal Laplink or Null-Modem cable; if you use one of these, a patch 
  needs to be used at the PCW end.

2. On the PCW, set up the serial port, with a command like:

	SETSIO 9600,H ON,INTERRUPT ON

    The important thing is that hardware handshaking and interrupt mode should
  both be turned on. AUXD was tested at 9600 baud; slower speeds like 2400 
  should also work. 19200 probably won't.

    On other systems, you will need to set up the serial port to use 8 bits,
  1 stop bit, and no parity. If possible, hardware handshaking should be 
  enabled; if this isn't possible, you'll probably need to drop the connection 
  speed so that the CP/M computer doesn't drop bytes.

3. Load the RTS/CTS patch.

    If you are using a normal Laplink cable, PCW CP/M needs to be patched to
  handshake using RTS/CTS rather than DTR/CTS. To do this, run CRTSCTS.COM
  on this disc. You should see the message: "The system was patched.". 
  CRTSCTS should be run once before AUXD is launched; it patches the running 
  system but does not alter the .EMS / .EMT file on disc.

4. Launch AUXD. There are two versions - one for Amstrad CP/M and one for 
  generic CP/M. On the PCW, CPC or Spectrum +3, use

	AUXDPCW

  On other systems, use

	AUXDCPM

    You may optionally add "-V" to the command. If present, this causes AUXD
  to print a line of text for each operation it performs (such as open drive,
  read sector and so forth).

    Once AUXD is running (displaying its "Idle." message), you can remove 
  the disc you loaded it from. 

5. Use LibDsk on the remote machine. 

    Some example commands are:

	dskid serial:/dev/ttyS0,2400-crtscts,a:
	dsktrans serial:/dev/ttyS1,4800+crtscts,a: a.dsk
	dsktrans -otype remote b.dsk serial:COM2:,9600,b: 

  The syntax used by filenames here is:

	serial:<port>,<baud>{<crtscts>},<cpmdrive>

  where: 

<port> is the name of the serial port at the PC end.
<baud> is the speed of the connection to use.
<crtscts> controls whether to use RTS/CTS handshaking. It should be either
         "+crtscts" (on) or "-crtscts" (off). If it isn't present, then 
         it is assumed to be turned on.
<cpmdrive> The drive to access on the remote machine.
