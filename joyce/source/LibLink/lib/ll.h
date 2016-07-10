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
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define sleep(x) Sleep((x)*1000)
#endif

#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif

#include "liblink.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ll_vtable
{
	int  llv_datasize;	/* size of instance data */
	char *llv_drvname;
	char *llv_drvdesc;

	ll_error_t   (*llv_open)(LL_PDEV self);
	ll_error_t   (*llv_close)(LL_PDEV self);
	ll_error_t   (*llv_flush)(LL_PDEV self);
/* Sending/receiving data */
	ll_error_t   (*llv_send)(LL_PDEV self, unsigned char b);
	ll_error_t   (*llv_send_byte)(LL_PDEV self, unsigned char b);
	ll_error_t   (*llv_recv_poll)(LL_PDEV self, unsigned char *b);
	ll_error_t   (*llv_recv_wait)(LL_PDEV self, unsigned char *b);
/* Setting/reading control lines */	
        ll_error_t   (*llv_sctl)(LL_PDEV self, unsigned b);
        ll_error_t   (*llv_rctl_poll)(LL_PDEV self, unsigned *b);
        ll_error_t   (*llv_rctl_wait)(LL_PDEV self, unsigned *b);
/* Setting cable type */
	ll_error_t   (*llv_set_cable)(LL_PDEV self, LL_CABLE cable);
	ll_error_t   (*llv_get_cable)(LL_PDEV self, LL_CABLE *cable);

} LL_VTBL;

typedef struct ll_ser_vtable
{
	LL_VTBL llsv_super;

	ll_error_t (*llsv_txbaud)(LL_PSER self, unsigned baud);
	ll_error_t (*llsv_rxbaud)(LL_PSER self, unsigned baud);
	ll_error_t (*llsv_txbits)(LL_PSER self, unsigned bits);
	ll_error_t (*llsv_rxbits)(LL_PSER self, unsigned bits);
	ll_error_t (*llsv_stopbits)(LL_PSER self, unsigned bits);
	ll_error_t (*llsv_parity)(LL_PSER self, LL_PARITY p);
} LL_SVTBL;



/* The base class of all LibLink devices */
typedef struct ll_device
{
/* The name "clazz" cheerfully stolen from JDK 1.1. I always drezz for the
 * occasion... */
	LL_VTBL		*lld_clazz;
	char		*lld_filename; 	/* Filename (for reporting) */	
	unsigned char	 lld_lastread;	/* For emulating recv() on */
	unsigned	 lld_lastrctl;	/* data & control lines */
	LL_CABLE	 lld_cable;	/* In Laplink mode? */
} LL_DEVICE;




/* Open the connector file, fail if it's not present, or not suitable */
ll_error_t ll_copen(LL_PDEV self);
/* Create the connector file. */
ll_error_t ll_ccreat(LL_PDEV self, int mode, const char *drivername, 
		     const char *desc);
/* Close the connector file. */
ll_error_t ll_cclose(LL_PDEV self);

/* Set the "connected" flag */
ll_error_t ll_setconnected(LL_PDEV self, int connected);
/* Get the "connected" flag */
ll_error_t ll_getconnected(LL_PDEV self, int *connected);
/* Get the name of the driver to use */
ll_error_t ll_getdrivername(LL_PDEV self, char *drvname);


/* Equivalents of ll_send, ll_recv_* that don't check for funky laplink 
 * transformations */

ll_error_t ll_irecv_poll(LL_PDEV self, unsigned char *value);
ll_error_t ll_irecv_wait(LL_PDEV self, unsigned char *value);
ll_error_t ll_isend(LL_PDEV self, unsigned char value);
ll_error_t ll_irctl_poll(LL_PDEV self, unsigned *ctl);
ll_error_t ll_irctl_wait(LL_PDEV self, unsigned *ctl);
ll_error_t ll_isctl(LL_PDEV self, unsigned ctl);


void microsleep(int msecs);

#define LL_USERMEM 256	// Start of modifiable area in linkfile
#define LL_USEREND 4096 // End of modifiable area in linkfile

#ifdef __cplusplus
}
#endif                                                                          


