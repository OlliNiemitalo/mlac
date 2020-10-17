// Calculate MLAC compression statistics of 192 stereo sample blocks
//
// Copyright Olli Niemitalo (o@iki.fi)
//
// For Emacs: -*- compile-command: "make -C .. statistics" -*-

#include <stdio.h>
#include <sndfile.h>
#include <stdint.h>
#include "mlac-core.hpp"

int main (int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s input.wav\n", argv[0]);
    return 1;
  }
  SF_INFO sfInfo;
  SNDFILE *sndFile = sf_open(argv[1], SFM_READ, &sfInfo);
  if (!sndFile) {
    printf("Error: could not open %s\n", argv[1]);
    return 1;
  }
  if (sfInfo.channels != 2) {
    printf("Error: input audio file must have %d channels", 2);
    return 1;
  }
  if (SHRT_MAX != 0x7fff) {
    printf("Error: C short must be 16-bit\n");
    return 1;
  }
  if (INT_MAX != 0x7fffffff) {
    printf("Error: C int must be 32-bit\n");
    return 1;
  }
  short *inBuf = new short[sfInfo.frames*2];
  sf_read_short(sndFile, inBuf, sfInfo.frames*2); // Not buffered
  sf_close(sndFile);
  const int maxNumCompressedBytes = 244;
  uint8_t outBuf[maxNumCompressedBytes];
  MLACEncoder mlacEncoder;
  MLACDecoder mlacDecoder;

  double mean = 0;
  int meanCount = 0;
  int16_t compareBuf[2*MLAC_BLOCK_MAX_NUM_SAMPLETUPLES];
  for (long int i = 0; i < sfInfo.frames - 192; i += 192) {
      int numSampleTuplesWritten;
      int numBitsWritten;
      mlacEncoder.encode((int16_t *)&inBuf[i*2], outBuf, i & 255, numSampleTuplesWritten, numBitsWritten);
      printf("%ld,%d\n",i,numSampleTuplesWritten);
      uint8_t compareTimeStamp;
      int compareNumSampleTuples;
      mlacDecoder.decode(outBuf, compareBuf, compareTimeStamp, compareNumSampleTuples);
      if (numSampleTuplesWritten != compareNumSampleTuples) {
	printf("numSampleTuples DIFFER %d %d\n", numSampleTuplesWritten, compareNumSampleTuples);
      }
      bool passes = true;
      for (int k = 0; k < compareNumSampleTuples*2; k++) {
	if (((int16_t *)inBuf)[i*2 + k] != compareBuf[k]) {
	  passes = false;
	}
      }
      if (!passes) {
	printf("fails\n");
      }
      mean += numSampleTuplesWritten;
      meanCount++;
  }
  printf("mean = %f\n", mean/meanCount);

  delete[] inBuf;
}
