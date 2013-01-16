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

	It contains common functions as well as specific implementations such as 
	Perlin Noise and Turbulence.
*/

/**
	Represents a noise lattice.
*/
struct ln_lattice_s
{	
	/**
		The actual values of the lattice.

		Methods are provided for accessing lattices up to 4 dimensions, but the 
		library allows higher dimensions still.

		To access a value in a 5D library you'd calculate the final index as such:

			def val(x, y, z, w, q):
				x = x
				y = y * m
				z = z * m * m
				w = w * m * m * m
				q = q * m * m * m * m
				return lattice.values[x + y + z + w + q]

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
	int seed;
	/**
		The number of dimensions in the lattice.
	*/
	unsigned int dimensions;
};

typedef ln_lattice_s *ln_lattice;

// LATTICE FUNCTIONS.

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
	\param	seed 	A seed value for the RNG used to initialize the lattice.
					It is optional and may be set to 0, if so the function seeds 
					the RNG itself.
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
	int seed, 
	ln_rng_func_def rng_func);

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