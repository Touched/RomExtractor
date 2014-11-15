#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "image.h"

typedef struct {
	uint32_t image;
	short idk;
	short idk2;
} pokemonFront;

typedef struct {
	uint32_t pal;
	uint32_t idk;
} pokemonPal;

#define ADDR2OFFSET(x) (x - 0x8000000)

void *readBuf(uint8_t *buffer, uint32_t address) {
	return ((buffer + ADDR2OFFSET(address)));
}

void extract_image(uint8_t *buffer, uint32_t imgAddr, uint32_t palAddr, int compImg,
		int compPal, uint32_t **out) {
	/*
	 * Calculate the offset of the image
	 */
	uint32_t destSize, srcSize, *pixels = NULL, *image = NULL;
	uint8_t *output = NULL, *offset = NULL;
	size_t i;
	offset = (uint8_t *) (buffer + ADDR2OFFSET(imgAddr));
	srcSize = (1 << 24) - ADDR2OFFSET(imgAddr);

	/*
	 * Detect compression of the image data and uncompress if needed
	 */
	if (compImg) {
		decompress((const uint8_t *) offset, &output, &destSize, srcSize);
	} else {
		output = offset;
	}


	/*
	 * Calculate the offset of the palette and uncompress if needed
	 */
	uint16_t *paletteGBA = NULL;
	uint8_t *paletteComp = (uint8_t *) (ADDR2OFFSET(palAddr) + buffer);
	srcSize = (1 << 24) - ADDR2OFFSET(palAddr);

	/*
	 * Detect compression of the palette data
	 */
	if (compPal) {
		decompress((const uint8_t *) paletteComp, (uint8_t **) &paletteGBA,
		NULL, srcSize);
	} else {
		paletteGBA = (uint16_t *) paletteComp;
	}

	/*
	 * Convert the 555 GBA colours in the palette to a 32 bit RGBA colour space
	 */
	uint32_t paletteRGBA[16] = { };
	for (i = 0; i < 16; i++) {
		uint32_t r, g, b, a;
		r = ((paletteGBA[i] >> 0) & 0x1F) << (3);
		g = ((paletteGBA[i] >> 5) & 0x1F) << (3 + 8);
		b = ((paletteGBA[i] >> 10) & 0x1F) << (3 + 16);
		a = (i ? 0xFF : 0x0) << 24;
		paletteRGBA[i] = r | g | b | a;
	}

	/*
	 * Convert the indexed image to a RGBA bitmap
	 */
	int length = deindex_4bpp(output, destSize, paletteRGBA, &pixels);

	/*
	 * We only free this if space was malloced (during compression)
	 */
	if (compImg) free(output);

	/*
	 * Convert the array of 8x8 GBA tiles to a standard image pixel array
	 */
	detile(pixels, length, 8, &image);

	/*
	 * Return the output pointer and clean up
	 */
	*out = image;
	free(pixels);
}

int main(int argc, char **argv) {
	uint8_t *buffer;
	FILE *rom;
	size_t i = 0, totalPokemon = 412;

	rom = fopen("BPRE0.gba", "rb");
	buffer = (uint8_t *) malloc(1 << 24);
	fread(buffer, 1, 1 << 24, rom);
	fclose(rom);

	for (i = 0; i < totalPokemon; ++i) {
		//save_png_to_file(image, 64, 64, filename);
	}

	free(buffer);
	return 0;
}
