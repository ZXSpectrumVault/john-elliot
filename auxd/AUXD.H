
#define BUFSIZE 10240	/* Max buffer size. Two 16k buffers is about
			   as much as we can hope to fit in the average
			   TPA; 2x10k should be ample. */

#include "cpmdsk.h"	/* LibDsk typedefs & prototypes */

dsk_err_t read_packet(unsigned char  *pkt, int *len);
dsk_err_t write_packet(unsigned char *pkt, int len);
dsk_err_t dsk_rpc_server(unsigned char *input, int inp_len, 
		         unsigned char *output, int *out_len,
			 int *ref_count);

#define const /* HTC has no const */

/* Options */
extern unsigned char gl_verbose;
