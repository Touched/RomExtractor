#include "crc32.h"

uint32_t crc32(uint32_t crc, uint8_t *buf, size_t len) {
	const uint8_t *p;

	p = buf;
	crc = ~crc;

	while (len--)
		crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return ~crc;
}
