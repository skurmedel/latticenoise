/*
	2012, Simon Otter
	All rights reserved.

	This is free and unencumbered software released into the public domain.

	Anyone is free to copy, modify, publish, use, compile, sell, or
	distribute this software, either in source code form or as a compiled
	binary, for any purpose, commercial or non-commercial, and by any
	means.

	In jurisdictions that recognize copyright laws, the author or authors
	of this software dedicate any and all copyright interest in the
	software to the public domain. We make this dedication for the benefit
	of the public at large and to the detriment of our heirs and
	successors. We intend this dedication to be an overt act of
	relinquishment in perpetuity of all present and future rights to this
	software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

	For more information, please refer to <http://unlicense.org/>
*/

/** \file

	mknoise.c

	Implements the mknoise program that complements the latticenoise library.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <math.h>
#include <time.h>

#include "latticenoise.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "parg.h"

#define PRINTERRF(...) fprintf(stderr, __VA_ARGS__)
#define ABORTIF(expr, ...) \
	if ((expr)) \
	{ \
		PRINTERRF(__VA_ARGS__); abort(); \
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

#define NOISE_METHOD_PERLIN		0
#define NOISE_METHOD_FSUM		1

#define NOISE_FORMAT_UNKNOWN	0
#define NOISE_FORMAT_JPEG		1
#define NOISE_FORMAT_PNG		2
#define NOISE_FORMAT_TGA		3
#define NOISE_FORMAT_BMP		4


typedef	struct mknoise_args_s
{
	/* Whether to run benchmark instead. */
	uint32_t benchmark;
	/* Method to use for noise creation. */
	uint32_t method;
	char     outpath[0xFFF];	
	/* Image width. */
	uint32_t width;
	/* Image height. */
	uint32_t height;
	/* Length of output path. */
	uint16_t outpath_len;
	/* Value to seed the RNG with. */
	uint32_t seed;
	/* File format. */
	uint8_t	format;	
	/* Scale for the lattice, basically how a pixel position maps to the lattice
	   coordinates.  */
	float 	scale;
	/* Iterations when doing fractal sum. */
	ln_fsum_options fsum_opts;
} mknoise_args;

uint8_t find_format_from_path(char const *path)
{
	char *p = strrchr(path, '.');
	if (p == NULL || strlen(p) < 4)
		return NOISE_FORMAT_UNKNOWN;
	char c1 = tolower(*(p + 1));
	char c2 = tolower(*(p + 2));
	char c3 = tolower(*(p + 3));
	if (c1 == 't' && c2 == 'g' && c3 == 'a')
		return NOISE_FORMAT_TGA;
	else if (c1 == 'p' && c2 == 'n' && c3 == 'g')
		return NOISE_FORMAT_PNG;
	/*else if (c1 == 'j' && c2 == 'p' && c3 == 'g')
		return NOISE_FORMAT_JPEG;*/
	else
		return NOISE_FORMAT_UNKNOWN;
}

char const *format_to_str(uint8_t format)
{
	switch (format)
	{
		case NOISE_FORMAT_BMP:
			return "BMP";
		case NOISE_FORMAT_JPEG:
			return "JPEG";
		case NOISE_FORMAT_PNG:
			return "PNG";
		case NOISE_FORMAT_TGA:
			return "TGA";
		default:
			return "Unknown";	
	}
}

#define EPRINT(msg){\
	fprintf(stderr, (msg));\
	fputc('\n', stderr);\
}

#define EPRINT_AND_EXIT(msg, ecode){\
	EPRINT(msg);\
	exit((ecode));\
}

void parse_options(int argc, char *argv[], mknoise_args *out)
{
	struct parg_state ps;
	
	out->scale = 4.0f;
	out->fsum_opts = ln_default_fsum_options();

	parg_init(&ps);
	int c;
	int nonoptions = 0;
	while ((c = parg_getopt(&ps, argc, argv, "hm:s:bS:n:")) != -1)
	{
		switch (c)
		{
			case 1:
				if (nonoptions == 0)
				{
					out->width = atoi(ps.optarg);
				}
				else if (nonoptions == 1)
				{
					out->height = atoi(ps.optarg);
				}
				else if (nonoptions == 2)
				{
					strncpy(out->outpath, ps.optarg, 0xFFF - 1);
				}
				nonoptions++;
				break;
			case 'S':
				out->scale = (float) atof(ps.optarg);
				if (out->scale == 0.0)
					EPRINT_AND_EXIT("ARGS: Invalid noise scale.", -3);
				break;
			case 'b':
				out->benchmark = 1;
				break;
			case 'm':
				if (strcmp(ps.optarg, "fsum") == 0)
				{
					out->method = NOISE_METHOD_FSUM;
				}
				else if (strcmp(ps.optarg, "perlin") == 0)
				{
					out->method = NOISE_METHOD_PERLIN;					
				}
				else
				{
					fprintf(stderr, "ARGS: Unknown method value: %s\n", ps.optarg);
					exit(-3);
				}
				break;
			case 's':
				out->seed = atoi(ps.optarg);
				break;
			case 'n':
				out->fsum_opts.n = atoi(ps.optarg);
				if (out->fsum_opts.n < 1)
				{
					fprintf(stderr, "ARGS: Invalid number of iterations %d for fsum.", out->fsum_opts.n);
					exit(-3);
				}
				break;
			case 'h':
				fprintf(stdout, "Usage: mknoise [-m] [-h] WIDTH HEIGHT FILENAME\n");
				fprintf(stdout, "       -m\tmethod flag, has options perlin ");
				fprintf(stdout, "and fsum. fsum is a fractal sum which gives ");
				fprintf(stdout, "a more turbulent kind of noise\n");
				fprintf(stdout, "       -h\tprint this help\n");
				fprintf(stdout, "       -b\trun benchmarks.\n");
				fprintf(stdout, "       -S\tset noise frequency scale.\n");
				fprintf(stdout, "       -n\twhen using fsum method, sets the iterations\n");
				exit(0);
				break;
			case '?':
				exit(-2);
			default:
				fprintf(stderr, "ARGS: Unknown option -%c\n", c);
				exit(-1);
				break;
		}
	}
	
	if (out->benchmark != 1)
	{
		if (out->width == 0 || out->height == 0)
		{
			fputs("ARGS: Illegal size specified.", stderr);
			exit(-3);
		}
		if (nonoptions < 3)
		{
			fputs("ARGS: Missing argument.", stderr);
			exit(-2);
		}
	}
	
	out->format = find_format_from_path(out->outpath);
}

/* -----------------------------------
	TEST FUNCTIONS. 
   ---------------------------------*/

void benchmark()
{
	#if 0
	ln_lattice lattice = ln_lattice_new(2, 128, NULL);
	
	tga_data *tga = tga_create(4096, 4096, 24);

	uint64_t len = tga_len(tga->width, tga->height, 24) / 3;
	
	float *vals = malloc(len * 3 * sizeof(float));
	ABORTIF(vals == NULL, "Out of memory.\n");

	printf("Benchmarking started...\n");

	float maxN = 1.0;

	int loops = 10; double secs_acc = 0.0;
	for (int loop_i = 0; loop_i < loops; ++loop_i)
	{
		clock_t start, end;
		start = clock();

		for (uint64_t i = 0; i < len; ++i)
		{
			uint64_t offset = i * 3;
			float x = ((float) (i % 4096)) / 4.0f;
			float y = ((float) (i / 4096)) / 4.0f;

			float n = 
				ln_lattice_noise2d(lattice, x, y);
			if (n == INFINITY)
				puts("Bug in ln_lattice_noise2d somewhere.");
			if (n > maxN)
				maxN = n;
			vals[offset] = n;
			vals[offset+1] = n;
			vals[offset+2] = n;
		}
		end = clock();
		secs_acc += ((double) (end - start)) / CLOCKS_PER_SEC;
	}

	printf("Average Seconds Spent: %f.\n", secs_acc / loops);

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
	#endif
	puts("Benchmarking not implemented.");
}

int write_image_data(char const *fname, uint8_t format, int width, int height, void const *data)
{
	switch (format)
	{
		case NOISE_FORMAT_PNG:
			stbi_write_png(fname, width, height, 3, data, width * 3 * sizeof(char));
			break;
		case NOISE_FORMAT_BMP:
			stbi_write_bmp(fname, width, height, 3, data);
			break;
		case NOISE_FORMAT_TGA:
			stbi_write_tga(fname, width, height, 3, data);
			break;
		case NOISE_FORMAT_JPEG:
	default:
		return 0;
	}
	
	return 1;
}

inline float clamp01(float v)
{
    if (v < 0.0f)
    {
       return 0.0f;
    }
    else if (1.0f < v)
    {
        return 1.0f;
    }
    return v;
}

void output_noise_image(mknoise_args const *args)
{
	char *rgb = malloc(sizeof(char) * 3 * args->width * args->height);
	if (rgb == NULL)
	{
		EPRINT_AND_EXIT("Could not allocate image buffer.", -4);
	}
	
	/* Todo: Make lattice size a setting. */ 
	ln_lattice lattice = ln_lattice_new(2, 256, NULL);
	
	if (lattice == NULL)
	{
		free(rgb);
		EPRINT_AND_EXIT("Could not allocate noise lattice. Possibly memory error.", -4);
	}
	
	float fsumnorm = 1.0f / ln_fsum_max_value(&args->fsum_opts);
	
	for (size_t y = 0; y < args->height; ++y)
	{
		float fy = (float) y / ((float) args->height) * args->scale;
		for (size_t x = 0; x < args->width; ++x)
		{
			size_t offset = (y * args->width + x) * 3;
			float fx = (float) x / ((float) args->width) * args->scale;
			float v = 0.0f;
			if (args->method != NOISE_METHOD_FSUM)
				v = ln_lattice_noise2d(lattice, fx, fy);
			else
				v = ln_lattice_fsum2d(lattice, fx, fy, &args->fsum_opts) * fsumnorm;
			if (v == INFINITY)
				EPRINT_AND_EXIT("Value with infinity detected, bug in library.", -100);
			
			v = clamp01(v);
			if (v > 1.0f)
				printf("Found value with %f\n", v);
			
			rgb[offset + 0] = (char) (v * 254.999f);
			rgb[offset + 1] = (char) (v * 254.999f);
			rgb[offset + 2] = (char) (v * 254.999f);
		}
	}
	
	if (!write_image_data(
		args->outpath, 
		args->format,
		args->width,
		args->height,
		rgb))
	{
		ln_lattice_free(lattice);
		free(rgb);
		EPRINT_AND_EXIT("Unknown image format.", -6);
	}
	
	printf("Wrote (at least) %lu pixels to %s!\n", (long unsigned) args->width * args->height, args->outpath);
	
	ln_lattice_free(lattice);
	free(rgb);
}

int main(int argc, char *argv[])
{
	mknoise_args args;
	parse_options(argc, argv, &args);
	
	if (args.benchmark == 1)
	{
		benchmark();		
	}
	else 
	{
		if (args.method == NOISE_METHOD_FSUM)
			puts("Using fractal sum noise method.");
		else
			puts("Using perlin noise method.");
		
		printf("Writing %dx%d %s to '%s'\n", args.width, args.height, format_to_str(args.format), args.outpath);
		output_noise_image(&args);
	}

	return 0;
}
