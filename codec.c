#include <stdint.h>
#include <stdlib.h>

int deindex_4bpp(uint8_t *data, int length, uint32_t *palette, uint32_t **out) {
    int i;
    uint32_t pixel, pixel2;
    uint8_t index, index2;

    uint32_t *output = (uint32_t *) malloc(length * 2 * sizeof(uint32_t));
    *out = output;

    for (i = 0; i < length; ++i) {
        /* Read both pixels in this byte, and get the appropriate colour from
        the palette */
        index = data[i] & 0xF;
        pixel = palette[index];
        index2 = (data[i] >> 4);
        pixel2 = palette[index2];

        /* Pack the first pixel into the output */
        *output = (pixel & 0xFFFFFFFF);// | 0xFF000000;
        output++;

        /* Pack the second pixel into the output */
        *output = (pixel2 & 0xFFFFFFFF);// | 0xFF000000;
        output++;
    }

    return length * 2;
}

int detile(uint32_t *tiles, int length, int width, uint32_t **out) {
    int i, x = 0, y = 0, j = 0, k = 0;
    int tile, pixel;

    uint32_t *output = (uint32_t *) malloc(length * sizeof(uint32_t));
    *out = output;

    for (i = 0; i < length; ++i) {
        tile = i / 64;
        pixel = i % 64;

        y = tile / width;
        j = pixel / 8;
        x = tile % width;
        k = pixel % 8;

        output[((y * 8) + j) * width * 8 + (x * 8) + k] = tiles[i];
    }

    return 0;
}
