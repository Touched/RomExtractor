#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "crc32.h"

/*
 * Max file sizes that this patcher can support
 */
#define MAX_ROM_SIZE 0x2000000
#define  MAX_PATCH_SIZE (MAX_ROM_SIZE + 100)

static const char ups_magic[] = { 'U', 'P', 'S', '1' };

uint64_t upsVlqRead(uint8_t **buffer) {
	uint8_t byte = 0;
	unsigned int i = 0;
	uint64_t value = 0;

	/* Read byte, preserving position
	 * While MSB is LOW, read 7 LSBs
	 * */
	do {
		byte = *((*buffer)++);
		value |= ((byte & 0x7F) + (i ? 1 : 0)) << (7 * i);
		i++;
	} while ((byte & 0x80) == 0);
	return value;
}

int64_t _upsApply(FILE *base, FILE *patch, int skipCrc, uint8_t **output) {
	uint8_t *patchBuf = NULL, *romBuf = NULL, *patchBufStart = NULL;
	size_t patchSize = 0, romSize = 0, outputSize = 0;
	uint64_t fileSizeOld, fileSizeNew, offset = 0;
	size_t i;
	uint32_t crcPatch, crcOriginal, crcModified;

	/* Alloc more than enough space for the patch */
	patchBufStart = (uint8_t *) malloc(MAX_PATCH_SIZE);
	if (patchBufStart == NULL) {
		return -1;
	}

	/*
	 * Read into the over-large buffer. If the buffer is too large, we can just
	 * ignore it. This is a GBA specific implementation.
	 * Track the start position of the buffer too.
	 */
	patchSize = fread(patchBufStart, sizeof(uint8_t), MAX_PATCH_SIZE, patch);
	patchBuf = patchBufStart;

	/*
	 * Read and validate the patch header before checking the size of the patch
	 */
	if (memcmp(patchBuf, ups_magic, 4)) {
		/* Invalid identifier */
		free(patchBufStart);
		return -1;
	}
	patchBuf += 4;

	/* Read the file sizes from the header */
	fileSizeOld = upsVlqRead(&patchBuf);
	fileSizeNew = upsVlqRead(&patchBuf);

	/*
	 * The files that this patch is for are too large, and can't possibly
	 * be valid GBA ROMs.
	 */
	if (fileSizeOld > MAX_ROM_SIZE || fileSizeNew > MAX_ROM_SIZE) {
		free(patchBufStart);
		return -1;
	}

	if (!feof(patch)) {
		/*
		 * We didn't read the entire patch - it must be fucking huge. Thus we
		 * can assume its not a GBA patch.
		 */
		free(patchBufStart);
		return -1;
	}

	/*
	 * Allocate and zero out. Zeroes needed for XORing to create modified files
	 * larger than the input base: (a ^ 0 = a)
	 */
	romBuf = (uint8_t *) calloc(sizeof(uint8_t), MAX_ROM_SIZE);
	if (romBuf == NULL) {
		free(patchBuf);
		return -1;
	}

	/*
	 * Read the ROM into the buffer and note its size
	 */
	romSize = fread(romBuf, sizeof(uint8_t), MAX_ROM_SIZE, base);
	if (!skipCrc)
		crcOriginal = crc32(0, romBuf, romSize);

	if (fileSizeOld == fileSizeNew) {
		outputSize = fileSizeOld;
	} else {
		outputSize = (fileSizeNew > fileSizeOld) ? fileSizeNew : fileSizeOld;
	}

	/*
	 * Go through all the blocks in the UPS patch
	 * XOR the necessary changes
	 */
	while (patchBuf - patchBufStart + 1 < patchSize - 12) {
		offset += upsVlqRead(&patchBuf);

		for (i = 0; *patchBuf; i++) {
			uint8_t byte = *patchBuf++;
			romBuf[offset + i] ^= byte;
		}
		offset += i + 1;
		patchBuf++;
	}
	if (!skipCrc)
		crcModified = crc32(0, romBuf, outputSize);

	/*
	 * Calculate the CRC using the all data but the last four bytes of the patch
	 * as this is where this CRC is stored.
	 */
	if (!skipCrc)
		crcPatch = crc32(0, patchBufStart, patchSize - 4);

	if (!skipCrc) {
		if (*((uint32_t *) (patchBuf)) != crcOriginal)
			return -1;
		if (*((uint32_t *) (patchBuf + 4)) != crcModified)
			return -1;
		if (*((uint32_t *) (patchBuf + 8)) != crcPatch)
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
	return outputSize;
}
