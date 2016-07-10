#include <stdio.h>
#include <stdlib.h>
#include "liblink.h"

int main(int argc, char **argv)
{
	LL_PDEV link;
	unsigned char v;
	ll_error_t e;
	
	printf("Creating: %d\n",
		e = ll_open(&link, "unix:S:/tmp/locolink.socket", "parsocket"));

	if (e) printf("%s\n", ll_strerror(e));
	printf("Receive poll: %d\n", e = ll_lrecv_poll(link, 0, &v));
	if (e) printf("%s\n", ll_strerror(e));
	printf("Writing 3: %d\n", e = ll_lsend(link, 0, 3));
	if (e) printf("%s\n", ll_strerror(e));
	getchar();
	printf("Writing 2: %d\n", e = ll_lsend(link, 0, 2));
	if (e) printf("%s\n", ll_strerror(e));
	getchar();
	printf("Closing: %d\n", e = ll_close(&link));
	if (e) printf("%s\n", ll_strerror(e));

	return 0;
}
