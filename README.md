MLAC, a mostly lossless audio codec
-----------------------------------
Copyright 2020 Olli Niemitalo (o@iki.fi). Licensed under MIT license (see [LICENSE](LICENSE)). There is no warranty.

This is work in progress. The MLAC format is not yet frozen, so audio encoded using one version will probably not be decoded properly using another version.

16-bit stereo audio encoding and decoding works.

The lossless mode gives almost FLAC-like compression for difficult material, and is faster both in encoding and decoding. The packet size is suitable for Bluetooth Low Energy audio applications. The lossy mode could still be improved, and there is no stream handling yet. I suggest to switch to lossy mode for each packet that might otherwise result in an audio buffer underrun.

Prerequisities
--------------

For compiling and running tests that require file input/output, install libsndfile:

    sudo apt-get install -y libsndfile-dev

Compilation
-----------

Compile and run unit tests:

    make all
    ./unittest

See `makefile` for other things you can make. To use the MLAC codec in your own program, either include the C++ core `src/mlac-core.hpp` or, for a C program, make `libmlac-encoder.o` and `libmlac-decoder.o` and use those using C include files `src/libmlac-decoder.h` and `src/libmlac-encoder.h`.
