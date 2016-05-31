latticenoise
============

A small C library and executable for generating procedural textures.

Current Status
--------------

There are functions for creating and destroying a lattice, and retrieving raw values
from it in 1D, 2D and 3D.

Currently has support for:

    - General perlin noise lookups in 1D and 2D.
    - Fractal sum methods for 1D and 2D.
    - Catmull-Rom or Hermite interpolation (through compiler define atm.)
    - Custom random number generators.
    - A simple standalone program for generating noise images.

About the code
-------------

The code is standard complaint C99 code.

Documentation
-------------

Simple 1D noise
'''''''''''''''
```
// Use a size 64 lattice for noise lookups with default PRNG.
ln_lattice lattice = ln_lattice_new(1, 64, NULL);
// Sample the middle of the lattice with cubic interpolation.
float v = ln_lattice_noise1d(lattice, 0.5f * 64.0f);
// Cleanup. 
ln_lattice_free(lattice);
```

2D Fractal Noise
''''''''''''''''
```
ln_lattice lattice = ln_lattice_new(2, 64, NULL);

ln_fsum_options fsum_opts = ln_default_fsum_options();

/*
    Calculates a fractal noise sum and normalizes it with the analytically derived
    maximum value.
    
    This way v will always be between [0.0, 1.0].
*/
float norm = 1.0f / ln_fsum_max_value(&fsum_opts);
float v = ln_lattice_fsum2d(lattice, x, y, &fsum_opts) * norm;

ln_lattice_free(lattice);
```

An example of some "fbm" type noise generated with the default settings for
ln_lattice_fsum2d.

![a pseudorandom cloud texture generated with the library](fsum_example.png)

License
-------

The main library is under a BSD License. For more information, check each 
individual file.

stbi_image.h and parg is used by the mknoise executable. They are not used by 
latticenoise.h and latticenoise.c.
