
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"

static char *filename;

int main(int argc, char **argv)
{
	CPI_FILE file;
	int err;

	if (argc < 2)
	{
		fprintf(stderr, "Syntax: %s cpifile\n", argv[0]);
		return EXIT_FAILURE;
	}
	filename = argv[1];
	
	err = cpi_load(&file, filename);
	if (err)
	{
		fprintf(stderr, "Error %d loading %s\n", err, filename);
		return EXIT_FAILURE; 
	}
	if (argc == 2) cpi_dump(&file);
	else
	{
		err = cpi_save(&file, argv[2], 1);
		if (err) fprintf(stderr, "Error %d saving %s\n", err, argv[2]);
	}
	
	cpi_delete(&file);
	return 0;
}
