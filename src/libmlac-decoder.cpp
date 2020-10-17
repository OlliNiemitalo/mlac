// MLAC decoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)

#include "libmlac-decoder.h"
#include "mlac-core.hpp"

static MLACDecoder decoder;

extern "C" int mlac_decode(const uint8_t *input, int16_t *output, uint8_t *timeStamp, int *numSampleTuplesRead) {
  return decoder.decode(input, output, *timeStamp, *numSampleTuplesRead);
}
