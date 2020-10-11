// Header file for molo decoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
//
// Include this in a C program that uses libmolo-decoder.

#pragma once

#include "molo-constants.h"
#ifdef __cplusplus
#include <cstdint>
#else 
#include <stdint.h>
#endif 

#ifdef __cplusplus

extern "C" {
#endif 

  // molo decode
  // Arguments:
  //   input = pointer to beginning of a block of MOLO_BLOCK_NUM_BYTES encoded audio.
  //   output = pointer to beggining of interleaved stereo 16-bit audio that must have room for at least MOLO_BLOCK_MAX_NUM_SAMPLETUPLES stereo samples to be written.
  // Returns:
  //   timeStamp = time stamp read, not yet implemented. NOTE: TIME STAMPS ARE NOT YET FUNCTIONAL AND ARE INSTEAD USED FOR STORING NUMBER OF SAMPLE TUPLES
  //   numSampleTuplesRead = number of stereo samples read
  //   Return value = Effective resolution of audio in bits, 16 for lossless compression, less for lossy compression
  extern int molo_decode(const uint8_t *input, int16_t *output, uint8_t *timeStamp, int *numSampleTuplesRead);

#ifdef __cplusplus
}
#endif

