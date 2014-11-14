#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_ROM_SIZE 0x1000000
#define  MAX_PATCH_SIZE (MAX_ROM_SIZE + 100)

static const char ips_magic[] = { 'P', 'A', 'T', 'C', 'H' };
static const char ips_magic_end[] = { 'E', 'O', 'F' };

int ipsApply(FILE *base, FILE *patch, uint8_t **output) {
	uint8_t *patchBufStart = NULL, *patchBuf, *romBuf;
	size_t patchSize = 0, i = 0, romSize = 0;
	uint32_t offset;
	uint16_t size;

	patchBufStart = (uint8_t *) malloc(MAX_PATCH_SIZE);

	if (patchBufStart == NULL) {
		return -1;
	}

	patchSize = fread(patchBufStart, sizeof(uint8_t), MAX_PATCH_SIZE, patch);
	patchBuf = patchBufStart;

	/*
	 * Read and validate the patch header before checking the size of the patch
	 */

	if (memcmp(patchBuf, ips_magic, 5)) {
		/* Invalid identifier */
		free(patchBufStart);
		return -1;
	}
	patchBuf += 5;

	if (!feof(patch)) {
		/*
		 * We didn't read the entire patch - it must be fucking huge. Thus we
		 * can assume its not a GBA patch.
		 */
		free(patchBufStart);
		return -1;
	}

	romBuf = (uint8_t *) calloc(sizeof(uint8_t), MAX_PATCH_SIZE);
	if (romBuf == NULL) {
		free(patchBuf);
		return -1;
	}

	romSize = fread(romBuf, sizeof(uint8_t), MAX_ROM_SIZE, base);

	/*
	 * ROM too big - we couldn't read it all into the buffer
	 */
	if (!feof(romSize)) {
		free(patchBuf);
		free(romBuf);
		return -1;
	}

	while (patchBuf - patchBufStart < patchSize - 3) {
		/*
		 * Always big endian in IPS
		 */
		offset = (*patchBuf++) << 16;
		offset |= (*patchBuf++) << 8;
		offset |= (*patchBuf++);

		size = (*patchBuf++) << 8;
		size |= (*patchBuf++);

		if (size) {
			if (offset + size > romSize) {
				free(patchBufStart);
				free(romBuf);
				return -1;
			}

			for (i = 0; i < size; ++i) {
				romBuf[offset + i] = (*patchBuf++);
			}
		} else {
			/*
			 * RLE Encoded Block
			 */
			uint8_t byte = 0;
			size = *patchBuf++ << 8;
			size |= (*patchBuf++);
			byte = *patchBuf++;

			if (offset + size > romSize) {
				free(patchBufStart);
				free(romBuf);
				return -1;
			}

			for (i = 0; i < size; ++i) {
				romBuf[offset + i] = byte;
			}
		}
	}

	if (memcmp(patchBuf, ips_magic_end, 3)) {
		/* Invalid EOF identifier */
		free(patchBufStart);
		free(romBuf);
		return -1;
	}

	/*
	 * Only free this one, as the other is returned
	 */
	free(patchBufStart);

	/*
	 * Return the buffer and the buffer length
	 */
	*output = romBuf;
	return romSize;
}
