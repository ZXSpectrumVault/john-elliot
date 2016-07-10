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

#ifndef HAVE_WINSOCK_H
#define SOCKET int	/* Type of a socket */
#define INVALID_SOCKET (-1)
#endif

/* Simulating a parallel port using a file */
typedef struct ll_parsocket
{
	LL_DEVICE	 llps_super;	
	unsigned char    llps_ibyte;
	unsigned	 llps_istat;
	SOCKET		 llps_sockfd;	/* Listening socket */
	SOCKET		 llps_sockfd2;	/* Open socket */
#ifndef HAVE_WINSOCK_H	/* Winsock doesn't have unix domain sockets */
        struct sockaddr_un llps_s_unix;
#endif
        struct sockaddr_in llps_s_inet;
	struct sockaddr	   llps_client;
	int	         llps_listening;
	char		 llps_clserver;
} LL_PARSOCKET;


/* Open connection */
ll_error_t llps_open(LL_PDEV self);
/* Close connection */
ll_error_t llps_close(LL_PDEV self);
/* Flush data */
ll_error_t llps_flush(LL_PDEV self);
/* Sending data */
ll_error_t llps_send(LL_PDEV self, unsigned char b);
/* Setting control lines */
ll_error_t llps_sctl(LL_PDEV self, unsigned b);
/* Reading control lines */
ll_error_t llps_rctl_poll(LL_PDEV self, unsigned *b);
/* Reading control lines */
ll_error_t llps_rctl_wait(LL_PDEV self, unsigned *b);
/* Reading data lines */
ll_error_t llps_recv_poll(LL_PDEV self, unsigned char *b);
/* Reading data lines */
ll_error_t llps_recv_wait(LL_PDEV self, unsigned char *b);
/* Setting cable type */ 
ll_error_t llps_set_cable(LL_PDEV self, LL_CABLE cable);


