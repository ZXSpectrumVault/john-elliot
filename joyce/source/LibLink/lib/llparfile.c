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
#include "llparfile.h"

LL_VTBL llv_parfile =
{
	sizeof(LL_PARFILE),
	"parfile",
	"Parallel port output to file",
	llpf_open,
	llpf_close,
	llpf_flush,
	llpf_send,
	llpf_send_byte,
	NULL,	/* recv_poll */
	NULL,	/* recv_wait */
	llpf_sctl,
	llpf_rctl_poll,
	NULL,	/* No handy "sleep on control signal change" */
};

extern LL_VTBL llv_parpipe;


/* Open connection */
ll_error_t llpf_open(LL_PDEV self)
{
	LL_PARFILE *trueself;

	if (!self || self->lld_clazz != &llv_parfile || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARFILE *)self;
	trueself->llpf_latch = EOF;
	trueself->llpf_fp = fopen(self->lld_filename, "a");
	if (!trueself->llpf_fp) return LL_E_SYSERR;
	return LL_E_OK;
}


/* Close connection */
ll_error_t llpf_close(LL_PDEV self)
{
        LL_PARFILE *trueself;
	int err;

        if (!self || self->lld_clazz != &llv_parfile) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
	trueself->llpf_latch = EOF;
        if (!trueself->llpf_fp) return LL_E_CLOSED;
	err = fclose(trueself->llpf_fp);
	trueself->llpf_fp = NULL;
	if (err) return LL_E_SYSERR;
	return LL_E_OK;
}


/* Flush data */
ll_error_t llpf_flush(LL_PDEV self)
{
        LL_PARFILE *trueself;

        if (!self || (self->lld_clazz != &llv_parfile && self->lld_clazz != &llv_parpipe)) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
        if (!trueself->llpf_fp) return LL_E_CLOSED;
        
	return fflush(trueself->llpf_fp) ? LL_E_SYSERR : LL_E_OK;
}


/* Sending data */
ll_error_t llpf_send(LL_PDEV self, unsigned char b)
{
        LL_PARFILE *trueself;

        if (!self || (self->lld_clazz != &llv_parfile && self->lld_clazz != &llv_parpipe)) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
        if (!trueself->llpf_fp) return LL_E_CLOSED;

	trueself->llpf_latch = b;
	return LL_E_OK;
}


/* Setting control lines. Twiddling STROBE sends the byte. */
ll_error_t llpf_sctl(LL_PDEV self, unsigned b)
{
        LL_PARFILE *trueself;
	int e = 0;

        if (!self || (self->lld_clazz != &llv_parfile && self->lld_clazz != &llv_parpipe)) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
        if (!trueself->llpf_fp) return LL_E_CLOSED;
	if (b & LL_CTL_STROBE)
	{
		if (trueself->llpf_latch != EOF)
		{
			e = fputc(trueself->llpf_latch, trueself->llpf_fp);
		}	
		trueself->llpf_latch = EOF;
	}
	if (e == EOF) return LL_E_SYSERR;
	return LL_E_OK;
}


/* Reading control lines */
ll_error_t llpf_rctl_poll(LL_PDEV self, unsigned *b)
{
	if (!b) return LL_E_BADPTR;
	*b = 0;
	return LL_E_OK;
}
 

/* Sending a whole byte */
ll_error_t llpf_send_byte(LL_PDEV self, unsigned char b)
{
        LL_PARFILE *trueself;

        if (!self || (self->lld_clazz != &llv_parfile && self->lld_clazz != &llv_parpipe)) return LL_E_BADPTR;
        trueself = (LL_PARFILE *)self;
        if (!trueself->llpf_fp) return LL_E_CLOSED;
	if (fputc(b, trueself->llpf_fp) == EOF) return LL_E_SYSERR;

	return LL_E_OK;
}

