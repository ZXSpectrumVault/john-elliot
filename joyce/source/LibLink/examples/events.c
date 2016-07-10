
#include <stdio.h>
#include <ctype.h>
#include "utils.h"


int main(int argc, char **argv)
{
	LL_PDEV link;
	unsigned char c = 0;
	char *port = "/dev/parport0";
	char *drvr = "parport";
	ll_error_t err;
	unsigned ctl;
	unsigned char obyte = 0;
	
	if (argc > 1) port = argv[1];
	if (argc > 2) drvr = argv[2]; 
	printf("Opening port %s with driver %s: %d\n",
		port, drvr, err = ll_open(&link, port, drvr));

	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	ll_send(link, obyte);
	while (1)
	{
		switch(pollch())
		{
			case '0': obyte ^= 1; ll_send(link, obyte); break;
			case '1': obyte ^= 2; ll_send(link, obyte); break;
			case '2': obyte ^= 4; ll_send(link, obyte); break;
			case '3': obyte ^= 8; ll_send(link, obyte); break;
			case '4': obyte ^= 16; ll_send(link, obyte); break;
			case '5': obyte ^= 32; ll_send(link, obyte); break;
			case '6': obyte ^= 64; ll_send(link, obyte); break;
			case '7': obyte ^= 128; ll_send(link, obyte); break;
			case 'q':
			case 'Q': goto finish;
		}
		err = ll_rctl_poll(link, &ctl);
		if (err == LL_E_CLOSED) break;
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

		printf("err=%d, New ctl = %08x %02x\n", err, ctl, c);
	}
finish:
	printf("Closing: %d\n", err = ll_close(&link));
	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	return 0;
}
