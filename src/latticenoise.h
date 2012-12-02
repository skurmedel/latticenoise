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
	/* 
		NOTE:	PLEASE DO NOT TOUCH THESE VALUES MANUALLY, UNLESS YOU ABSOLUTELY 
				KNOW WHAT YOU ARE DOING.
	*/

	/**
		The actual values of the lattice.

		The memory layout of this array is as follows for an n-dimensional lattice:
 
			dim_length many floats for dimension 1
			dim_length many floats for dimension 2
			...
			dim_length many floats for dimension n

		To find the first value for one axle n: dim_length * (n - 1)
	*/
	float *values;

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
	unsigned short dimensions;
};

typedef ln_lattice_s *ln_lattice;

// point3 TYPE.

/**
	Represents a three dimensional point in space.
*/
typedef struct point_s
{
	float x, y, z;
} point3;

/**
	A simple vector addition.

	\return			(point3){ p1.x + p2.x, p1.y + p2.y, p1.z + p2.z }
*/
#define point3_add(p1, p2) ((point3){p1.x + p2.x, p1.y + p2.y, p1.z + p2.z})

/**
	Prints a point3 to console, purely for debugging purposes.

	No newline is written.
*/
#define point3_print(p1) printf("<%0.5f, %0.5f, %0.5f>", p1.x, p1.y, p1.z)

// LATTICE FUNCTIONS.

/**
	Creates a new lattice.

	\param	size	The size of the lattice. It must be >= 1.

	\param	seed 	A seed value for the RNG used to initialize the lattice.
					It is optional and may be set to 0, if so the function seeds 
					the RNG itself.

	\return 		A new lattice object on success.
			 		NULL if:
						size < 1
						size memory could not be allocated (out of memory.)

*/
extern ln_lattice ln_lattice_new(int size, int seed);

/**
	Frees an allocated lattice. You should always call
	this when you are done with your lattice.
*/
extern void ln_lattice_free(ln_lattice lattice);

/**
	Gets a value from the lattice.

	It permutes the index with a special permutation table to increase visual 
	quality and makes sure it's within bounds.

	As a result, any integer will produce a value. It will not be a mapping to the
	exact index specified.

	If you want to get the actual value at some index, you'll have to manually 
	access the values-array stored in the lattice.
*/
extern float ln_lattice_value(ln_lattice lattice, int index);

/**
	Represents an interpolation technique for a 2D point.

	The components of p will be normalised to [0.0, 1.0).
*/
typedef float (*ln_interpolator2d)(
	ln_lattice lattice, 
	point3 p);



/**
	Gets a value from the lattice for point p.

	This function treats p as a 2D point and discards Z in the calculations.
	
	\param	p 		A point3 space to calculate a value from.
	\param	interp 	A 2D interpolator function.
	\return			A value interpolated between 4 lattice points.
*/
extern float ln_lattice_2dvalue(
	ln_lattice lattice, 
	point3 p, 
	ln_interpolator2d interp);













