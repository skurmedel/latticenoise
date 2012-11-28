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

/**
	latticenoise.h

	This file contains various methods for generating and rendering procedural 
	textures based on the Lattice Noise method.

	It contains common functions as well as specific implementations such as 
	Perlin Noise and Turbulence.
*/

/**
	Represents a lattice of random values. It contains the actual lattice as well
	as various metadata.

	The lattice is one-dimensional.
*/
struct ln_lattice_s
{
	/* NOTE:	DO NOT TOUCH THESE VALUES AFTER THE LATTICE HAS BEEN CONSTRUCTED.
				IT WILL PROBABLY BLOW UP YOUR PROGRAM. */

	/* The actual values. */
	float *values;
	/* 
		The size of the lattice.
	*/ 
	int size;
	/*
		The value that was used to seed the RNG upon construction of the lattice.
	*/
	int seed;
};

typedef ln_lattice_s *ln_lattice;

// POINT TYPE.

/*
	Represents a three dimensional point in space.
*/
typedef struct point_s
{
	float x, y, z;
} point;

/*
	A simple vector addition.

	Returns:

		(point){ p1.x + p2.x, p1.y + p2.y, p1.z + p2.z }
*/
#define point_add(p1, p2) ((point){p1.x + p2.x, p1.y + p2.y, p1.z + p2.z})

/*
	Prints a point to console, purely for debugging purposes.

	No newline is written.
*/
#define point_print(p1) printf("<%0.5f, %0.5f, %0.5f>", p1.x, p1.y, p1.z)

// LATTICE FUNCTIONS.

/*
	Creates a new lattice.

	size:

		The size of the lattice. It must be >= 1.

	seed:

		A seed value for the RNG used to initialize the lattice.

		It is optional and may be set to 0, if so the function seeds the RNG itself.

	Returns:

		A new lattice object on success.

		NULL if:
			size < 1
			size memory could not be allocated (out of memory.)

*/
extern ln_lattice ln_lattice_new(int size, int seed);

/*
	Frees an allocated lattice. You should always call
	this when you are done with your lattice.
*/
extern void ln_lattice_free(ln_lattice lattice);

/*
	Gets a value from the lattice.

	It permutes the index with a special permutation table to increase visual 
	quality and makes sure it's within bounds.

	As a result, any integer will produce a value. It will not be a mapping to the
	exact index specified.

	If you want to get the actual value at some index, you'll have to manually 
	access the values-array stored in the lattice.
*/
extern float ln_lattice_value(ln_lattice lattice, int index);

extern float ln_lattice_2dvalue(ln_lattice lattice, point p);