// Calculate amplitude statistics of an audio file
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
//
// For Emacs: -*- compile-command: "make -C .. ampstatistics" -*-

#include <limits.h>
#include <stdio.h>
#include <sndfile.h>

const int NUM_CHANNELS = 2;
const int NUM_CATEGORIES = 17;
const int BLOCK_SIZE = 100;

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
  printf("Input sample frames: %ld\n", sfInfo.frames);
  printf("Input sampling frequency: %d\n", sfInfo.samplerate);
  printf("Input channels: %d\n", sfInfo.channels);
  printf("Input format: 0x%x\n", sfInfo.format);
  if (sfInfo.channels != NUM_CHANNELS) {
    printf("Error: input audio file must have %d channels", NUM_CHANNELS);
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
  short *inBuf = new short[sfInfo.frames*NUM_CHANNELS];
  sf_read_short(sndFile, inBuf, sfInfo.frames*NUM_CHANNELS); // Not buffered
  sf_close(sndFile);
  //  int *outBuf = new int[sfInfo.frames*NUM_CHANNELS];

  // process sfInfo.frames sample frames from inBuf to outBuf
  short lastValue = 0;
  int histogram[256*NUM_CATEGORIES];
  for (int i = 0; i < 256*NUM_CATEGORIES; i++) {
    histogram[i] = 0;
  }
  for (int i = 0; i < sfInfo.frames/BLOCK_SIZE; i++) {
    int minBitDepthBin = NUM_CATEGORIES - 1;    
    for (int j = 0; j < BLOCK_SIZE; j++) {
      short inSample = inBuf[i*BLOCK_SIZE*NUM_CHANNELS + j*NUM_CHANNELS]; // Left channel only
      short delta = inSample-lastValue;
      inBuf[i*BLOCK_SIZE*NUM_CHANNELS + j*NUM_CHANNELS] = delta;
      lastValue = inSample;
      int intDelta = delta;
      int bitDepthBin;
      for (bitDepthBin = 1; bitDepthBin < NUM_CATEGORIES; bitDepthBin++) { // Calculate bit depth (0 = 16 bit)
	short shifted = delta << bitDepthBin;
	if (shifted != intDelta << bitDepthBin) {
	  bitDepthBin--;
	  break;
	}
      }
      if (bitDepthBin < minBitDepthBin) {
	minBitDepthBin = bitDepthBin;
      }
    }
    for (int j = 0; j < BLOCK_SIZE; j++) {
      int temp = inBuf[i*BLOCK_SIZE*NUM_CHANNELS + j*NUM_CHANNELS];
      temp <<= minBitDepthBin;
      temp >>= 8;
      char msByte = temp;
      histogram[(unsigned char) msByte + minBitDepthBin*256]++;
    }
  }
  for (int j = 0; j < NUM_CATEGORIES; j++) {
    printf(",%d", j);
  }
  printf("\n");
  for (int i = 128; i < 256; i++) {
    printf("%d", (char)i);
    for (int j = 0; j < NUM_CATEGORIES; j++) {
      printf(",%d", histogram[i+(NUM_CATEGORIES - 1 - j)*256]/*/(float)(sfInfo.frames)*/);
    }
    printf("\n");
  }
  for (int i = 0; i < 128; i++) {
    printf("%d", (char)i);
    for (int j = 0; j < NUM_CATEGORIES; j++) {
      printf(",%d", histogram[i+(NUM_CATEGORIES - 1 - j)*256]/*/(float)(sfInfo.frames)*/);
    }
    printf("\n");
  }

  delete[] inBuf;
}
