#ifndef _PERF_IMAGE_H
#define _PERF_IMAGE_H

#include <stdint.h>

int compress(uint8_t *data, int length, uint8_t *output);
int decompress(const uint8_t *src, uint8_t **destination, uint32_t *outSize,
		uint32_t srcSize);

int deindex_4bpp(uint8_t *data, int length, uint32_t *palette, uint32_t **out);
int detile(uint32_t *tiles, int length, int width, uint32_t **out);
int save_png_to_file(uint32_t *pixels, uint32_t width, uint32_t height,
		const char *path);

#endif /* _PERF_IMAGE_H */
