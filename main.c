#include <stdio.h>

#include "ipcountry.h"

int
main(int argc, char *argv[])
{
	char cc[3];
	struct ipcountry ipcountry;

	if (argc < 3) {
		fprintf(stderr, "%s ipcountry.db ipaddress\n", argv[0]);
		return 0;
	}

	if (ipcountry_open(&ipcountry, argv[1]) < 0) {
		fprintf(stderr, "can't open %s\n", argv[1]);
		return 0;
	}

	cc[2] = '\0';
	if (ipcountry_lookup(&ipcountry, argv[2], cc) < 0) {
		printf("NOT FOUND\n");
	} else {
		printf("%s\n", cc);
	}

	ipcountry_close(&ipcountry);

	return 0;
}
