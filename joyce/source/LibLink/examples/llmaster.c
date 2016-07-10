
#include <stdio.h>
#include "liblink.h"

int main(int argc, char **argv)
{
	LL_PDEV link;
	unsigned char c;
	ll_error_t e;

	printf("Opening: %d\n",
		e = ll_open(&link, "unix:C:/tmp/locolink.socket", "parsocket"));
	if (e) printf("%s\n", ll_strerror(e));

	printf("Reading: %d\n", e = ll_lrecv_poll(link, 1, &c));
	printf("  = %d\n", (int)c);
	if (e) printf("%s\n", ll_strerror(e));
	getchar();
	printf("Reading: %d\n", e = ll_lrecv_poll(link, 1, &c));
	printf("  = %d\n", (int)c);
	if (e) printf("%s\n", ll_strerror(e));
	printf("Closing: %d\n", e = ll_close(&link));
	if (e) printf("%s\n", ll_strerror(e));

	return 0;
}
