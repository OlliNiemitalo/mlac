// Molo decoder C wrapper.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)

#include "libmolo-decoder.h"
#include "molo-core.hpp"

static MoloDecoder decoder;

extern "C" int molo_decode(const uint8_t *input, int16_t *output, uint8_t *timeStamp, int *numSampleTuplesRead) {
  return decoder.decode(input, output, *timeStamp, *numSampleTuplesRead);
}
