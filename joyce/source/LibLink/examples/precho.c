
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "liblink.h"

#ifdef _WIN32
#define sleep(x) Sleep((x)*1000)
#endif

int main(int argc, char **argv)
{
	LL_PDEV link;
	unsigned char c;
	char *port = "unix:S:/tmp/echodev";
	char *drvr = "parsocket";
	ll_error_t err;
	unsigned ctl;
	
	if (argc > 1) port = argv[1];
	if (argc > 2) drvr = argv[2]; 
	printf("Opening port %s with driver %s: %d\n",
		port, drvr, err = ll_open(&link, port, drvr));

	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	while (1)
	{
//		printf("%02x: Waiting for strobe low\n", ((char *)link)[0]);
		err = ll_rctl_wait(link, &ctl);
		if (err == LL_E_CLOSED) break;
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

		if (!(ctl & LL_CTL_STROBE)) continue;

//		printf("%02x:Reading data\n", ((char *)link)[0]);
		err = ll_recv_poll(link, &c);
		if (err == LL_E_CLOSED) break;
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }
		printf("Got char: %c %02x\n", isprint(c) ? c : '?', c);

		while (1)
		{
//			printf("Waiting for strobe high\n");
			err = ll_rctl_wait(link, &ctl);
			if (err == LL_E_CLOSED) break;
			if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

			if (!(ctl & LL_CTL_STROBE)) break;
		}
	}
	printf("Closing: %d\n", err = ll_close(&link));
	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	return 0;
}
