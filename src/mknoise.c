/*
	Copyright (c) 2012, Simon Otter
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the FreeBSD Project.
*/

/** \file

	mknoise.c

	Implements the mknoise program that complements the latticenoise library.
*/

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

#include "math.h"

#include "latticenoise.h"

#define PRINTERRF(...) fprintf(stderr, __VA_ARGS__)
#define ABORTIF(expr, ...) \
	if ((expr)) \
	{ \
		PRINTERRF(__VA_ARGS__); abort(); \
	}

/* -----------------------------------
	TGA FUNCTIONS. 
   ---------------------------------*/
typedef struct tga_data_s
{
	/* 
		The image data is arranged from top left to bottom right,
		BGRA (32 bit) or BGR (24 bit).

		Thus data[0] is the blue byte of the topmost left corner.
	*/
	uint8_t  *data;
	uint16_t width;
	uint16_t height;
	/* 24 or 32 (32 is with alpha mask.) */
	uint8_t  bitdepth;
} tga_data;

static uint64_t tga_len(uint16_t w, uint16_t h, uint8_t bitdepth)
{
	uint64_t bytespp = bitdepth == 24? 3 : 4;
	uint64_t len = ((uint32_t) w) * ((uint32_t) h) * bytespp;
	return len;
}

// bithdepth 24 or 32
static tga_data *tga_create(uint32_t w, uint32_t h, uint8_t bitdepth)
{
	if (bitdepth != 24 && bitdepth != 32)
		return NULL;

	tga_data *tga = malloc(sizeof(tga_data));
	if (tga != NULL)
	{
		tga->width = w;
		tga->height = h;
		tga->bitdepth = bitdepth;

		uint64_t len = tga_len(w, h, bitdepth);
		tga->data = malloc(len);

		if (tga->data == NULL)
		{
			free(tga);
			tga = NULL;
		}
	}
	return tga;
}

static void tga_free(tga_data *tga)
{
	if (tga != NULL)
		free(tga->data);
	free(tga);
}

static void tga_write(tga_data *data, FILE *f)
{
	/* Write the header. */

	/* Identification field. */
	/* We don't support the image identification field. */
	putc(0, f);

	/* Color map type. */
	/* Always zero because we don't support color mapped images. */
	putc(0, f);

	/* Image type code. */
	/* Always 2 since we only support uncompressed RGB. */
	putc(2, f);

	/*  Color map specification, not used. */
	for (int i = 0; i < 5; i++)
		putc(0, f);

	/* X-origin. lo-hi 2 byte integer. */
	putc(0, f); putc(0, f);

	/* Y-origin. lo-hi 2 byte integer. */
	putc(0, f); putc(0, f);

	/* Image width. lo-hi 2 byte integer. */
	putc(data->width & 0x00FF, f);
	putc((data->width & 0xFF00) >> 8, f);

	/* Image height. lo-hi 2 byte integer. */
	putc(data->height & 0x00FF, f);
	putc((data->height & 0xFF00) >> 8, f);

	/* Image Pixel Size (amount of bits per pixel.) */
	putc(data->bitdepth, f);

	/* Image Descriptor Byte */
	putc(0x20 | (data->bitdepth == 32? 0x08 : 0x00), f);

	/* Write the image data. */
	uint64_t len = tga_len(data->width, data->height, data->bitdepth);

	for (uint64_t i = 0; i < len; ++i)
	{
		putc(data->data[i], f);
	}
}

/* -----------------------------------
	ARGUMENT PARSING.
   ---------------------------------*/

uint32_t utf8_strlen(char *str) 
{
  uint32_t i = 0, j = 0;
  while (str[i]) 
  {
    if ((str[i] & 0xc0) != 0x80) 
    	j++;
    i++;
  }
  return j;
}

typedef	struct mknoise_args_s
{
	char     outpath[0xFFF];
	/* Image width. */
	uint32_t width;
	/* Image height. */
	uint32_t height;
	/* Length of output path. */
	uint16_t outpath_len;
	/* Value to seed the RNG with. */
	uint32_t seed;
} mknoise_args;

bool parse_args(int argc, char *argv[], mknoise_args *output)
{
	if (argc < 3)
	{
		return 0;
	}
}

/* -----------------------------------
	TEST FUNCTIONS. 
   ---------------------------------*/
static void tga_test()
{
	tga_data *tga = tga_create(128, 128, 24);

	uint64_t len = tga_len(tga->width, tga->height, tga->bitdepth);
	for (uint64_t i = 0; i < len; i++)
	{
		uint8_t v = (i % 8) * 32;
		tga->data[i] = v;
	}

	/* Mark upper left corner red. */
	tga->data[0] = 0x00;
	tga->data[1] = 0x00;
	tga->data[2] = 0xFF;

	FILE *f = fopen("test.tga", "wb");

	tga_write(tga, f);

	tga_free(tga);

	fclose(f);
}

static void test_write_lattice(ln_lattice lattice)
{
	if (lattice->dimensions != 2)
	{
		puts("Dimensions is not 2.\n");
		goto die_horribly;
	}

	if (lattice->dim_length > 65535)
	{
		puts("dim_length can't be larger than 2^16-1.\n");
		goto die_horribly;
	}

	tga_data *tga = tga_create(lattice->dim_length, lattice->dim_length, 24);
	if (tga == NULL)
	{
		puts("Memory exhausted. Good Bye.\n");
		goto die_horribly;
	}

	uint64_t len = tga_len(tga->width, tga->height, tga->bitdepth);

	for (uint64_t i = 0; i < len; i += 3)
	{
		uint32_t x = (i / 3) % lattice->dim_length;
		uint32_t y = (i / 3) / lattice->dim_length;
		float v = ln_lattice_value2(lattice, x, y);
		if (v == INFINITY)
			puts("BUG! We hit INFINITY!");
		uint8_t bv = (uint8_t) (v * 255.9);

		tga->data[i] = bv;
		tga->data[i + 1] = bv;
		tga->data[i + 2] = bv;
	}

	FILE *f = fopen("test.tga", "wb");

	tga_write(tga, f);

	tga_free(tga);

	fclose(f);

	return;

die_horribly:
	ln_lattice_free(lattice);
	abort();
}

int main(int argc, char *argv[])
{
	ln_lattice lattice = ln_lattice_new(2, 32, NULL);
	
	tga_data *tga = tga_create(128, 128, 24);

	uint64_t len = tga_len(128, 128, 24) / 3;
	
	float *vals = malloc(len * 3 * sizeof(float));
	ABORTIF(vals == NULL, "Out of memory.\n");

	float maxN = 1.0;
	for (uint64_t i = 0; i < len; ++i)
	{
		uint64_t offset = i * 3;
		float x = ((float) (i % 128)) / 4.0f;
		float y = ((float) (i / 128)) / 4.0f;

		float n = 
			ln_lattice_noise2d(lattice, x, y) 
			+ (ln_lattice_noise2d(lattice, x * 2, y * 2) / 2)
			+ (ln_lattice_noise2d(lattice, x / 4, y / 4) * 4);
		if (n == INFINITY)
			puts("Bug in ln_lattice_noise2d somewhere.");
		if (n > maxN)
			maxN = n;
		vals[offset] = n;
		vals[offset+1] = n;
		vals[offset+2] = n;
	}

	/* We are no longer in the [0.0, 1.0) range, so we'll have to
	   scale the range back to 0.0 - 1.0. */
	float rescale = 1.0 / maxN;
	for (uint64_t i = 0; i < len; ++i)
	{
		uint64_t offset = i * 3;
		float v = vals[offset] * rescale;
		uint8_t x = (uint8_t)(v * 255.f);
		tga->data[offset] =   x;
		tga->data[offset+1] = x;
		tga->data[offset+2] = x;
	}

	free(vals);

	FILE *f = fopen("test.tga", "wb");

	tga_write(tga, f);

	tga_free(tga);

	fclose(f);

	ln_lattice_free(lattice);

	return 0;
}
