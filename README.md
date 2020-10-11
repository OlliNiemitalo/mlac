Molo, a fast and mostly lossless audio codec.
---------------------------------------------
Copyright 2020 Olli Niemitalo (o@iki.fi). Licensed under Creative Commons Attribution 4.0 International. See License. There is no warranty.

This is work in progress.

16-bit stereo audio encoding and decoding works.

The lossless mode gives almost FLAC-like compression for difficult material. The lossy mode could still be improved, and there is no stream handling yet.

Prerequisities
--------------

For compiling and running tests that require file input/output, install libsndfile:

sudo apt-get install -y libsndfile-dev

For compiling and running tests that require sound output, install libsoundio:

sudo apt-get install -y libsoundio-dev

Compilation
-----------
make unittest

./unittest
