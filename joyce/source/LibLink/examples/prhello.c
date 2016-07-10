
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "liblink.h"
#include "config.h"
#ifdef HAVE_WINDOWS_H
#include "windows.h"
#endif

int main(int argc, char **argv)
{
	LL_PDEV link;
	char *port = "unix:C:/tmp/echodev";
	char *drvr = "parsocket";
//	char *port = "/dev/parport0";
//	char *drvr = "parport";
	char *s = "Hello, World\r\n";
	ll_error_t err;
	
	if (argc > 1) port = argv[1];
	if (argc > 2) drvr = argv[2]; 
	printf("Opening port %s with driver %s: %d\n",
		port, drvr, err = ll_open(&link, port, drvr));

	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	while (*s)
	{
		printf("Sending %c: %d\n", isprint(*s) ? *s : ' ', err = ll_send(link, *s));
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }
		++s;
		printf("Strobing: %d\n", err = ll_sctl(link, LL_CTL_STROBE));
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }
#ifdef HAVE_WINDOWS_H
		Sleep(1);
#else
		usleep(150);
#endif
		printf("Strobing: %d\n", err = ll_sctl(link, 0));
		if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }
	}
	printf("Closing: %d\n", err = ll_close(&link));
	if (err) { fprintf(stderr, "%s\n", ll_strerror(err)); return 1; }

	return 0;
}
