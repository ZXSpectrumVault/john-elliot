/* List of all LibLink drivers. */

	&llv_parfile,	/* Parallel port to a file */
	&llv_parpipe,	/* Parallel port to a pipe */
#ifdef HAVE_LINUX_PPDEV_H
	&llv_parport,	/* Parallel port is an actual port */
#endif
	&llv_parsocket,	/* Parallel port to/from a socket */

