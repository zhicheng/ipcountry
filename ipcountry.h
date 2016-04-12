#ifndef __IPCOUNTRY_H__
#define __IPCOUNTRY_H__

#include <stdint.h>

struct ipcountry {
	int fd;
	void *map;

	int64_t  len;
	uint32_t ipv4len;
	uint32_t ipv6len;

	struct ipv4record *ipv4records;
	struct ipv6record *ipv6records;
};

int
ipcountry_open(struct ipcountry *ipcountry, const char *filename);

/*
 * @prarm addr: ipv4 ipv6
 * @param cc: 2 bytes country code
 * @return 0 success, -1 failure
 */
int
ipcountry_lookup(struct ipcountry *ipcountry, const char *addr, char cc[2]);

void
ipcountry_close(struct ipcountry *ipcountry);

#endif /* __IPCOUNTRY_H__ */
