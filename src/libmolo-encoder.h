// Header file for molo encoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
//
// Include this in a C program that uses libmolo-encoder.

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
  
  // molo encode
  // Arguments:
  //   input = pointer to beggining of interleaved stereo 16-bit audio that must contain at least MOLO_BLOCK_MAX_NUM_SAMPLETUPLES stereo samples.
  //   output = pointer to beginning of a block of encoded audio to be written. Will write MOLO_BLOCK_NUM_BYTES bytes.
  //   timeStamp = time stamp to be written, not yet implemented. NOTE: TIME STAMPS ARE NOT YET FUNCTIONAL AND ARE INSTEAD USED FOR STORING NUMBER OF SAMPLE TUPLES
  // Returns:
  //   numSampleTuplesWritten = number of stereo sample pairs encoded
  //   numBitsWritten = number of bits written (if less than MOLO_BLOCK_NUM_BYTES*8, then there is room for auxiliary data after encoded audio)
  //   minNumSampleTuples = minimum number of stero samples that must fit to packet, range: MOLO_BLOCK_MIN_NUM_SAMPLETUPLES inclusive to BLOCK_MAX_NUM_SAMPLETUPLES inclusive.
  //                        this setting can force lossy compression.
  //   Return value = Effective resolution of audio in bits, 16 for lossless compression, less for lossy compression
  extern int molo_encode(const int16_t *input, uint8_t *output, uint8_t timeStamp, int *numSampleTuplesWritten, int *numBitsWritten, int minNumSampleTuples);
  
#ifdef __cplusplus
}
#endif
