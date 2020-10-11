// Molo encoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)

#include "libmolo-encoder.h"
#include "molo-core.hpp"

static MoloEncoder encoder;

extern "C" int molo_encode(const int16_t *input, uint8_t *output, uint8_t timeStamp, int *numSampleTuplesWritten, int *numBitsWritten, int minNumSampleTuples) {
  return encoder.encode(input, output, timeStamp, *numSampleTuplesWritten, *numBitsWritten, minNumSampleTuples);
}
