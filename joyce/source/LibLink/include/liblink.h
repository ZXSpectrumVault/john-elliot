#ifndef LIBLINK_H_INCLUDED

#define LIBLINK_H_INCLUDED 1

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

    liblink is intended for use in emulators, to simulate ports that can 
    be used to interface with other hardware. The intention is that it 
    should be able to simulate:
    
    * Parallel ports:
    	> Real parallel ports
	> Printing to file
	> Printing to a pipeline
	> Simulated LocoLink (or LapLink?) with other processes
    * Serial ports:
    	> Real serial ports
	> I/O to file
	> Loopback (eg, to simulate a serial mouse)
	> Simulated serial links to other processes

    Using the LibDsk approach of opening (filename, drivername), we have:

	"/dev/parport0",   "parport"
	"/foo/bar",        "parfile"
	"/bin/lpr",	   "parpipe"
	"tcp:S:localhost:8256", "parsocket"
	"/dev/ttyS0",      "serport"  (or "COM1:", "serport")
	"/foo/bar",        "serfile"	// need 2 filenames
	NULL,		   "serloop"
	"tcp:C:localhost:8512", "sersocket"

    Using Java syntax, we have:

	class Device
	class ParDevice extends Device
	class SerDevice extends Device
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef int ll_error_t;
struct ll_device;
struct ll_pardev;
struct ll_serdev;
typedef struct ll_device *LL_PDEV;
typedef struct ll_pardev *LL_PPAR;
typedef struct ll_serdev *LL_PSER;

/* Open up a device. Returns LL_E_OK if OK, else an error. 
 * On return, "self" will be a device. */
ll_error_t ll_open(LL_PDEV *self, const char *filename, const char *devtype);

/* Close the link. */
ll_error_t ll_close(LL_PDEV *self);

/* Flush any pending bytes */
ll_error_t ll_flush(LL_PDEV self);

/* Read a byte from the link, asynchronously. */
ll_error_t ll_recv_poll(LL_PDEV self, unsigned char *value);
/* Read a byte from the link, synchronously. */
ll_error_t ll_recv_wait(LL_PDEV self, unsigned char *value);
/* Send a byte down the link. */
ll_error_t ll_send(LL_PDEV self, unsigned char value);
/* Send a byte down the link, toggling the STROBE line and delaying if 
 * necessary */
ll_error_t ll_send_byte(LL_PDEV self, unsigned char value);

/* Control lines */
/* Parallel port outgoing :*/
#define LL_CTL_STROBE     1	/* STROBE */
#define LL_CTL_AUFEED     2	/* AUTO FEED XT */
#define LL_CTL_OSELECT    4	/* SELECT */
#define LL_CTL_INIT	  8	/* INIT */
/* Parallel port incoming: */
#define LL_CTL_BUSY     256	/* BUSY */
#define LL_CTL_ACK      512	/* ACK */
#define LL_CTL_NOPAPER 1024	/* NO PAPER */
#define LL_CTL_ISELECT 2048	/* SELECT */
#define LL_CTL_ERROR   4096	/* ERROR */
/* Serial port outgoing */
#define LL_CTL_DTR    65536  
#define LL_CTL_RTS   131072 
/* Serial port incoming */
#define LL_CTL_CTS 16777216
#define LL_CTL_DSR 33554432

/* Read control lines */
ll_error_t ll_rctl_poll(LL_PDEV self, unsigned *ctl);
/* Wait for control lines to change */
ll_error_t ll_rctl_wait(LL_PDEV self, unsigned *ctl);
/* Set control lines */
ll_error_t ll_sctl(LL_PDEV self, unsigned ctl);

typedef enum { LL_CABLE_NORMAL, LL_CABLE_LAPLINK } LL_CABLE;
/* Set LapLink mode. This emulates the effect of a LapLink cable 
 * between two devices, by wiring the first 5 data lines to the control
 * lines. */
ll_error_t ll_set_cable(LL_PDEV self, LL_CABLE c);
/* Get LapLink setting */
ll_error_t ll_get_cable(LL_PDEV self, LL_CABLE *c);

/* Read LocoLink control lines*/
ll_error_t ll_lrecv_poll(LL_PDEV self, int master, unsigned char *v);
/* Wait for control lines to change */
ll_error_t ll_lrecv_wait(LL_PDEV self, int master, unsigned char *v);
/* Set LocoLink control lines */
ll_error_t ll_lsend(LL_PDEV self, int master, unsigned char v);

/* Serial functions: Setting baudrates & protocol */
typedef enum { PE_NONE, PE_EVEN, PE_ODD } LL_PARITY;

ll_error_t ll_txbaud(LL_PSER self, unsigned baud);
ll_error_t ll_rxbaud(LL_PSER self, unsigned baud);
ll_error_t ll_txbits(LL_PSER self, unsigned bits);
ll_error_t ll_rxbits(LL_PSER self, unsigned bits);
ll_error_t ll_stopbits(LL_PSER self, unsigned bits);
ll_error_t ll_parity(LL_PSER self, LL_PARITY p);

const char *ll_strerror(ll_error_t error);

/* Send a packet down the link (using the standard LocoLink protocol) 
ll_error_t ll_sendpkt(LL_PDATA self, const unsigned char *packet, unsigned int len);
*/
/* Receive a packet (using the standard LocoLink protocol) 
ll_error_t ll_recvpkt(LL_PDATA self, unsigned char *packet, unsigned int *len);
*/
/* RPC functions (from the point of view of a master) */
/* Start a session as a master 
ll_error_t llm_start(LL_PDATA self, char *master_desc, char *slave_desc);
*/
/* Terminate a session 
ll_error_t llm_finish(LL_PDATA self);
*/
/* Errors */
#define LL_E_OK	      (0)		/* OK */
#define LL_E_UNKNOWN  (-1)		/* Unknown error */
#define LL_E_NOMEM    (-2)		/* No memory */
#define LL_E_BADPTR   (-3)		/* Bad pointer passed in */
#define LL_E_SYSERR   (-4)		/* See strerror() */
#define LL_E_EXIST    (-5)		/* File exists */
#define LL_E_NOMAGIC  (-6)		/* Not a LocoLink shared mem file */ 
#define LL_E_CLOSED   (-7)		/* Link is closed */ 
#define LL_E_OPEN     (-8)		/* Trying to open a link that's 
					 * already open */
#define LL_E_INUSE    (-9)		/* Master cannot link to server */ 
#define LL_E_TIMEOUT  (-10)		/* Operation timed out */ 
#define LL_E_BADPKT   (-11)		/* Packet length must be 1-255 bytes */
#define LL_E_CRC      (-12)		/* Received packet has bad CRC */
#define LL_E_UNEXPECT (-13)		/* Other machine sent an unexpected */
					/* value.  */
#define LL_E_NODRIVER (-14)		/* Unknown driver name */
#define LL_E_NOTIMPL  (-15)		/* Function not implemented */
#define LL_E_NOTDEV   (-16)		/* Trying to open a file as a device */
#define LL_E_BADNAME  (-17)		/* Bad filename */
#define LL_E_PASSWORD (-18)		/* Password required & not supplied 
					 * (not yet implemented) */
#define LL_E_AGAIN    (-19)		/* Non-blocking I/O not completed */
#define LL_E_SOCKERR  (-20)		/* See hstrerror() */
#ifdef __cplusplus
}
#endif                                                                          

#endif /* ndef LIBLINK_H_INCLUDED */



