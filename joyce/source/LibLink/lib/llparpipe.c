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

#include "ll.h"
#include "llparpipe.h"
#include "llparfile.h"

LL_VTBL llv_parpipe =
{
	sizeof(LL_PARFILE),
	"parpipe",
	"Parallel port output to pipe",
	llpp_open,
	llpp_close,
	llpf_flush,
	llpf_send,
	llpf_send_byte,
	NULL,	/* recv_poll */
	NULL,	/* recv_wait */
	llpf_sctl,
	llpf_rctl_poll,
	NULL,	/* No handy "sleep on control signal change" */
};




/* Open connection */
ll_error_t llpp_open(LL_PDEV self)
{
	LL_PARFILE *trueself;

	if (!self || self->lld_clazz != &llv_parpipe || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARFILE *)self;
	trueself->llpf_latch = EOF;
	trueself->llpf_fp = popen(self->lld_filename, "w");
	if (!trueself->llpf_fp) return LL_E_SYSERR;
	return LL_E_OK;
}


/* Close connection */
ll_error_t llpp_close(LL_PDEV self)
{
        LL_PARFILE *trueself;
	int err;

        if (!self || self->lld_clazz != &llv_parpipe) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
	trueself->llpf_latch = EOF;
        if (!trueself->llpf_fp) return LL_E_CLOSED;
	err = pclose(trueself->llpf_fp);
	trueself->llpf_fp = NULL;
	if (err == -1) return LL_E_SYSERR;	// Ignore program return code
	return LL_E_OK;
}




