#include "ipcountry.h"

#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>

/* uint128_t start */

typedef struct {
	uint64_t high;
	uint64_t low;
} uint128_t;

static inline void
uint128_from_uint8(uint128_t *a, uint8_t v)
{
	a->high = 0;
	a->low  = v;
}

static inline void
uint128_from_be128(uint128_t *a, const uint8_t *buf)
{
	a->high = 0;
	a->low  = 0;

	a->high |= ((uint64_t)buf[0] << 56) & 0xFF00000000000000;
	a->high |= ((uint64_t)buf[1] << 48) & 0x00FF000000000000;
	a->high |= ((uint64_t)buf[2] << 40) & 0x0000FF0000000000;
	a->high |= ((uint64_t)buf[3] << 32) & 0x000000FF00000000;
	a->high |= ((uint64_t)buf[4] << 24) & 0x00000000FF000000;
	a->high |= ((uint64_t)buf[5] << 16) & 0x0000000000FF0000;
	a->high |= ((uint64_t)buf[6] << 8)  & 0x000000000000FF00;
	a->high |= ((uint64_t)buf[7])       & 0x00000000000000FF;

	a->low |= ((uint64_t)buf[8]  << 56) & 0xFF00000000000000;
	a->low |= ((uint64_t)buf[9]  << 48) & 0x00FF000000000000;
	a->low |= ((uint64_t)buf[10] << 40) & 0x0000FF0000000000;
	a->low |= ((uint64_t)buf[11] << 32) & 0x000000FF00000000;
	a->low |= ((uint64_t)buf[12] << 24) & 0x00000000FF000000;
	a->low |= ((uint64_t)buf[13] << 16) & 0x0000000000FF0000;
	a->low |= ((uint64_t)buf[14] << 8)  & 0x000000000000FF00;
	a->low |= ((uint64_t)buf[15])       & 0x00000000000000FF;
}

static inline void
uint128_add128(uint128_t *acc, const uint128_t *add)
{
	acc->high += add->high;
	acc->low  += add->low;

	if (acc->low < add->low) {
		acc->high++;
	}
}

static inline void
uint128_shl(uint128_t *dest, uint8_t shift)
{
	if (shift >= 64) {
		dest->high = dest->low << (shift - 64);
		dest->low  = 0;
	} else {
		dest->high <<= shift;
		dest->high |= (dest->low >> shift) & (((uint64_t)1 << shift) - 1);
		dest->low  <<= shift;
	}
}

static inline int
uint128_cmp128(const uint128_t *a, const uint128_t *b)
{
	if (a->high < b->high) {
		return -1;
	}

	if (a->high > b->high) {
		return 1;
	}

	if (a->low < b->low) {
		return -1;
	}

	if (a->low > b->low) {
		return 1;
	}

	return 0;
}

/* uint128_t end */

/* binary search start */

static inline int64_t
binarysearch(const void *key, const void *base, const size_t len, const size_t width,
             int (*compare)(const void *, const void *))
{
	int cmp;
	int64_t low;
	int64_t mid;
	int64_t high;

	low = 0;
	high = len;
	while ((low + 1) < high) {
		mid = low + ((high - low) / 2);
		cmp = compare((char *)base + mid * width, key);
		if (cmp == 0) {
			return mid;
		}
		if (cmp < 0) {
			low = mid;
		} else {
			high = mid;
		}
	}

	return low;
}

/* binary search end */

struct ipv4record {
	uint32_t start;
	uint32_t value;
	uint32_t flags;
	char cc[4];
};

struct ipv6record {
	uint8_t start[16];
	uint32_t value;
	uint32_t flags;
	uint32_t unuse;
	char cc[4];
};

static int
ipv4compare(const void *a, const void *b)
{
	return memcmp(a, b, 4);
}

static int
ipv6compare(const void *a, const void *b)
{
	return memcmp(a, b, 16);
}

int
ipcountry_open(struct ipcountry *ipcountry, const char *filename)
{
	int fd;
	void *map;
	off_t len;
	char *base;

	uint32_t ipv4len;
	uint32_t ipv6len;

	memset(ipcountry, 0, sizeof(struct ipcountry));
	fd = open(filename, O_RDONLY, 0644);
	if (fd < 0) {
		goto err;
	}

	if (read(fd, &ipv4len, sizeof(ipv4len)) != sizeof(ipv4len)) {
		goto err;
	}
	ipv4len = ntohl(ipv4len);

	if (read(fd, &ipv6len, sizeof(ipv6len)) != sizeof(ipv6len)) {
		goto err;
	}
	ipv6len = ntohl(ipv6len);

	len = lseek(fd, 0LL, SEEK_END);
	if (len < sizeof(ipv4len) +
	          sizeof(ipv6len) +
	          ipv4len * sizeof(struct ipv4record) +
	          ipv6len * sizeof(struct ipv6record))
	{
		goto err;
	}
	map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		goto err;
	}

	ipcountry->fd = fd;
	ipcountry->map = map;
	ipcountry->len = len;
	ipcountry->ipv4len = ipv4len;
	ipcountry->ipv6len = ipv6len;

	base = (char *)map + sizeof(ipv4len) + sizeof(ipv6len);
	ipcountry->ipv4records = (struct ipv4record *)base;
	ipcountry->ipv6records = (struct ipv6record *)(base + ipv4len * sizeof(struct ipv4record));

	return 0;
err:

	ipcountry_close(ipcountry);

	return -1;
}

int
ipcountry_lookup(struct ipcountry *ipcountry, const char *addr, char cc[2])
{
	struct addrinfo hints, *res0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST,
	hints.ai_socktype = SOCK_STREAM;

	res0 = NULL;
	if (getaddrinfo(addr, NULL, &hints, &res0)) {
		goto err;
	}

	if (res0->ai_family == AF_INET) {
		uint32_t i;
		struct ipv4record  key;
		struct ipv4record *record;

		if (ipcountry->ipv4len == 0) {
			goto err;
		}

		memset(&key, 0, sizeof(key));
		memcpy(&key.start,
		       &((struct sockaddr_in *)res0->ai_addr)->sin_addr,
		       sizeof(struct in_addr));

		i = binarysearch(&key,
		                 ipcountry->ipv4records,
		                 ipcountry->ipv4len,
		                 sizeof(struct ipv4record),
		                 ipv4compare);

		record = ipcountry->ipv4records + i;
		if (ntohl(record->start) > ntohl(key.start) ||
		    (ntohl(record->start) + ntohl(record->value)) < ntohl(key.start))
		{
			goto err;
		}

		memcpy(cc, record->cc, 2);
	} else if (res0->ai_family == AF_INET6) {
		uint32_t i;
		struct ipv6record  key;
		struct ipv6record *record;

		uint128_t start;
		uint128_t value;
		uint128_t key128;

		if (ipcountry->ipv6len == 0) {
			goto err;
		}

		memset(&key, 0, sizeof(key));
		memcpy(key.start,
		       &((struct sockaddr_in6 *)res0->ai_addr)->sin6_addr,
		       sizeof(key.start));

		i = binarysearch(&key,
		                 ipcountry->ipv6records,
		                 ipcountry->ipv6len,
		                 sizeof(struct ipv6record),
		                 ipv6compare);

		record = ipcountry->ipv6records + i;

		uint128_from_be128(&start, record->start);
		uint128_from_be128(&key128, key.start);
		if (uint128_cmp128(&start, &key128) == 1) {
			goto err;
		}

		uint128_from_uint8(&value, 1);
		uint128_shl(&value, 128 - ntohl(record->value));
		uint128_add128(&start, &value);
		if (uint128_cmp128(&start, &key128) == -1) {
			goto err;
		}

		memcpy(cc, record->cc, 2);
	}

	freeaddrinfo(res0);

	return 0;
err:

	freeaddrinfo(res0);

	return -1;
}

void
ipcountry_close(struct ipcountry *ipcountry)
{
	if (ipcountry->map != NULL || ipcountry->map != MAP_FAILED) {
		munmap(ipcountry->map, ipcountry->len);
	}

	ipcountry->len = 0;
	ipcountry->map = NULL;
	ipcountry->ipv4len = 0;
	ipcountry->ipv6len = 0;
	ipcountry->ipv4records = NULL;
	ipcountry->ipv6records = NULL;

	close(ipcountry->fd);
	ipcountry->fd = 0;
}
