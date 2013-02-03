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

	latticenoise.h

	Main implementation.
*/

#include "latticenoise.h"
#include "math.h"
#include "stdlib.h"
#include "limits.h"

/* For seeding the default RNG. */
#include "time.h"

/* Some pre-declares. */
inline static float lerp(float, float, float);
inline static float catmull_rom(float p0, float p1, float p2, float p3, float x);

/* rng_func_def that uses the stdlib RNG. */
static float default_rng_func(void *state)
{
	return (float) rand() / (float) RAND_MAX;
}

static ln_rng_func_def default_rng()
{
	struct ln_rng_func_def_s def = {0};

	time_t t = time(NULL);

	def.func = &default_rng_func;
	def.seed = (unsigned long) t * 241;
	def.state = NULL;

	srand(def.seed);

	return def;
}

ln_lattice ln_lattice_new(
	unsigned int dimensions, 
	unsigned int dim_length,
	ln_rng_func_def *rng_func)
{
	if (dimensions < 1 || dim_length < 1)
		return NULL;

	/* We simply use a bigger datatype here and avoid lengthy overflow
	   checks. */
	unsigned long long size = 1;
	for (unsigned int i = 0; i < dimensions; i++)
	{
		size *= (unsigned long long) dim_length;
	}

	if (size > UINT_MAX)
		return NULL;

	unsigned int ulsize = (unsigned int) size;

	/* Set up the Lattice. */
	ln_lattice lattice = malloc(sizeof(struct ln_lattice_s));
	if (lattice == NULL)
		return NULL;

	lattice->values = malloc(ulsize * sizeof(float));
	if (lattice->values == NULL)
		goto die_clean;
	
	/* only used if rng_func == NULL */
	ln_rng_func_def default_rng_def = {0};
	if (!rng_func)
	{
		default_rng_def = default_rng();
		rng_func = &default_rng_def;
	}

	lattice->dimensions = dimensions;
	lattice->dim_length = dim_length;
	lattice->size = ulsize;
	lattice->seed = rng_func->seed;

	/* Initialize the values. */

	for (unsigned int i = 0; i < ulsize; ++i)
	{
		float v = rng_func->func(rng_func->state);
		lattice->values[i] = v;
	}

	return lattice;

die_clean:
	free(lattice->values);
	free(lattice);

	lattice = NULL;

	return lattice;
}

void ln_lattice_free(ln_lattice lattice)
{
	free(lattice->values);
	free(lattice);
}

float ln_lattice_value1(
	ln_lattice lattice, 
	unsigned int x)
{
	if (lattice == NULL || lattice->dimensions != 1 || x > lattice->size - 1)
		return INFINITY;
	return lattice->values[x];
}

float ln_lattice_value2(
	ln_lattice lattice, 
	unsigned int x,
	unsigned int y)
{
	if (lattice == NULL 
		|| lattice->dimensions != 2 
		|| x >= lattice->dim_length
		|| y >= lattice->dim_length)
		return INFINITY;
	return lattice->values[(y * lattice->dim_length) + x];
}

float ln_lattice_value3(
	ln_lattice lattice, 
	unsigned int x,
	unsigned int y,
	unsigned int z)
{
	if (lattice == NULL 
		|| lattice->dimensions != 2 
		|| x >= lattice->dim_length
		|| y >= lattice->dim_length
		|| z >= lattice->dim_length)
		return INFINITY;
	return lattice->values[
		  (z * lattice->dim_length * lattice->dim_length) 
		+ (y * lattice->dim_length) 
		+ x];
}

float ln_lattice_value4(
	ln_lattice lattice, 
	unsigned int x,
	unsigned int y,
	unsigned int z,
	unsigned int w)
{
	if (lattice == NULL 
		|| lattice->dimensions != 2 
		|| x >= lattice->dim_length
		|| y >= lattice->dim_length
		|| z >= lattice->dim_length
		|| w >= lattice->dim_length)
		return INFINITY;
	return lattice->values[
		  (w * lattice->dim_length * lattice->dim_length * lattice->dim_length) 
		+ (z * lattice->dim_length * lattice->dim_length) 
		+ (y * lattice->dim_length) 
		+ x];
}

/*
	If x is minus, we must turn it into the "correct" offset,
	we can't just discard the sign, for example:

		x = -4
		dim_length = 10

		abs(x) = 4

	But we want an infinitely repeating lattice, and if x is negative
	it should point to element #6 in the lattice.

	So we instead do:

		dim_length + x = 5

	If x is positive, the value will of course become bigger than 
	dim_length, but that is no problem, we simply wrap with modulus.

	We also remove the fractional part, we'll use that for interpolation
	later on.
*/
#define FLOAT_TO_OFFSET(x) (((int) lattice->dim_length) + (int) (x))
#define WRAP(offset) ((offset) % lattice->dim_length)

float ln_lattice_noise1d(ln_lattice lattice, float x)
{
	int ix = FLOAT_TO_OFFSET(x);
	float r = x - (int) x;

	/* Sign no longer matters. */
	unsigned int uix = (unsigned int) ix;

	float p0 = ln_lattice_value1(lattice,  WRAP(uix - 1));
	float p1 = ln_lattice_value1(lattice,  WRAP(uix));
	float p2 = ln_lattice_value1(lattice,  WRAP(uix + 1));
	float p3 = ln_lattice_value1(lattice,  WRAP(uix + 2));

	return catmull_rom(p0, p1, p2, p3, r);
}

float ln_lattice_noise2d(ln_lattice lattice, float x, float y)
{
	int ix = FLOAT_TO_OFFSET(x);
	int iy = FLOAT_TO_OFFSET(y);
	
	float r1 = x - (int) x;
	float r2 = y - (int) y;

	unsigned int uix  = WRAP((unsigned int) ix);
	unsigned int uiy  = WRAP((unsigned int) iy);

	float v[4] = {0, 0, 0, 0};

	size_t curr = 0;
	unsigned int y_base = WRAP(uiy - 1);
	for (unsigned int i = 0; i < 4; ++i)
	{
		y_base = WRAP(y_base + 1);
		float p0 = ln_lattice_value2(lattice, WRAP(uix - 1),  y_base);
		float p1 = ln_lattice_value2(lattice, WRAP(uix),      y_base);
		float p2 = ln_lattice_value2(lattice, WRAP(uix + 1),  y_base);
		float p3 = ln_lattice_value2(lattice, WRAP(uix + 2),  y_base);
		v[curr++] = catmull_rom(p0, p1, p2, p3, r1);
	}

	return catmull_rom(v[0], v[1], v[2], v[3], r2);
}

inline static float catmull_rom(
	float p0, 
	float p1, 
	float p2, 
	float p3, 
	float x)
{
	float f0 = p1, f1 = p2;
	float fd0 = (p2 - p0) / 2, fd1 = (p3 - p1) / 2;

	float a = (2 * f0)  - (2 * f1) + fd0 + fd1;
	float b = (-3 * f0) + (3 * f1) - (2 * fd0) - fd1;
	float c = fd0;
	float d = f0;

	float x2 = x * x; float x3 = x2 * x;

	return a * x3 + b * x2 + c * x + d;
}

static float lerp(float a, float b, float r)
{
	return a + r * (b - a);
}

