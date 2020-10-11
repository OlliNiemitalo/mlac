// Molo-transcode
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
//
// Input.wav will be read.
// Output.molo will be written.
// Output.wav will be written.
//
// For Emacs: -*- compile-command: "make -C .. transcode" -*-

#include <atomic>
#include <stdio.h>
#include <sndfile.h>
#include <stdint.h>
#include "molo-core.hpp"
#include <fstream>
#include <iostream>

int strToInt(const char *s) {
  int val = 0;
  while (*s) {
    val *= 10;
    val += *s - '0';
    s++;
  }
  return val;
}

class StereoFIFO {
  std::atomic<int> readPosition;
  std::atomic<int> writePosition;
  int totalNumSampleTuples;
  StereoFIFO(int totalNumSampleTuples) : totalNumSampleTuples(totalNumSampleTuples) {
    writePosition = 0;
    readPosition = 0;
    
  }
};

int main (int argc, char *argv[]) {
  bool info = true;

  int latency_ms = 100;
  int bitrate_kbps = 1500;
    
  if (argc < 3) {
    printf("Usage: %s input.wav output.wav [bitrate_kbps] [latency_ms]\n", argv[0]);
    return 1;
  }
  if (argc >= 4) {
    bitrate_kbps = strToInt(argv[3]);
  }
  if (argc >= 5) {
    latency_ms = strToInt(argv[4]);
  }
  if (info) printf("Latency = %d ms\n", latency_ms);
  if (info) printf("Bitrate = %d kbps\n", bitrate_kbps);
  SF_INFO sfInfo;
  SNDFILE *inputSndFile = sf_open(argv[1], SFM_READ, &sfInfo);
  if (!inputSndFile) {
    printf("Error: could not open %s\n", argv[1]);
    return 1;
  }
  if (info) printf("Sample tuples: %ld (%ld+ min)\n", sfInfo.frames, sfInfo.frames/(44100*60));
  long int totalNumSampleTuples = sfInfo.frames;
  if (info) printf("Sampling frequency: %d Hz\n", sfInfo.samplerate);
  if (info) printf("Number of channels: %d\n", sfInfo.channels);
  if (info) printf("Format code: 0x%x\n", sfInfo.format);
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
  short *inBuf = new short[totalNumSampleTuples*2];
  sf_read_short(inputSndFile, inBuf, totalNumSampleTuples*2);
  sf_close(inputSndFile);
  SNDFILE *outputSndFile = sf_open(argv[2], SFM_WRITE, &sfInfo);
  if (!outputSndFile) {
    printf("Error: could not open %s\n", argv[2]);
    return 1;
  }
  short *outBuf = new short[totalNumSampleTuples*2];
  uint8_t encodeBuf[MOLO_BLOCK_NUM_BYTES];
  MoloEncoder moloEncoder;
  MoloDecoder moloDecoder;

  double requiredCompressionRate = 1411.2/bitrate_kbps;
  int requiredNumSampleTuples = ceil(MOLO_BLOCK_NUM_BYTES/4*requiredCompressionRate);
  if (requiredNumSampleTuples < MOLO_BLOCK_MIN_NUM_SAMPLETUPLES) {
    requiredNumSampleTuples = MOLO_BLOCK_MIN_NUM_SAMPLETUPLES;
  }
  if (info) printf("Required number of sample tuples per packet: %d\n", requiredNumSampleTuples);
  if (requiredNumSampleTuples < MOLO_BLOCK_MAX_NUM_SAMPLETUPLES);
  int numLossyBlocks = 0;
  int numBlocks = 0;
  long int bitDepthAccu = 0;
  int latencyInSampleTuples = latency_ms/1000.0*44100;
  if (info) printf("Latency in sample tuples: %d\n", latencyInSampleTuples);
  int *rateHistory = new int[totalNumSampleTuples + latencyInSampleTuples];
  int j;
  int lowestRate = 1412*(MOLO_BLOCK_NUM_BYTES/4)/MOLO_BLOCK_MAX_NUM_SAMPLETUPLES;
  if (info) printf("Lowest possible rate: %d kbps\n", lowestRate);
  long int rateHistoryAccu = 0;
  for (j = 0; j < latencyInSampleTuples; j++) {
    rateHistory[j] = lowestRate;
    rateHistoryAccu += rateHistory[j];
  }
  long int i;

  for (int i = 0;;i++) {
    if (argv[1][i] == '.' || argv[1][i] == 0) {
      argv[1][i] = 0;
      break;
    }
  }
  char moloFileName[65536];
  sprintf(moloFileName, "%s_%dkbps.molo", argv[2], bitrate_kbps);
  std::ofstream moloFile(moloFileName, std::ios::out | std::ios::binary);
  
  for (i = 0; i < totalNumSampleTuples - MOLO_BLOCK_MAX_NUM_SAMPLETUPLES;) {    
      int numSampleTuplesWritten;
      int numBitsWritten;
      int forkTopNumSampleTuples = MOLO_BLOCK_MAX_NUM_SAMPLETUPLES;
      int forkBottomNumSampleTuples = MOLO_BLOCK_MIN_NUM_SAMPLETUPLES;
      int forkNumSampleTuples = (MOLO_BLOCK_MAX_NUM_SAMPLETUPLES + MOLO_BLOCK_MIN_NUM_SAMPLETUPLES + 1) / 2;
      int bitDepth = moloEncoder.encode((int16_t *)&inBuf[i*2], encodeBuf, 0, numSampleTuplesWritten, numBitsWritten, requiredNumSampleTuples);
      moloFile.write((char *)encodeBuf, MOLO_BLOCK_NUM_BYTES);
      uint8_t compareTimeStamp;
      int compareNumSampleTuples;
      int trueBitDepth = moloDecoder.decode(encodeBuf, &outBuf[i*2], compareTimeStamp, compareNumSampleTuples);
      if (trueBitDepth != 16) {
	if (info) printf("lossy %ld %d %d\n", i, trueBitDepth, numSampleTuplesWritten);
      }
      int advancej = j + numSampleTuplesWritten;
      for (;j < advancej; j++) {
	rateHistory[j] = (int)(1411.2*(MOLO_BLOCK_NUM_BYTES/4)/numSampleTuplesWritten);
	rateHistoryAccu += rateHistory[j];
	rateHistoryAccu -= rateHistory[j-latencyInSampleTuples];	
      }
      bitDepthAccu += numSampleTuplesWritten*trueBitDepth;
      i += numSampleTuplesWritten;
      numBlocks++;
  }
  if (info) printf("Lossy blocks / total blocks: %d/%d = %f\n", numLossyBlocks, numBlocks, numLossyBlocks/(float)numBlocks);
  if (info) printf("Average bit depth: %f\n", (bitDepthAccu*10/i)/10.0);
  sf_write_short(outputSndFile, outBuf, i*2);
  sf_close(outputSndFile);
  return 0;
}
