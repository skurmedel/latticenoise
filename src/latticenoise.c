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
	for (unsigned int i; i < dimensions; i++)
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

	lattice->values = malloc(ulsize);
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