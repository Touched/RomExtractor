#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LZSS_MAX_MATCH_LENGTH 18
#define LZSS_MIN_MATCH_LENGTH 3
#define LZSS_DICTIONARY_SIZE 4096

typedef struct {
	int flag;
} Block;

typedef struct {
	Block base;
	int distance;
	int length;
} BlockCompressed;

typedef struct {
	Block base;
	uint8_t byte;
} BlockUncompressed;

int compress(uint8_t *data, int length, uint8_t *output) {
	int wrote = 4;
	int current = 0, i, j, window_size = 0;
	uint8_t *window = data;

	int match_length = 0;
	int biggest_match_length = 0;
	int biggest_match_offset = 0;

	int block_count = 0;
	Block *blocks[8] = { };

	/* TODO: Length check, output alignment */

	/* Write header to output buffer */
	output[0] = 0x10;
	output[1] = (length) & 0xFF;
	output[2] = (length >> 8) & 0xFF;
	output[3] = (length >> 16) & 0xFF;

	/* Initialize the sliding window to point to the first byte of the input */

	while (current < length) {
		/* Find the longest match in the sliding window */
		biggest_match_length = 0;
		for (i = 0; i < window_size; ++i) {
			/* Find longest valid match */
			match_length = 0;
			for (j = 0; j < LZSS_MAX_MATCH_LENGTH; ++j) {
				/* TODO: Array index checks */

				if (data[current + j] != window[i + j])
					break;
				match_length++;
			}

			/* Is this match bigger? */
			if (match_length > biggest_match_length) {
				biggest_match_length = match_length;
				biggest_match_offset = i;
			}

			/* Biggest match found, no point searching more */
			if (biggest_match_length == LZSS_MAX_MATCH_LENGTH - 1) {
				break;
			}
		}

		/* Create the block */
		if (biggest_match_length > LZSS_MIN_MATCH_LENGTH) {
			/* This block should be compressed */
			BlockCompressed *temp = malloc(sizeof(BlockCompressed));
			temp->base.flag = 1;
			temp->distance = biggest_match_offset + 1;
			temp->length = biggest_match_length;
			blocks[block_count++] = (Block *) temp;
			current += biggest_match_length;
		} else {
			BlockUncompressed *temp = malloc(sizeof(BlockUncompressed));
			temp->base.flag = 0;
			temp->byte = data[current];
			blocks[block_count++] = (Block *) temp;
			current++;
		}

		/* Write blocks to the output buffer */
		if (block_count == 8) {
			/* Write flags byte (flags for next eight blocks) */
			uint8_t flags = 0;

			/* MSB first */
			for (i = 7; i >= 0; --i) {
				flags |= (blocks[i]->flag) << i;
			}
			output[wrote++] = flags;

			for (i = 0; i < 8; ++i) {
				if (blocks[i]->flag) {
					uint16_t out = 0;
					/* Compressed block */
					BlockCompressed *temp = (BlockCompressed *) blocks[i];
					free(temp);

					/* Displacement LSBs */
					out |= (temp->distance & 0xFF) << 8;

					/* Disp MSBs */
					out |= (temp->distance & 0xF00) >> 8;

					/* Length */
					out |= (temp->length & 0xF) << 4;

					/* Takes two bytes to write a length-distance pair */
					output[wrote++] = out & 0xFF;
					output[wrote++] = out >> 8;
				} else {
					BlockUncompressed *temp = (BlockUncompressed *) blocks[i];
					free(temp);

					/* Takes one byte to write an uncompressed byte */
					output[wrote++] = temp->byte;
				}
			}

			block_count = 0;
		}

		/* Determine how big our window is */
		window_size++;

		/* Slide the window if necessary */
		while (window_size > LZSS_DICTIONARY_SIZE) {
			window++;
			window_size--;
		}
	}

	return wrote;
}

int decompress(const uint8_t *src, uint8_t **destination, uint32_t *outSize,
		uint32_t srcSize) {
	uint8_t flags = 0;

	int current = 0, read = 0, block = 0, i = 0;
	int displacement = 0, count = 0, start = 0;
	int from = 0, to = 0;

	if (*src != 0x10) {
		printf("Error: Invalid header\n");
	}

	uint32_t destSize = (*((uint32_t *) src)) >> 8;

	if (!destSize) {
		printf("Error: Invalid size\n");
	}

	uint8_t *dest = (uint8_t *) malloc(destSize);
	if (outSize != NULL)
		*outSize = destSize;

	*destination = dest;

	/*
	 * Header size
	 */
	src += 4;

	while (current < destSize) {
		if (read >= srcSize) {
			// TODO: Error: Out of data
			printf("Error: Out of data\n");
			return -1;
		}

		flags = (int) src[read++];
		//read++;

		for (block = 0; block < 8; ++block) {

			if (current > destSize) {
				// TODO: Error: Out of memory
				printf("Error: Out of memory\n");
				return -1;
			}

			/* Out of memory, invalid */
			if (read >= srcSize) {
				// TODO: Error
				printf("Error: Out of data\n");
				return -1;
			}

			if ((flags & (0x80 >> block)) > 0) {
				/* Compressed case */
				displacement = 0;

				/* Read block size */
				count = (((int) src[read]) >> 4) + 3;

				/* Read displacement */
				displacement = ((((int) src[read]) & 0xF) << 8);
				read++;
				displacement |= (int) src[read];
				read++;

				/* Byte copy */
				start = current;

				for (i = 0; i < count; ++i) {
					from = start - displacement - 1 + i;
					to = current++;

					if (to >= destSize || from >= destSize)
						return -2;

					uint8_t b = dest[from];
					dest[to] = b;
				}
			} else {
				/* Uncompressed case */
				dest[current++] = src[read++];
			}

			if (current >= destSize)
				break;

		}
	}

	return read;
}
