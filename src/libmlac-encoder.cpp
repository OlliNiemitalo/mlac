// MLAC encoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)

#include "libmlac-encoder.h"
#include "mlac-core.hpp"

static MLACEncoder encoder;

extern "C" int mlac_encode(const int16_t *input, uint8_t *output, uint8_t timeStamp, int *numSampleTuplesWritten, int *numBitsWritten, int minNumSampleTuples) {
  return encoder.encode(input, output, timeStamp, *numSampleTuplesWritten, *numBitsWritten, minNumSampleTuples);
}
