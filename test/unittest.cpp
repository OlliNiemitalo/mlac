// Unit tests for mlac-core.hpp
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
//
// For Emacs: -*- compile-command: "make -C .. unittest" -*-

#include <climits>
#include <cstdint>
#include <stdio.h>
#include <sndfile.h>
#include <math.h>
#include <algorithm>
#include <time.h>

#include "mlac-core.hpp"

// Unit tests, uncomment to enable
#define UNITTEST_BITDEPTH_16
#define UNITTEST_VALUE_TO_EXPGOLOMBLIKE_NUM_BITS_16
#define UNITTEST_TOTAL_EXPGOLOMBLIKE_NUM_BITS_16
#define UNITTEST_BEST_EXPGOLOMBLIKE_PARAMETER_16
#define UNITTEST_EXPGOLOMBLIKE_CODE
#define UNITTEST_BITSTREAMWRITEREAD
#define UNITTEST_BISTREAM_WRITE_READ_EXPGOLOMBLIKE
#define UNITTEST_LOSSLESS_TRANSCODE
#define UNITTEST_BITSTREAM_WRITE_READ_RESIDUAL_EXPGOLOMBLIKE_PARAMETER

// Speed tests, uncomment to enable
const char *transcodeInputFileName = "sounds/Oulu Space Jam Collective - Strike of the Death Anvil (excerpt).flac";
#define SPEEDTEST_LOSSLESS_TRANSCODE

// Debug a bug triggered by some particularly difficult input, uncomment to enable
#define DEBUG_LOSSLESS_TRANSCODE

static void printb(uint32_t val, int numBits) {
  val <<= 32-numBits;
  for (int i = 0; i < numBits; i++) {
    if (val & 0x80000000) {
      printf("1");
    } else {
      printf("0");
    }
    val <<= 1;
  }
}

// This is a function that is not currently needed by codec (so moved out of mlac-core.hpp) but we test it also
static int totalExpGolombLikeNumBits16(int *bitDepthCounts, int expGolombLikeParameter) {
  int numBits = 0;
  for (int i = 0; i < 17-RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
    int bitDepth = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + i;
    if (bitDepth <= expGolombLikeParameter) {
      numBits += bitDepthCounts[i]*(1 + expGolombLikeParameter);
    } else if (bitDepth == 16) {
      numBits += bitDepthCounts[i]*(2*bitDepth - expGolombLikeParameter - 1);
    } else {
      numBits += bitDepthCounts[i]*(2*bitDepth - expGolombLikeParameter);
    }
  }
  return numBits;
}

int bruteForceBestExpGolombLikeParameter16(int *bitDepthCounts, int &bestNumBits) {
  int bestExpGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER;
  bestNumBits = totalExpGolombLikeNumBits16(bitDepthCounts, RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER);
  for (int expGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + 1; expGolombLikeParameter < 15; expGolombLikeParameter++) {
    int numBits = totalExpGolombLikeNumBits16(bitDepthCounts, expGolombLikeParameter);
    if (numBits < bestNumBits) {
      bestExpGolombLikeParameter = expGolombLikeParameter;
      bestNumBits = numBits;
    }
  }
  int rawNumBits = 0;
  for (int i = 0; i < 17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
    rawNumBits += bitDepthCounts[i];
  }
  rawNumBits *= 16;
  if (bestNumBits > rawNumBits) {
    bestExpGolombLikeParameter = 15; // denotes raw 16-bits
    bestNumBits = rawNumBits;
  }
  return bestExpGolombLikeParameter;
}

// ExpGolombLikeBasebitdepth   
// |    7  8  9 10 11 12 13 14 15 bitdepth 
// |    0  1  2  3  4  5  6  7  8 i
//14   15 15 15 15 15 15 15 15/16
//13   14 14 14 14 14 14 14/15 17
//12   13 13 13 13 13 13/14 16 18
//11   12 12 12 12 12/13 15 17 19
//10   11 11 11 11/12 14 16 18 20
// 9   10 10 10/11 13 15 17 19 21
// 8    9  9/10 12 14 16 18 20 22
// 7    8/ 9 11 13 15 17 19 21 23

// ExpGolombLikeBasebitdepth   
// |    7  8  9 10 11 12 13 14 15 bitdepth 
// |    0  1  2  3  4  5  6  7  8 i
//14   15 15 15 15 15 15 15 15/ 1
//13   14 14 14 14 14 14 14/ 1  2
//12   13 13 13 13 13 13/ 1  2  2
//11   12 12 12 12 12/ 1  2  2  2
//10   11 11 11 11/ 1  2  2  2  2
// 9   10 10 10/ 1  2  2  2  2  2
// 8    9  9/ 1  2  2  2  2  2  2
// 7    8/ 1  2  2  2  2  2  2  2

void printPass(bool pass) {
  if (pass) {
    printf("Pass\n\n");
  } else {
    printf("Fail\n\n");
  }
}

timespec timeDiff(timespec t0, timespec t1)
{
  timespec td;
  if (t1.tv_nsec - t0.tv_nsec < 0) {
    td.tv_sec = t1.tv_sec - t0.tv_sec - 1;
    td.tv_nsec = 1000000000 + t1.tv_nsec - t0.tv_nsec;
  } else {
    td.tv_sec = t1.tv_sec - t0.tv_sec;
    td.tv_nsec = t1.tv_nsec - t0.tv_nsec;
  }
  return td;
}

int main() {
  unsigned int randomSeed = 1522866229;
  printf("randomSeed=%d\n", randomSeed);
  srand(randomSeed);
  bool pass;
#ifdef UNITTEST_BITDEPTH_16
  printf("UNITTEST_BITDEPTH_16: bitDepth16\n");
  pass = true;
  for (int minBitDepth = 1; minBitDepth <= 16; minBitDepth++) {
    for (int trueBitDepth = 1; trueBitDepth <= 16; trueBitDepth++) {
      for (int i = 1 << (trueBitDepth - 1) >> 1; i < 1 << trueBitDepth >> 1; i++) {
        for (int negative = 0; negative <= 1; negative++) {
          int j = (negative)? -i - 1 : i;
          int bitDepth = bitDepth16((int16_t) j, minBitDepth);
          int compareBitDepth = (trueBitDepth < minBitDepth) ? minBitDepth : trueBitDepth;
          if (bitDepth != compareBitDepth) {
            printf("Error: minBitDepth=%d, j=", minBitDepth);
            printb((uint32_t) j, 16);
            printf("b (%d), bitDepth=%d, compareBitDepth=%d\n", j, bitDepth, compareBitDepth);
            pass = false;
          }
        }
      }
    }
  }
  printPass(pass);           
#endif
#ifdef UNITTEST_VALUE_TO_EXPGOLOMBLIKE_NUM_BITS_16
  printf("UNITTEST_VALUE_TO_EXPGOLOMBLIKE_NUM_BITS_16: valueToExpGolombLikeNumBits16\n");
  pass = true;
  for (int expGolombLikeParameter = 1; expGolombLikeParameter <= 15; expGolombLikeParameter++) {
    for (int i = -0x8000; i < 0x8000; i++) {
      int expGolombLikeNumBits = valueToExpGolombLikeNumBits16((int16_t) i, expGolombLikeParameter);
      int bitDepth = bitDepth16((int16_t) i, expGolombLikeParameter);
      int trueExpGolombLikeNumBits = 0;
      if (bitDepth <= expGolombLikeParameter) {
        trueExpGolombLikeNumBits += 1; // Exponent
        trueExpGolombLikeNumBits += expGolombLikeParameter; // Significand
      } else {
        trueExpGolombLikeNumBits += 1 + bitDepth - expGolombLikeParameter; // Exponent
        trueExpGolombLikeNumBits += bitDepth - 1; // Significand
      }
      if (bitDepth == 16) {
        trueExpGolombLikeNumBits--; // Exponent
      }
      if (expGolombLikeNumBits != trueExpGolombLikeNumBits) {
        printf("Error: expGolombLikeParameter = %d, i=", expGolombLikeParameter);
        printb((uint32_t) i, 16);
        printf("b (%d), expGolombLikeNumBits=%d, trueExpGolombLikeNumBits=%d\n", i, expGolombLikeNumBits, trueExpGolombLikeNumBits);
        pass = false;
      }
    }
  }
  printPass(pass);           
#endif
#ifdef UNITTEST_TOTALEXOGOLOMBLIKENUMBITS16
  printf("UNITTEST_TOTAL_EXPGOLOMBLIKE_NUM_BITS_16: totalExpGolombLikeNumBits16\n");
  pass = true;
  for (int i = 0; i < 100000; i++) {
    for (int expGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; expGolombLikeParameter <= 15; expGolombLikeParameter++) {
      int trueTotalExpGolombLikeNumBits = 0;
      int counts[17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER];
      for (int i = 0; i < 17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
        counts[i] = 0;
      }
      for (int i = 0; i < BLOCK_MIN_NUM_SAMPLETUPLES; i++) {
        int bitDepth = (rand()%(17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER)) + RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; // RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER..16 inclusive
        trueTotalExpGolombLikeNumBits += bitDepthToExpGolombLikeNumBits16(bitDepth, expGolombLikeParameter);
        counts[bitDepth - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]++;
      }
      int totalExpGolombLikeNumBits = totalExpGolombLikeNumBits16(counts, expGolombLikeParameter);
      if (totalExpGolombLikeNumBits != trueTotalExpGolombLikeNumBits) {        
        printf("Error: expGolombLikeParameter=%d, totalExpGolombLikeNumBits=%d, trueTotalExpGolombLikeNumBits=%d\n", expGolombLikeParameter, totalExpGolombLikeNumBits, trueTotalExpGolombLikeNumBits);
        printf("\n");
        for (int i = 0; i < 17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
          printf("%d ", counts[i]);          
        }
        printf("\n");
        pass = false;
      }
    }
  }
  printPass(pass);
#endif  
#ifdef UNITTEST_BEST_EXPGOLOMBLIKE_PARAMETER_16
  printf("UNITTEST_BEST_EXPGOLOMBLIKE_PARAMETER_16: bestExpGolombLikeParameter16\n");
  pass = true;
  int bestExpGolombLikeParameterCounts[16 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + 1]; 
  for (int i = 0; i < 16 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
    bestExpGolombLikeParameterCounts[i] = 0;
  }   
  for (int i = 0; i < 10000; i++) {
    for (int minBitDepth = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; minBitDepth < 17; minBitDepth++) {
      for (int maxBitDepth = minBitDepth; maxBitDepth < 17; maxBitDepth++) {
        int counts[17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER];
        for (int i = 0; i < 17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
          counts[i] = 0;
        }
        for (int i = 0; i < BLOCK_MIN_NUM_SAMPLETUPLES; i++) {
          int bitDepth = minBitDepth + rand()%(1 + maxBitDepth - minBitDepth);
          counts[minBitDepth - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]++;
        }
        int bruteForceBestNumBits;
        int bruteForceBestExpGolombLikeParameter = bruteForceBestExpGolombLikeParameter16(counts, bruteForceBestNumBits);
        int bestNumBits;
        int bestExpGolombLikeParameter = bestExpGolombLikeParameter16(counts, bestNumBits, BLOCK_MIN_NUM_SAMPLETUPLES);
        bestExpGolombLikeParameterCounts[bestExpGolombLikeParameter - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]++;
        if (bestExpGolombLikeParameter - bruteForceBestExpGolombLikeParameter || bestNumBits - bruteForceBestNumBits) {
          printf("Error:\n");
          printf("Brute force: bestExpGolombLikeParameter=%d, bestNumBits=%d\n", bruteForceBestExpGolombLikeParameter, bruteForceBestNumBits);
          printf("Current imp: bestExpGolombLikeParameter=%d, bestNumBits=%d\nDiff: %d %d\n", bestExpGolombLikeParameter, bestNumBits, bestExpGolombLikeParameter - bruteForceBestExpGolombLikeParameter, bestNumBits - bruteForceBestNumBits);
          pass = false;
        }
      }
    }
  }
  printPass(pass);
#endif  
#ifdef UNITTEST_EXPGOLOMBLIKE_CODE
  printf("UNITTEST_EXPGOLOMBLIKE_CODE: expGolombLikeEncode16, expGolombLikeDecode16\n");
  pass = true;
  for (int expGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; expGolombLikeParameter <= 15; expGolombLikeParameter++) {
    for (int i = -0x8000; i < 0x8000; i++) {
      int numBits;
      uint32_t encoded = expGolombLikeEncode16((int16_t) i, expGolombLikeParameter, 16, numBits);
      int trueNumBits = valueToExpGolombLikeNumBits16((int16_t) i, expGolombLikeParameter);
      encoded <<= 32 - numBits;
      uint32_t padding = rand() % (1 << (32 - numBits));
      encoded |= padding;
      int bitDepth;
      int16_t value16 = expGolombLikeDecode16(encoded, expGolombLikeParameter, 16, bitDepth);
      if (value16 != i || numBits != trueNumBits) {
        printf("Error expGolombLikeParameter=%d, numBits=%d, trueNumBits=%d, padding=%d, i=%d, value16=%d, encoded=%d\n", expGolombLikeParameter, numBits, trueNumBits, padding, i, value16, encoded);
        pass = false;
      }
    }
  }
  printPass(pass);
#endif
#ifdef UNITTEST_BITSTREAMWRITEREAD
  printf("UNITTEST_BITSTREAMWRITEREAD: BitStreamWriter.write, BitStreamReader.read\n");
  pass = true;
  for (int bufLen = 1; bufLen < 100; bufLen++) {
    uint8_t buf[bufLen];
    uint32_t values[bufLen*8];
    int numBits[bufLen*8];
    for (int k = 0; k < 100; k++) {
      BitStreamWriter writer(buf);
      int totalNumBits = 0;
      int i;
      for (i = 0; i < bufLen*8; i++) {
        numBits[i] = (rand()%25) + 1;
        values[i] = rand()%(1 << numBits[i]);
        if (totalNumBits + numBits[i] > bufLen*8) {
          break;
        }
        totalNumBits += numBits[i];
        writer.write(values[i], numBits[i]);
      }
      BitStreamReader reader(buf, bufLen);
      for (int j = 0; j < i; j++) {
        uint32_t val;
        reader.read(val, numBits[j]);
        if (val != values[j]) {
          printf("Error: numBits=%d, wrote %d, read %d\n", numBits[j], values[j], val);
          pass = false;
        }
      }
    }
  }
  printPass(pass);
#endif
#ifdef UNITTEST_BITSTREAM_WRITE_READ_RESIDUAL_EXPGOLOMBLIKE_PARAMETER
  printf("UNITTEST_BITSTREAM_WRITE_READ_RESIDUAL_EXPGOLOMBLIKE_PARAMETER: BitStreamWriter.writeResidualExpGolombLikeParameter, BitStreamReader.readResidualExpGolombLikeParameter\n");
  pass = true;
  for (int bufLen = 1; bufLen < 100; bufLen++) {
    uint8_t buf[bufLen];
    int values[bufLen*8];
    BitStreamWriter writer(buf);
    int totalNumBits = 0;
    int k;
    for (k = 0; k < bufLen*8; k++) {
      values[k] = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + rand()%(15 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER);
      if (writer.numBitsWritten + residualExpGolombLikeParameterEncodingNumBits[values[k] - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER] > bufLen*8) {
        break;
      }
      writer.writeResidualExpGolombLikeParameter(values[k]);      
    }
    BitStreamReader reader(buf, bufLen);
    for (int i = 0; i < k; i++) {
      int read;
      reader.readResidualExpGolombLikeParameter(read);
      if (read != values[i]) {
        printf("i=%d, value=%d, read=%d\n", i, values[i], read);
        pass = false;
      }
    }
  }
  printPass(pass);
#endif
#ifdef UNITTEST_BISTREAM_WRITE_READ_EXPGOLOMBLIKE
  printf("UNITTEST_BISTREAM_WRITE_READ_EXPGOLOMBLIKE: BitStreamReader.writeExpGolombLike, BitStreamReader.readExpGolombLike\n");
  pass = true;
  for (int bufLen = 1; bufLen < 100; bufLen++) {
    uint8_t buf[bufLen];
    int16_t values[bufLen*8];
    int numBits[bufLen*8];
    int expGolombLikeParameters[bufLen*8];
    for (int k = 0; k < 1000; k++) {
      BitStreamWriter writer(buf);
      int totalNumBits = 0;
      int i;
      for (i = 0; i < bufLen*8; i++) {
        expGolombLikeParameters[i] = (rand() % 10) + 6; // 1..15
        values[i] = (int16_t) rand();
        values[i] >>= (rand() % 16);
        numBits[i] = valueToExpGolombLikeNumBits16(values[i], expGolombLikeParameters[i]);
        if (totalNumBits + numBits[i] > bufLen*8) {
          break;
        }
        totalNumBits += numBits[i];
        writer.writeExpGolombLike(values[i], expGolombLikeParameters[i], 16);              
      }
      BitStreamReader reader(buf, bufLen);
      for (int j = 0; j < i; j++) {
        int16_t val;
        reader.readExpGolombLike(val, expGolombLikeParameters[j], 16);
        if (val != values[j]) {
          printf("Error: i=%d, j=%d, expGolombLikeParameter=%d, numBits=%d, wrote %d, read %d\n", i, j, expGolombLikeParameters[j], numBits[j], values[j], val);
        }
      }
    }
  }
  printPass(pass);
#endif
#ifdef UNITTEST_LOSSLESS_TRANSCODE
  printf("UNITTEST_LOSSLESS_TRANSCODE: MLACEncoder.encode, MLACDecoder.decode\n");
  pass = true;
  MLACEncoder encoder;
  MLACDecoder decoder;
  double audioBuf[BLOCK_MAX_NUM_SAMPLETUPLES*2];
  int16_t sourceBuf[BLOCK_MAX_NUM_SAMPLETUPLES*2];
  for (int k = 0; k < 10000; k++) {
    for (int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
      audioBuf[i*2] = 0;
      audioBuf[i*2 + 1] = 0;
    }
    // Stereo impulses
    int numImpulses = rand()%10;
    for (int j = 0; j < numImpulses; j++) {
      int pos = rand()%BLOCK_MAX_NUM_SAMPLETUPLES;
      audioBuf[pos*2] += rand()/(RAND_MAX*2.0) - 0.5;
      audioBuf[pos*2 + 1] += rand()/(RAND_MAX*2.0) - 0.5;
    }
    // Sine waves
    int numSineWaves = rand()%10;
    for (int j = 0; j < numSineWaves; j++) {
      double phaseL = rand()/(double)RAND_MAX*M_PI;
      double phaseR = rand()/(double)RAND_MAX*M_PI;
      double ampL = rand()/(double)RAND_MAX;
      double ampR = rand()/(double)RAND_MAX;
      double w = rand()/(double)RAND_MAX*M_PI;
      //      printf("phaseL=%f, phaseR=%f, ampL=%f, ampR=%f, w=%f\n", phaseL, phaseR, ampL, ampR, w);
      for (int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
        audioBuf[i*2] += sin(phaseL + i*w)*ampL;
        audioBuf[i*2 + 1] += sin(phaseR + i*w)*ampR;
      }
    }
    double peak = 0;
    for (int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
      if (peak < audioBuf[i*2]) {
        peak = audioBuf[i*2];
      }
      if (peak < audioBuf[i*2 + 1]) {
        peak = audioBuf[i*2 + 1];
      }
    }
    double normFactor = pow(2, (rand()%1600)/100.0); // Will clip sometimes
    for (int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES*2; i++) {
      audioBuf[i] *= normFactor;
      if (audioBuf[i] > 0x7fff) {
        audioBuf[i] = 0x7fff;
      }
      if (audioBuf[i] < -0x8000) {
        audioBuf[i] = -0x8000;
      }
      sourceBuf[i] = audioBuf[i];
    }
    uint8_t dataBuf[BLOCK_NUM_BYTES];
    uint8_t timeStamp = (int8_t) rand();
    int numSampleTuplesWritten;
    int numBitsWritten;
    encoder.encode(sourceBuf, dataBuf, timeStamp, numSampleTuplesWritten, numBitsWritten);
    int16_t destBuf[BLOCK_MAX_NUM_SAMPLETUPLES*2];
    int numSampleTuplesRead;
    decoder.decode(dataBuf, destBuf, timeStamp, numSampleTuplesRead);
    if (numSampleTuplesRead != numSampleTuplesWritten) {
      printf("numSampleTuplesRead=%d != numSampleTuplesWritten=%d\n", numSampleTuplesRead, numSampleTuplesWritten);
      pass = false;
    }
    for (int i = 0; i < numSampleTuplesWritten*2; i++) {
      if (destBuf[i] != sourceBuf[i]) {
        printf("Error: i=%d, source: %d, dest: %d, numSampleTuplesRead=%d\n", i, sourceBuf[i], destBuf[i], numSampleTuplesRead);
        pass = false;
      }
    }
    if (!pass) {
      printf("i, sourceBuf, destBuf\n");
      for(int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
        printf("%d,%d,", sourceBuf[i*2], sourceBuf[i*2+1]);
      }
      printf("\n");
      return 1;
    }
  }
  printPass(pass);
#endif
#ifdef SPEEDTEST_LOSSLESS_TRANSCODE
  printf("SPEEDTEST_LOSSLESS_TRANSCODE: Test speed of encoder and decoder on CD audio.\n");
  pass = true;
  SF_INFO sfInfo;
  SNDFILE *inputSndFile = sf_open(transcodeInputFileName, SFM_READ, &sfInfo);
  if (inputSndFile == NULL) {
    printf("Transcode input file: %s\nLibsndfile error: %s\n", transcodeInputFileName, sf_strerror(inputSndFile));
    pass = false;    
  } else {
    long int totalNumSampleTuples = sfInfo.frames;
    if (SHRT_MAX != 0x7fff) {
      printf("Error: C short must be 16-bit\n");
      return 1;
    }
    if (INT_MAX != 0x7fffffff) {
      printf("Error: C int must be 32-bit\n");
      return 1;
    }
    short *audioBuf = new short[totalNumSampleTuples*2];
    uint8_t *codedBuf = new uint8_t[(totalNumSampleTuples/BLOCK_MIN_NUM_SAMPLETUPLES)*BLOCK_NUM_BYTES];
    short *audioBuf2 = new short[totalNumSampleTuples*2];
    sf_read_short(inputSndFile, audioBuf, totalNumSampleTuples*2);
    sf_close(inputSndFile);

    timespec before, after, difference;
    MLACEncoder encoder;
    MLACDecoder decoder;
    int audioBufPos = 0;
    int codedBufPos = 0;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &before);
    for (;audioBufPos <= totalNumSampleTuples - BLOCK_MAX_NUM_SAMPLETUPLES;) {
      int numSampleTuplesWritten;
      int numBitsWritten;
      encoder.encode(&audioBuf[audioBufPos*2], &codedBuf[codedBufPos], 0, numSampleTuplesWritten, numBitsWritten);
      audioBufPos += numSampleTuplesWritten;
      codedBufPos += BLOCK_NUM_BYTES;
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &after);  
    difference = timeDiff(before, after);
    double differenceSeconds = (difference.tv_sec*1000000000.0d + difference.tv_nsec)/1000000000.0d;
    printf("Encoding: %f seconds, equivalent to %f CPU load\n", differenceSeconds, differenceSeconds/(audioBufPos/44100.0));
    int maxCodedBufPos = codedBufPos;
    codedBufPos = 0;
    audioBufPos = 0;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &before);
    for (;codedBufPos < maxCodedBufPos;) {
      int numSampleTuplesRead;
      uint8_t timeStamp;
      decoder.decode(&codedBuf[codedBufPos], &audioBuf2[audioBufPos*2], timeStamp, numSampleTuplesRead);
      //    printf("numSampleTuplesRead = %d\n", numSampleTuplesRead);
      audioBufPos += numSampleTuplesRead;
      codedBufPos += BLOCK_NUM_BYTES;
    }         
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &after);  
    difference = timeDiff(before, after);
    differenceSeconds = (difference.tv_sec*1000000000.0d + difference.tv_nsec)/1000000000.0d;
    printf("Decoding: %f seconds, equivalent to %f CPU load\n", differenceSeconds, differenceSeconds/(audioBufPos/44100.0));
    int maxAudioBufPos = audioBufPos;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &before);
    for (audioBufPos = 0; audioBufPos < maxAudioBufPos; audioBufPos++) {
      if (audioBuf[audioBufPos*2] != audioBuf2[audioBufPos*2]) {
	//      printf("audioBufpos = %d (L)\n", audioBufPos);     
	pass = false;
	break;
      }
      if (audioBuf[audioBufPos*2 + 1] != audioBuf2[audioBufPos*2 + 1]) {
	//      printf("audioBufpos = %d (R)\n", audioBufPos);     
	pass = false;
	break;
      }    
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &after);
    difference = timeDiff(before, after);
    differenceSeconds = (difference.tv_sec*1000000000.0d + difference.tv_nsec)/1000000000.0d;
    printf("Verifying: %f seconds, equivalent to %f CPU load\n", differenceSeconds, differenceSeconds/(audioBufPos/44100));
    delete[] audioBuf;
    delete[] codedBuf;
    delete[] audioBuf2;
  }
  printPass(pass);

#endif
#ifdef DEBUG_TRANSCODE
  printf("DEBUG_TRANSCODE: MLACEncoder.encode, MLACDecoder.decode (a particularly difficult case)");
  pass = true;
  int16_t sourceBuf[BLOCK_MAX_NUM_SAMPLETUPLES*2] = {173,7858,145,4119,72,-902,-23,-5643,-112,-8626,-166,-8922,-168,-6438,-117,-1948,-30,3147,65,7263,141,9116,173,8129,151,4610,82,-345,-12,-5193,-103,-8424,-162,-9029,-170,-6822,-125,-2489,-41,2618,55,6911,135,9050,172,8370,156,5083,91,212,-2,-4725,-95,-8190,-158,-9104,-172,-7181,-132,-3021,-51,2079,45,6532,128,8950,171,8580,160,5537,100,769,8,-4238,-86,-7926,-153,-9144,-173,-7513,-139,-3542,-61,1532,35,6129,121,8817,169,8758,164,5971,108,1323,18,-3736,-76,-7632,-148,-9150,-173,-7818,-145,-4050,-71,979,25,5704,113,8652,166,8904,167,6382,116,1872,29,-3220,-67,-7310,-142,-9123,-173,-8093,-150,-4542,-80,423,14,5257,105,8454,163,9016,170,6770,124,2414,39,-2693,-57,-6961,-136,-9061,-173,-8338,-155,-5018,-89,-134,3,4791,96,8224,159,9095,172,7132,131,2948,49,-2155,-47,-6587,-129,-8966,-171,-8553,-160,-5475,-98,-691,-6,4307,87,7965,154,9140,173,7469,138,3470,59,-1609,-36,-6187,-122,-8838,-169,-8735,-164,-5911,-107,-1246,-17,3807,78,7675,149,9152,173,7777,144,3979,69,-1057,-26,-5765,-114,-8677,-166,-8885,-167,-6326,-115,-1796,-27,3293,68,7357,143,9129,173,8056,150,4474,79,-501,-15,-5321,-106,-8483,-163,-9003,-169,-6717,-123,-2339,-38,2767,58,7012};  
  int minPeak = 0;
  int maxPeak = 0;
  for (int i = 0; i < BLOCK_MAX_NUM_SAMPLETUPLES*2; i++) {
    if (minPeak > sourceBuf[i]) {
      minPeak = sourceBuf[i];
    }
    if (maxPeak < sourceBuf[i]) {
      maxPeak = sourceBuf[i];
    }
  }
  printf("Range = %d to %d\n", minPeak, maxPeak);
  MLACEncoder encoder;
  MLACDecoder decoder;
  uint8_t dataBuf[BLOCK_NUM_BYTES];
  uint8_t timeStamp = (int8_t) rand();
  int numSampleTuplesWritten;
  int numBitsWritten;
  encoder.encode(sourceBuf, dataBuf, timeStamp, numSampleTuplesWritten, numBitsWritten);
  int16_t destBuf[BLOCK_MAX_NUM_SAMPLETUPLES*2];
  int numSampleTuplesRead;
  decoder.decode(dataBuf, destBuf, timeStamp, numSampleTuplesRead);
  for (int i = 0; i < numSampleTuplesWritten*2; i++) {
    if (destBuf[i] != sourceBuf[i]) {
      printf("Error: i=%d, source: %d, dest: %d, numSampleTuplesRead=%d\n", i, sourceBuf[i], destBuf[i], numSampleTuplesRead);
      pass = false;
    }
  }
  printPass(pass);
#endif
  return 0;
}
