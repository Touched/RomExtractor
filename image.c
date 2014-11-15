#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int save_png_to_file(uint32_t *pixels, uint32_t width, uint32_t height,
		const char *path) {
	FILE * fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	size_t x, y;
	png_byte ** row_pointers = NULL;
	/* "status" contains the return value of this function. At first
	 it is set to a value which means 'failure'. When the routine
	 has finished its work, it is set to a value which means
	 'success'. */
	int status = -1;
	/* The following number is set by trial and error only. I cannot
	 see where it it is documented in the libpng manual.
	 */
	int pixel_size = 4;
	int depth = 8;

	fp = fopen(path, "wb");
	if (!fp) {
		goto fopen_failed;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		goto png_create_write_struct_failed;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		goto png_create_info_struct_failed;
	}

	/* Set up error handling. */

	if (setjmp(png_jmpbuf (png_ptr))) {
		goto png_failure;
	}

	/* Set image attributes. */

	png_set_IHDR(png_ptr, info_ptr, width, height, depth,
	PNG_COLOR_TYPE_RGBA,
	PNG_INTERLACE_ADAM7,
	PNG_COMPRESSION_TYPE_DEFAULT,
	PNG_FILTER_TYPE_DEFAULT);

	/* Initialize rows of PNG. */

	row_pointers = png_malloc(png_ptr, height * sizeof(png_byte *));
	for (y = 0; y < height; ++y) {
		png_byte *row = png_malloc(png_ptr,
				sizeof(uint8_t) * width * pixel_size);
		row_pointers[y] = row;
		for (x = 0; x < width; ++x) {
			*row++ = ((*pixels) >> 0) & 0xFF;  // R
			*row++ = ((*pixels) >> 8) & 0xFF; // G
			*row++ = ((*pixels) >> 16) & 0xFF; // B
			*row++ = ((*pixels) >> 24) & 0xFF; // A
			pixels++; // A
		}
	}

	/* Write the image data to "fp". */

	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/* The routine has successfully written the file, so we set
	 "status" to a value which indicates success. */

	status = 0;

	for (y = 0; y < height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);

	png_failure: png_create_info_struct_failed: png_destroy_write_struct(
			&png_ptr, &info_ptr);
	png_create_write_struct_failed: fclose(fp);
	fopen_failed: return status;
}
