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

	This file contains various methods for generating and rendering procedural 
	textures based on the Lattice Noise method.
*/

#ifndef LATTICENOISE_H
#define LATTICENOISE_H

/**
	Represents a noise lattice.
*/
struct ln_lattice_s
{	
	/**
		The actual values of the lattice.

		Methods are provided for accessing lattices up to 4 dimensions, but the 
		library allows higher dimensions still.

		To access a value in a 5D lattice you'd calculate the final index as such:

			def val(x, y, z, w, q):
				x = x
				y = y * m
				z = z * m * m
				w = w * m * m * m
				q = q * m * m * m * m
				return lattice.values[x + y + z + w + q]

		Where m is the dim_length value of ln_lattice.

	*/
	float *values;

	/* 
		NOTE:	PLEASE DO NOT TOUCH THESE VALUES MANUALLY, UNLESS YOU ABSOLUTELY 
				KNOW WHAT YOU ARE DOING.
				EVEN THEN, I'M NOT SURE YOU SHOULD TOUCH THESE. IT COULD COMPLETELY
				BORK YOUR PROGRAM/MEMORY.
	*/


	/** 
		The length of one side of the lattice. The lattice will always be square,
		cubic etc.

		This value can always be used to index into one dimension of the lattice, 
		like so: index = val & my_lattice.length;
	*/ 
	unsigned int dim_length;
	/**
		Total number of values in the lattice, equal to pow(dim_length, n) where
		n is the number of dimensions of the lattice.
	*/ 
	unsigned int size;
	/**
		The value that was used to seed the RNG upon construction of the lattice.
	*/
	unsigned long seed;
	/**
		The number of dimensions in the lattice.
	*/
	unsigned int dimensions;
};

typedef struct ln_lattice_s *ln_lattice;

/* 
	GENERAL FUNCTIONS.
	---------------------------------------------------------------------------------
*/

/**
	Provides a custom RNG.
*/
typedef struct ln_rng_func_def_s
{
	/**
		A custom RNG that gets passed the custom state on each subsequent call.

		\return		a value between 0.0 and 1.0.
	*/
	float (*func)(void *state);
	/** 
		The seed value used to initialize the RNG.
	*/
	unsigned long seed;
	void *state;
} ln_rng_func_def;

/**
	Creates a new lattice, allocates the memory needed and feeds it with values.

	The lattice has an upper bound of (2 ^ 32) - 1 values.
	
	\param	dimensions
					How many dimensions the lattice has. It must be >= 1.
	\param	dim_length	
					The size in one dimension of the lattice. It must be >= 1. The 
					total size is pow(dim_length, dimensions).
	\param	rng_func
					A pointer to a ln_rng_func_def. This parameter is optional, leave 
					it to NULL to use the default C RNG.

	\return 		A new lattice object on success.
			 		NULL if:
			 			dimensions < 1
						dim_length < 1
						pow(dim_length, dimensions) > MAX(unsigned int)
						memory could not be allocated (out of memory.)

*/
extern ln_lattice ln_lattice_new(
	unsigned int dimensions, 
	unsigned int dim_length, 
	ln_rng_func_def *rng_func);

/**
	Frees an allocated lattice. You should always call
	this when you are done with your lattice.
*/
extern void ln_lattice_free(ln_lattice lattice);

/**
	Retrieves a value from a 1D lattice.

	\return			The value at coordinate x in the lattice or
					infinity if lattice.dimensions != 1 or the coordinate are
					out of bounds.
*/
extern float ln_lattice_value1(
	ln_lattice lattice, 
	unsigned int x);

/**
	Retrieves a value from a 2D lattice.

	\return			The value at coordinates (x, y) in the lattice or
					infinity if lattice.dimensions != 2 or the coordinates are
					out of bounds.
*/
extern float ln_lattice_value2(
	ln_lattice lattice, 
	unsigned int x, 
	unsigned int y);

/**
	Retrieves a value from a 3D lattice.

	\return			The value at coordinates (x, y, z) in the lattice or
					infinity if lattice.dimensions != 3 or the coordinates are
					out of bounds.
*/
extern float ln_lattice_value3(
	ln_lattice lattice, 
	unsigned int x, 
	unsigned int y, 
	unsigned int z);

/**
	Retrieves a value from a 4D lattice.

	\return			The value at coordinates (x, y, z, w) in the lattice or
					infinity if lattice.dimensions != 4 or the coordinates are
					out of bounds.
*/
extern float ln_lattice_value4(
	ln_lattice lattice, 
	unsigned int x, 
	unsigned int y, 
	unsigned int z, 
	unsigned int w);

/**
	Gets an interpolated noise value at coordinate x, if x > dim_length or 
	x < dim_length, it wraps around, so the lattice repeats infinitely.

	Uses cubic interpolation.
*/
extern float ln_lattice_noise1d(ln_lattice lattice, float x);

/**
	Gets an interpolated noise value at coordinate (x, y), if x > dim_length or 
	x < dim_length (same for y), it wraps around, so the lattice repeats infinitely 
	in 2D-space.

	Uses cubic interpolation.
*/
extern float ln_lattice_noise2d(ln_lattice lattice, float x, float y);

/* 
	FRACTAL SUMS. 
	---------------------------------------------------------------------------------
*/

/*
	Defines the options for a fractal sum operation.

	A fractal sum samples the noise lattice n times according to the
	following formula:

		offset + noise(p) + 1/2noise(2p) + 1/4noise(4p) ...

	Where p is a point in space and noise is the noise sampling function,
	for example ln_lattice_noise2d.
*/
typedef struct ln_fsum_options_s
{
	/*
		Number of terms in the fractal sum.

		Anything < 1 is illegal.
	*/
	unsigned int n;
	/*	
		A kind of scale on the fractal sum.

		It is raised to the index of each term, term zero
		has index 0.

		For example, for octaves = 2, amplitude_ratio = 0.5,
		frequency_ratio = 2:

			(0.5)noise(p) + (0.25)noise(2p) + ...

	*/
	float amplitude_ratio;
	/*
		Each argument to the noise function is scaled with this.
	*/
	float frequency_ratio;
	/*	
		A simple offset term. Added to the fractal sum.
	*/
	float offset;
} ln_fsum_options;
/*
	Gets a ln_fsum_options structure with default options emulating
	the classic "fbm" noise:
	
		n 				= 4
		amplitude_ratio = 1/2
		frequency_ratio = 2
*/
extern ln_fsum_options ln_default_fsum_options();
/*
	Calculates the maximum possible value that a fractal sum can result in by 
	assuming all the samples are 1.0f.
	
	Useful for normalizing the noise generated by a fractal sum operation.
	\return			The maximum possible value that ln_lattice_fsum* functions
					can output with the given settings.
					
					Returns infinity on error.
					Error conditions are:
						- ln_fsum_options.n < 1
*/ 
extern float ln_fsum_max_value(ln_fsum_options const *);

/** 
	Does a fractal sum using the given lattice and options.
	
	\return			The fractal sum value at the given coordinates or infinity 
					if there was an error.
					Error conditions are:
						- ln_fsum_options.n < 1
						- the lattice is not 1D.						
*/
extern float ln_lattice_fsum1d(ln_lattice lattice, float x, ln_fsum_options const *);
/**
	The 2D version of ln_lattice_fsum1d.
	
	See ln_lattice_fsum1d for more info.
	
	\return			The fractal sum value at the given coordinates or infinity 
					if there was an error.
					Error conditions are:
						- ln_fsum_options.n < 1
						- the lattice is not 2D.
*/
extern float ln_lattice_fsum2d(
	ln_lattice lattice, float x, float y, ln_fsum_options const *);

#endif