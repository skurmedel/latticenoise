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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* For seeding the default RNG. */
#include <time.h>

/* Some pre-declares. */
inline static float lerp(float, float, float);
inline static float catmull_rom(float p0, float p1, float p2, float p3, float x);
inline static float hermite01(float p0, float m0, float p1, float m1, float t);

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

inline float clamp01(float v)
{
	if (v < 0.0f)
		return 0.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
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
		lattice->values[i] = clamp01(v);
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
	Simply takes the value modulo dim_length to ensure we are always inside the
	lattice.
*/
#define WRAP(offset) ((offset) % lattice->dim_length)

float ln_lattice_noise1d(ln_lattice lattice, float x)
{
	/*
		Map x into the lattice space.
	*/
	x = fmodf(fabs(x), (float) lattice->dim_length);
	float fix; 
	/*
		We rip out the fractional part, we will use this for interpolation for 
		x-coordinates between lattice points.
		
		We use the integer part (fix) to actually get the discrete lattice 
		values.
	*/
	float r = modff(x, &fix);
	
	/*
		lattice->dim_length is unsigned int, and we have already computed x 
		module dim_length, so fix will always fit into an unsigned int.
	*/
	unsigned int uix = (unsigned int) fix;

	float p0 = ln_lattice_value1(lattice,  WRAP(uix - 1));
	float p1 = ln_lattice_value1(lattice,  WRAP(uix));
	float p2 = ln_lattice_value1(lattice,  WRAP(uix + 1));
	float p3 = ln_lattice_value1(lattice,  WRAP(uix + 2));

	return catmull_rom(p0, p1, p2, p3, r);
}

float ln_lattice_noise2d(ln_lattice lattice, float x, float y)
{
	/*
		See the 1D-version for a description of this. 
		We just do the same thing twice.
	*/
	x = fmodf(fabs(x), (float) lattice->dim_length);
	y = fmodf(fabs(y), (float) lattice->dim_length);
	float fix; float fiy;
	float r1 = modff(x, &fix);
	float r2 = modff(y, &fiy);

	unsigned int uix = (unsigned int) fix;
	unsigned int uiy = (unsigned int) fiy;
	
	/*
		Compute 4 interpolated values across x for each y-index.
		Then interpolate along y.
	*/	
	float v[4] = {0, 0, 0, 0};

	size_t curr = 0;
	unsigned int y_base = WRAP(uiy - 1);
	for (unsigned int i = 0; i < 4; ++i)
	{
		float p0 = ln_lattice_value2(lattice, WRAP(uix - 1),  y_base);
		float p1 = ln_lattice_value2(lattice, WRAP(uix),      y_base);
		float p2 = ln_lattice_value2(lattice, WRAP(uix + 1),  y_base);
		float p3 = ln_lattice_value2(lattice, WRAP(uix + 2),  y_base);
		
#ifdef LN_DEFAULT_HERMITE_INTERPOLATION
		/* 
			We use the slope between p0, p2 and p1, p3 here as tangents.
			It gives a reasonably smooth "continous" interpolation.
		*/
		v[curr++] = hermite01(p1, (p2 - p0) / 3.0f, p2, (p3 - p1) / 3.0f, r1);
#else
		/* 
			For Catmull-Rom we just use the samples as four points.
			
			The Catmull-Rom interpolation is guaranteed to go through the 
			points.
		*/
		v[curr++] = catmull_rom(p0, p1, p2, p3, r1);
#endif
		y_base = WRAP(y_base + 1);
	}

	/*
		We can actually wind up with values outside [0.0, 1.0] here so we clamp 
		the value and hope for the best.
	*/
	float r = 0.0f;
#ifdef LN_DEFAULT_HERMITE_INTERPOLATION
	r = clamp01(hermite01(v[1], (v[2] - v[0]) / 3.0f, v[2], (v[3] - v[1]) / 3.0f, r2));
#else
	r = clamp01(catmull_rom(v[0], v[1], v[2], v[3], r2));
#endif
	return r;
}

ln_fsum_options ln_default_fsum_options()
{
	ln_fsum_options options;
	options.n = 4;
	options.amplitude_ratio = 0.5f;
	options.frequency_ratio = 2.0f;
	options.offset = 0.0f;
	return options;
}

#define FSUM_IMPLEMENTATION(call, dims)\
	if (opt->n < 1 || lattice->dimensions != (dims))\
		return INFINITY;\
	\
	float result = opt->offset;\
	\
	float a = 1;\
	float f = 1;\
	for (unsigned int i = 0; i < opt->n; ++i)\
	{		 \
		result += a * (call);\
		a *= opt->amplitude_ratio;\
		f *= opt->frequency_ratio;\
	}\
	return result;\


float ln_lattice_fsum1d(ln_lattice lattice, float x, ln_fsum_options const *opt)
{
	FSUM_IMPLEMENTATION(ln_lattice_noise1d(lattice, f * x), 1)
}

float ln_lattice_fsum2d(ln_lattice lattice, float x, float y, ln_fsum_options const *opt)
{
	FSUM_IMPLEMENTATION(ln_lattice_noise2d(lattice, f * x, f * y), 2)
}

float ln_fsum_max_value(ln_fsum_options const *opt)
{
	if (opt->n < 1)
		return INFINITY;
	float v = opt->offset;
	/*
		In the algorithm above, we always add one term with 1.0f amplitude.
	*/
	v += 1.0f;
	/* 
		The maximum posssible value the fractal sum methods would output is the 
		one where each call to ln_lattice_noise* returns a 1.0f.
		
		That is:
			0 <= ln_lattice_noise* <= 1.0f 
		which means that 
			ln_lattice_noise* + ln_lattice_noise* + ... <= 1.0 + 1.0 + ... 
		
		So with r = amplitude_ratio and f = frequency_ratio we have 
			r^i * ln_lattice_noise*(f^i p) <= r^i * 1.0  
					
		The amplitude_ratios form a geometric series if n is allowed to be 
		infinite, but of course opt->n is finite so we don't have to worry about
		convergence. It will always have a well-defined value.
		
		We can use the formula for the n first elements of a geometric series 
		to obtain the exact value, unless amplitude_ratio is 1.0.
		
		If amplitude_ratio is 1.0 the result is just 1.0 * opt->n.
	*/
	float r = opt->amplitude_ratio;
	if (r != 1.0f)
		v = (1.0f - powf(r, opt->n)) / (1.0f - r);
	else
		v = opt->n; 
	return v;
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

inline static float hermite01(
	float p0,
	float m0,
	float p1,
	float m1,
	float t)
{
	float h00 = 2.0f * t * t * t - 3.0f * t * t + 1;
	float h10 = t * t * t - 2.0f * t * t + t;
	float h01 = t * t * (3.0f - 2.0f * t);
	float h11 = t * t * (t - 1.0f);
	
	return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

static float lerp(float a, float b, float r)
{
	return a + r * (b - a);
}

