// MLAC codec core header-only library.
//
// Copyright 2020 Olli Niemitalo (o@iki.fi)
// 
// You can include this C++ header file in your program, or you can include
// the encoder and decoder C wrapper header files and compile and link the lib.
//
// For Emacs: -*- compile-command: "make -C .. unittest" -*-

#pragma once

#include <limits.h>
#include <cstdint>
#include <math.h>
#include <bit>
#include "mlac-constants.h"

// Constants that are used in both the encoder and the decoder. Changing these will redefine the compression format. Some come from mlac-constants.h.
const int BLOCK_NUM_BYTES = MLAC_BLOCK_NUM_BYTES; // Number of bytes per block of compressed data
const int BLOCK_MAX_NUM_SAMPLETUPLES = MLAC_BLOCK_MAX_NUM_SAMPLETUPLES; // Maximum number of sample tuples
const int BLOCK_MIN_NUM_SAMPLETUPLES = MLAC_BLOCK_MIN_NUM_SAMPLETUPLES; // Minimum number of sample tuples
const int NUM_LP_COEFS = 2; // 
const int RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER = 7;
const int NUM_ITERATIONS = 1;
const int COEF_DIVISOR = 16;
const int COEF_SHIFT = 4;
const int C1_BIAS = 4;
const int C2_BIAS = -8;
const int D0_BIAS = 8;
const int C1_EXPGOLOMBLIKE_PARAMETER = 3;
const int C2_EXPGOLOMBLIKE_PARAMETER = 3;
const int D0_EXPGOLOMBLIKE_PARAMETER = 3;
// *** Exp-Golomb-like code parameter value 3 enables integer coefficient values in range -4096..4095 with the current implementation of reader and writer.
const int C1_MIN = -4096;
const int C1_MAX = 4095;
const int C2_MIN = -4096;
const int C2_MAX = 4095;
const int D0_MIN = -4096;
const int D0_MAX = 4095;

// Channel modes
// Left channel is independently coded. Right channel is coded as dependent on the left channel
const int CHMODE_INDEPENDENT_AND_DEPENDENT = 0;
// The left and right channel are coded independently. Lossy coding of the MSBs only or PCM coding if all bits are included,
const int CHMODE_MSB = 1;

// Base bit depths: Exp-Golomb-like code in reverse order:
//const int bitDepths = {15, 14, 13, 12, 11, 10, 9, 8, 7}
const int residualExpGolombLikeParameterEncodingNumBits[9] = {8, 8, 7, 6, 5, 4, 3, 2, 1};
const int residualExpGolombLikeParameterEncodings[9] = {0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}; // 00000000, 00000001, 0000001, 000001, 00001, 0001, 001, 01, 1

const int chModeMSBNumSampleTuples[17] = {0, 0, 0, 0, 0, 0, 0, 0, (BLOCK_NUM_BYTES*8-8-2-4)/(2*8), (BLOCK_NUM_BYTES*8-8-2-4)/(2*9), (BLOCK_NUM_BYTES*8-8-2-4)/(2*10), (BLOCK_NUM_BYTES*8-8-2-4)/(2*11), (BLOCK_NUM_BYTES*8-8-2-4)/(2*12), (BLOCK_NUM_BYTES*8-8-2-4)/(2*13), (BLOCK_NUM_BYTES*8-8-2-4)/(2*14), (BLOCK_NUM_BYTES*8-8-2-4)/(2*15),(BLOCK_NUM_BYTES*8-8-2-4)/(2*16)};
const int TRUE_BITDEPTH_BIAS = 1;

const uint32_t bitMasks[17] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff};

/* Unused function
double saturate(double value, double minimum, double maximum) {
  if (value > maximum) {
    return maximum;
  } else if (value < minimum) {
    return minimum;
  }
  return value;
}
*/

// Return the number of bits needed to represent the smallest range of numbers -2^N ... 2^N-1
// in which the given number belongs.
// For example:
// -1, 0 return 1;
// -2, 1 return 2;
// -4, -3, 2, 3 return 3;
// -8, -7, -6, -5, 4, 5, 6, 7 return 4;
// -16 .. 15 return 5;
// -32 .. 31 return 6;
// -64 .. 63 return 7;
// -128 .. 127 return 8;
// -256 .. 255 return 9;
inline int bitDepth16 (int16_t value16, int minBitDepth) {
  int32_t value32 = value16;
  if (value32 < 0) {
    value32 = value32 ^ 0xffffffff;
  }
  int bitDepth = 33 - std::countl_zero((uint32_t)value32); // Count leading zeros
  if (bitDepth < minBitDepth) {
    return minBitDepth;
  }  
  return bitDepth;
}

// It is OK if bitDepth is less than expGolombLikeParameter
inline int bitDepthToExpGolombLikeNumBits16(int16_t bitDepth, int expGolombLikeParameter) {
  if (bitDepth <= expGolombLikeParameter) {
    return 1 + expGolombLikeParameter;
  } else if (bitDepth == 16) {
    return 2*bitDepth - expGolombLikeParameter - 1;
  } else {
    return 2*bitDepth - expGolombLikeParameter;
  }
}

inline int valueToExpGolombLikeNumBits16(int16_t value16, int expGolombLikeParameter) {
  return bitDepthToExpGolombLikeNumBits16(bitDepth16(value16, expGolombLikeParameter), expGolombLikeParameter);
}

// The returned value will be at the LSB end.
inline uint32_t expGolombLikeEncode16(int16_t value16, int expGolombLikeParameter, int maxBitDepth, int &numBits) {
  int bitDepth = bitDepth16(value16, expGolombLikeParameter);
  if (bitDepth <= expGolombLikeParameter) {
    numBits = 1 + expGolombLikeParameter;
    return (uint16_t)value16 & bitMasks[expGolombLikeParameter];
  } else if (bitDepth >= maxBitDepth) {
    numBits = 2*bitDepth - expGolombLikeParameter - 1;
    return (bitMasks[bitDepth - expGolombLikeParameter + 1] << (bitDepth - 1)) | ((uint16_t)value16 & bitMasks[bitDepth-1]);
  } else {
    numBits = 2*bitDepth - expGolombLikeParameter;
    return (bitMasks[bitDepth - expGolombLikeParameter] << bitDepth) | ((uint16_t)value16 & bitMasks[bitDepth-1]);
  }
}

// expGolombLikeVal must begin at MSB. It is OK to have stuff after the exp-Golomb-like code on its LSB side.
inline int16_t expGolombLikeDecode16(uint32_t expGolombLikeVal, int expGolombLikeParameter, int maxBitDepth, int &bitDepth) {
  if (!(expGolombLikeVal & 0x80000000)) {
    bitDepth = expGolombLikeParameter;
    return (int32_t)(expGolombLikeVal << 1) >> (32-bitDepth);
  }
  for (bitDepth = expGolombLikeParameter + 1; bitDepth < maxBitDepth; bitDepth++) {
    expGolombLikeVal <<= 1;
    if (!(expGolombLikeVal & 0x80000000)) {
      if (!(expGolombLikeVal & 0x40000000)) {
	expGolombLikeVal |= 0x80000000;
      }
      return ((int32_t)expGolombLikeVal >> (32-bitDepth));
    }
  }
  if (expGolombLikeVal & 0x40000000) {
    expGolombLikeVal &= 0x7fffffff;
  }
  return ((int32_t)expGolombLikeVal >> (32-bitDepth));
}

class BitStreamWriter {
public:
  uint8_t *output;
  int numBitsWritten;

  // Write bits MSB first. The numBits bits at the LSB end of bitBuffer will be written to output.
  void write(uint32_t bits, int numBits) {
    bits <<= 32 - numBits; // MSB-align and zero LSB's
    for (;;) {
      // Clear each new byte as data becomes available to write to it
      if ((numBitsWritten & 7) == 0) {
        // Write from the beginning of a byte
        output[numBitsWritten >> 3] = bits >> (32-8);
        if (numBits > 8) {
          // Write whole byte and get ready to write more
          numBitsWritten += 8;
          bits <<= 8;
          numBits -= 8;
        } else {
          // Write everything in bit buffer and be done
          numBitsWritten += numBits;
          break;
        }
      } else {
        // Write from the middle of a byte
        int numBitsCanWrite = 8 - (numBitsWritten & 7);
        output[numBitsWritten >> 3] |= bits >> (32 - numBitsCanWrite);
        if (numBits > numBitsCanWrite) {
          // Write to the end of the byte and get ready to write more
          numBits -= numBitsCanWrite;
          bits <<= numBitsCanWrite;
          numBitsWritten += numBitsCanWrite;
        } else {
          // Write everything in bit buffer and be done
          numBitsWritten += numBits;
          break;
        }
      }    
    }
  }

  // Exp-Golomb-like encode and write value
  void writeExpGolombLike(int16_t value16, int expGolombLikeParameter, int maxBitDepth = 16) {
    int numBits;
    uint32_t expGolombLike = expGolombLikeEncode16(value16, expGolombLikeParameter, maxBitDepth, numBits);
    write(expGolombLike, numBits);
  }

  void writeResidualExpGolombLikeParameter(int expGolombLikeParameter) {
    write(residualExpGolombLikeParameterEncodings[expGolombLikeParameter - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER], residualExpGolombLikeParameterEncodingNumBits[expGolombLikeParameter - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]);    
  }

  BitStreamWriter(uint8_t *output): output(output), numBitsWritten(0) {
  }
};

class BitStreamReader {
public:
  const uint8_t *input;
  int totalNumBytes;
  int numBitsRead;
  // Read to the LSB end of val
  void read(uint32_t &val, int numBits) {
    // Use numBits as the variable that tells how many we still want to read.
    val = 0;    
    for (;;) {
      if ((numBitsRead & 7) == 0) {
        // Read from the beginning (MSB end) of a byte
        val <<= 8;
        val |= input[numBitsRead >> 3];
        if (numBits > 8) {
          // Read whole byte and get ready to read more
          numBits -= 8;
          numBitsRead += 8;
        } else {
          // Read partial byte (or whole byte but not more) and be done with it
          numBitsRead += numBits;
          val >>= (8 - numBits);
          break;
        }
      } else {
        // Read from the middle of a byte
        int numBitsToEndOfByte = 8 - (numBitsRead & 7);
	val <<= numBitsToEndOfByte;
	val |= input[numBitsRead >> 3] & bitMasks[numBitsToEndOfByte];
        if (numBits > numBitsToEndOfByte) {
          // Read to the end of the byte and get ready to read more
          numBits -= numBitsToEndOfByte;
          numBitsRead += numBitsToEndOfByte;
        } else {
          // Read at most to the end of the byte (or whole byte but not more) and be done with it
	  val >>= numBitsToEndOfByte - numBits;
          numBitsRead += numBits;
          break;
        }
      }      
    }
  }
  
  // *** Faking it! This does not report premature end of data. It does not access memory outside data.
  // Minimum exp-Golomb-like parameter is 6
  void readExpGolombLike(int16_t &val, int expGolombLikeParameter, int maxBitDepth = 16) {
    uint32_t bitBuf = input[numBitsRead >> 3];
    for (int i = (numBitsRead >> 3) + 1; i < (numBitsRead >> 3) + 4; i++) {
      bitBuf <<= 8;
      if (i < totalNumBytes) {
	bitBuf |= input[i];
      }
    }
    bitBuf <<= (numBitsRead & 7); // *** This only enables up to 25-bit exp-Golomb-like encodings
    int bitDepth;
    val = expGolombLikeDecode16(bitBuf, expGolombLikeParameter, maxBitDepth, bitDepth);
    if (bitDepth == expGolombLikeParameter) {
      numBitsRead += 1 + expGolombLikeParameter;
    } else if (bitDepth >= maxBitDepth) {
      numBitsRead += 2*bitDepth - expGolombLikeParameter - 1;
    } else {
      numBitsRead += 2*bitDepth - expGolombLikeParameter;
    }
  }
  
  /*  void readUnary(uint16_t &val, int maxValue = 7) {
      uint32_t bitBuf = input[numBitsRead >> 3];
      for (int i = (numBitsRead >> 3) + 1; i < (numBitsRead >> 3) + 4; i++) {
      bitBuf <<= 8;
      if (i < totalNumBytes) {
      bitBuf |= input[i];
      }
      }
      bitBuf <<= (numBitsRead & 7);
      val = unaryDecode16(bitBuf, maxValue);    
      // TODO EVERYTHING ***
      }*/
  
  void readResidualExpGolombLikeParameter(int &expGolombLikeParameter) {
    uint32_t bitBuf = input[numBitsRead >> 3];
    for (int i = (numBitsRead >> 3) + 1; i < (numBitsRead >> 3) + 2; i++) {
      bitBuf <<= 8;
      if (i < totalNumBytes) {
	bitBuf |= input[i];
      }
    }
    bitBuf <<= 16 + (numBitsRead & 7);
    expGolombLikeParameter = std::countl_zero((uint32_t)bitBuf); // Count leading zeros
    if (expGolombLikeParameter >= 8) {
      expGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER;
      numBitsRead += residualExpGolombLikeParameterEncodingNumBits[0];
    } else {
      numBitsRead += residualExpGolombLikeParameterEncodingNumBits[8 - expGolombLikeParameter];
      expGolombLikeParameter = RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + (8 - expGolombLikeParameter);
    }
    return;
  }
  
  BitStreamReader(const uint8_t *input, int numBytes): input(input), totalNumBytes(numBytes), numBitsRead(0) {
  }
};

inline int32_t saturate(int32_t value, int32_t minimum, int32_t maximum) {
  if (value > maximum) {
    return maximum;
  } else if (value < minimum) {
    return minimum;
  }
  return value;
}

inline int bestExpGolombLikeParameter16(int *bitDepthCounts, int &bestNumBits, int totalCount) {
  int numAboveBase = 0;
  bestNumBits = INT_MAX;
  int numBits = 16*totalCount - bitDepthCounts[16-RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER];
  for (int i = 16-RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i >= 0; i--) {
    numBits += numAboveBase;
    numAboveBase += bitDepthCounts[i];
    numBits += numAboveBase;
    if (numBits > bestNumBits) {
      return RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER + i;      
    }
    bestNumBits = numBits;
    numBits -= totalCount;
  }
  return RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER;
}

struct Channel {
  int16_t s[BLOCK_MAX_NUM_SAMPLETUPLES]; // Samples
  int bitDepthCounts[17 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]; // byte would be enough
  int expGolombLikeParameter;
  int numBits;
  void addToBitDepthCounts(int16_t value) {
    int bitDepth = bitDepth16(value, RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER);
    bitDepthCounts[bitDepth - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER]++;
  }
  void resetExpGolombLikeStats() {
    for (int i = 0; i <= 16 - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER; i++) {
      bitDepthCounts[i] = 0;
    }
  }
};

inline int16_t predict(int16_t xm2, int16_t xc2, int16_t xm1, int16_t xc1) {
  int32_t result = ((int32_t)xm2*(int32_t)xc2 + (int32_t)xm1*(int32_t)xc1 + (1 << (COEF_SHIFT - 1))) >> COEF_SHIFT; // Rounding gives marginal compression ratio improvement
  //#ifdef __ARM_FEATURE_SAT
  //  return (int16_t) __ssat32(result, 16);
  //#else  // __ARM_FEATURE_SAT
  return (int16_t)saturate(result, -0x8000, 0x7fff);
  //#endif // __ARM_FEATURE_SAT
  //return saturate(round(xm2*(float)xc2*(1.0f/COEF_DIVISOR) + xm1*(float)xc1*(1.0f/COEF_DIVISOR)), -32768.0f, 32767.0f); // Equivalent floating point calculation
}

inline int16_t predict(int16_t ym2, int16_t yc2, int16_t ym1, int16_t yc1, int16_t x0, int16_t yd0) {
  int32_t result = ((int32_t)ym2*(int32_t)yc2 + (int32_t)ym1*(int32_t)yc1 + (int32_t)x0*(int32_t)yd0 + (1 << (COEF_SHIFT - 1))) >> COEF_SHIFT; // Rounding gives marginal compression ratio improvement
  //  int32_t result = (int32_t)ym2*(int32_t)yc2 + (int32_t)ym1*(int32_t)yc1 + (int32_t)x0*(int32_t)yd0;
  //#ifdef __ARM_FEATURE_SAT
  //return (int16_t) __ssat32(result, 16);
  //#else  // __ARM_FEATURE_SAT
  //  asm ("ssat %0, #16, %0"
  //       : "+r" (result)); // Note COEF_SHIFT could be included as ", lsl #4"
  
  //return result;
  return (int16_t)saturate(result, -0x8000, 0x7fff);
  //#endif // __ARM_FEATURE_SAT
  //  return saturate(round(ym2*(float)yc2*(1.0f/COEF_DIVISOR) + ym1*(float)yc1*(1.0f/COEF_DIVISOR) + x0*(float)yd0*(1.0f/COEF_DIVISOR)), -32768.0f, 32767.0f); // Equivalent floating point calculation
}

struct LPCoefs {
  // Coefficients to calculate independent left channel sample i
  int16_t xc2; // Coef for left channel sample i-2
  int16_t xc1; // Coef for left channel sample i-1
  // Coefficients to calculate dependent right channel
  int16_t yc2; // Coef for right channel sample i-2
  int16_t yc1; // Coef for right channel sample i-1
  int16_t yd0; // Coef for left channel sample i
};

class MLACDecoder {
  int16_t x[BLOCK_MAX_NUM_SAMPLETUPLES];
  int16_t y[BLOCK_MAX_NUM_SAMPLETUPLES];

public:
  // MLAC decode
  // Arguments:
  //   input = pointer to beginning of a block of BLOCK_NUM_BYTES encoded audio.
  //   output = pointer to beggining of interleaved stereo 16-bit audio that must have room for at least BLOCK_MAX_NUM_SAMPLETUPLES stereo samples to be written.
  // Returns:
  //   timeStamp = time stamp read, not yet implemented. NOTE: TIME STAMPS ARE NOT YET FUNCTIONAL AND ARE INSTEAD USED FOR STORING NUMBER OF SAMPLE TUPLES
  //   numSampleTuplesRead = number of stereo samples read
  //   Return value = Effective resolution of audio in bits, 16 for lossless compression, less for lossy compression
  int decode(const uint8_t *input, int16_t *output, uint8_t &timeStamp, int &numSampleTuplesRead) {
    BitStreamReader reader(input, BLOCK_NUM_BYTES);

    // Read time stamp
    uint32_t temp;
    reader.read(temp, 8); 
    timeStamp = temp;
    numSampleTuplesRead = temp; // Fake it! ***    
    // Read channel mode
    uint32_t chMode;
    reader.read(chMode, 2);
    if (chMode != CHMODE_MSB) {
      // Read left channel warmup
      reader.readExpGolombLike(x[0], 14);
      reader.readExpGolombLike(x[1], 14);
      // Read right channel warmup
      reader.readExpGolombLike(y[0], 14);
      reader.readExpGolombLike(y[1], 14);
    }
    if (chMode == CHMODE_INDEPENDENT_AND_DEPENDENT) {
      int xrExpGolombLikeParameter, ydrExpGolombLikeParameter;
      int16_t xc1, xc2, yc1, yc2, yd0;
      // Read independent left channel exp-Golomb-like parameter and coefficients
      reader.readResidualExpGolombLikeParameter(xrExpGolombLikeParameter);
      reader.readExpGolombLike(xc1, C1_EXPGOLOMBLIKE_PARAMETER);
      xc1 += C1_BIAS;
      reader.readExpGolombLike(xc2, C2_EXPGOLOMBLIKE_PARAMETER);
      xc2 += C2_BIAS;
      // Read independent right channel exp-Golomb-like parameter and coefficients
      reader.readResidualExpGolombLikeParameter(ydrExpGolombLikeParameter);
      reader.readExpGolombLike(yc1, C1_EXPGOLOMBLIKE_PARAMETER);
      yc1 += C1_BIAS;
      reader.readExpGolombLike(yc2, C2_EXPGOLOMBLIKE_PARAMETER);
      yc2 += C2_BIAS;
      reader.readExpGolombLike(yd0, D0_EXPGOLOMBLIKE_PARAMETER);
      yd0 += D0_BIAS;
      // Read audio data residues
      for (int i = NUM_LP_COEFS; i < numSampleTuplesRead; i++) {
	int16_t xr, ydr;
	reader.readExpGolombLike(xr, xrExpGolombLikeParameter);
	reader.readExpGolombLike(ydr, ydrExpGolombLikeParameter);
	x[i] = predict(x[i - 2], xc2, x[i - 1], xc1) + xr;
	y[i] = predict(y[i - 2], yc2, y[i - 1], yc1, x[i], yd0) + ydr;
      }
    } else { // chMode == CHMODE_MSB
      // Read true bit depth
      uint32_t trueBitDepth;
      reader.read(trueBitDepth, 4);
      trueBitDepth += TRUE_BITDEPTH_BIAS;
      // Read raw PCM audio
      if (trueBitDepth == 16) {
        for (int i = 0; i < chModeMSBNumSampleTuples[trueBitDepth]; i++) {
          uint32_t val;
          reader.read(val, trueBitDepth);
          output[i*2 + 0] = val;
          reader.read(val, trueBitDepth); 
          output[i*2 + 1] = val;
        }
      } else {
        for (int i = 0; i < chModeMSBNumSampleTuples[trueBitDepth]; i++) {
          uint32_t val;
          reader.read(val, trueBitDepth);
          output[i*2 + 0] = (val << (16 - trueBitDepth)) | (0x8000 >> trueBitDepth);
          reader.read(val, trueBitDepth); 
          output[i*2 + 1] = (val << (16 - trueBitDepth)) | (0x8000 >> trueBitDepth);
        }
      }
      return trueBitDepth;
    }
    // chMode != CHMODE MSB
    output[0] = x[0];
    output[1] = y[0];
    for (int i = 1; i < numSampleTuplesRead; i++) {
      output[2*i + 0] = output[2*(i - 1) + 0] + x[i];
      output[2*i + 1] = output[2*(i - 1) + 1] + y[i];
    }
    return 16;
  }
};

class MLACEncoder {
  Channel xr;
  Channel ydr;
  int16_t x[BLOCK_MAX_NUM_SAMPLETUPLES];
  int16_t y[BLOCK_MAX_NUM_SAMPLETUPLES];

public:
  // MLAC encode
  // Arguments:
  //   input = pointer to begining of interleaved stereo 16-bit audio that must contain at least BLOCK_MAX_NUM_SAMPLETUPLES stereo samples.
  //   output = pointer to beginning of a block of encoded audio to be written. Will write BLOCK_NUM_BYTES bytes.
  //   timeStamp = time stamp to be written, not yet implemented. NOTE: TIME STAMPS ARE NOT YET FUNCTIONAL AND ARE INSTEAD USED FOR STORING NUMBER OF SAMPLE TUPLES
  // Returns:
  //   numSampleTuplesWritten = number of stereo sample pairs encoded
  //   numBitsWritten = number of bits written (if less than BLOCK_NUM_BYTES*8, then there is room for auxiliary data after encoded audio)
  //   minNumSampleTuples = minimum number of stero samples that must fit to packet, range: BLOCK_MIN_NUM_SAMPLETUPLES inclusive to BLOCK_MAX_NUM_SAMPLETUPLES inclusive.
  //                        this setting can force lossy compression.
  //   Return value = Effective resolution of audio in bits, 16 for lossless compression, less for lossy compression
  int encode(const int16_t *input, uint8_t *output, uint8_t timeStamp, int &numSampleTuplesWritten, int &numBitsWritten, int minNumSampleTuples = BLOCK_MIN_NUM_SAMPLETUPLES) {
    
    int trueBitDepth = 16;
    BitStreamWriter writer(output);
    
    // Delta values
    
    x[0] = input[0];
    y[0] = input[1];
    for (int i = 1; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
      x[i] = input[i*2] - input[(i - 1)*2];
      y[i] = input[i*2 + 1] - input[(i - 1)*2 + 1];
    }

    int j = 2;

    int64_t x1x1Pre = x[1]*(int32_t)x[1];
    int64_t x0x0Pre = x[0]*(int32_t)x[0] + x1x1Pre;
    int64_t x1x2Pre = x[1]*(int32_t)x[2];
    int64_t x0x1Pre = x[0]*(int32_t)x[1] + x1x2Pre;
    int64_t x0x2 = x[0]*(int32_t)x[2] + x[1]*(int32_t)x[3];

    int64_t y1y1Pre = y[1]*(int32_t)y[1];
    int64_t y0y0Pre = y[0]*(int32_t)y[0] + y1y1Pre;
    int64_t y1y2Pre = y[1]*(int32_t)y[2];
    int64_t y0y1Pre = y[0]*(int32_t)y[1] + y1y2Pre;
    int64_t y0y2 = y[0]*(int32_t)y[2] + y[1]*(int32_t)y[3];
    
    int64_t x2y2 = x[0 + 2]*(int32_t)y[0 + 2] + x[1 + 2]*(int32_t)y[1 + 2];
    int64_t y1x2 = y[0 + 1]*(int32_t)x[0 + 2] + y[1 + 1]*(int32_t)x[1 + 2];
    int64_t y0x2 = y[0]*(int32_t)x[0 + 2] + y[1]*(int32_t)x[1 + 2];
    
    int64_t xx0 = 0;
    int64_t xx1 = 0;
    
    int64_t yy0 = 0;
    int64_t yy1 = 0;
        
    int bestChMode = CHMODE_MSB;
    LPCoefs c;
    LPCoefs bestc;
    int bestNumSampleTuples = BLOCK_MIN_NUM_SAMPLETUPLES;
    int bestNumBits = 8 + 2 + 1 + bestNumSampleTuples*2*16;

    int commonNumBits =
      ( 8
        // time stamp      
        + 2 // chmode
        + valueToExpGolombLikeNumBits16(x[0], 14) + valueToExpGolombLikeNumBits16(y[0], 14) + valueToExpGolombLikeNumBits16(x[1], 14) + valueToExpGolombLikeNumBits16(y[1], 14) // warmup
        );
    int numAvailableBits = BLOCK_NUM_BYTES*8 - commonNumBits;
    int targetNumSampleTuples = BLOCK_MIN_NUM_SAMPLETUPLES + 1;
    int chMode;
    int numBits;
    int bestxrExpGolombLikeParameter;
    int bestydrExpGolombLikeParameter;

    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {

      // Calculate linear prediction coefficients. Aim a bit higher with numSampleTuples than we are sure we can go.
      
      for (; j < targetNumSampleTuples - NUM_LP_COEFS - 1; j += 2) { // This unrolled loop gives 3 % speedup on BeagleBone Black. This loop be removed without further modifications.
        xx0 -= (int32_t) - (x[j]*(int32_t)x[j]) - x[j + 1]*(int32_t)x[j + 1]; // Dual 16x16 multiply and 32-bit subtractive accumulate (- 0x40000000 - 0x40000000 is valid).
        xx1 -= (int32_t) - (x[j]*(int32_t)x[j + 1]) - x[j + 1]*(int32_t)x[j + 2];
        x0x2 -= (int32_t) - (x[j]*(int32_t)x[j + 2]) - x[j + 1]*(int32_t)x[j + 3];

        yy0 -= (int32_t) - (y[j]*(int32_t)y[j]) - y[j + 1]*(int32_t)y[j + 1];
        yy1 -= (int32_t) - (y[j]*(int32_t)y[j + 1]) - y[j + 1]*(int32_t)y[j + 2];
        y0y2 -= (int32_t) - (y[j]*(int32_t)y[j + 2]) - y[j + 1]*(int32_t)y[j + 3];

	x2y2 -= (int32_t) - (x[j + 2]*(int32_t)y[j + 2]) - x[j + 3]*(int32_t)y[j + 3];
	y1x2 -= (int32_t) - (y[j + 1]*(int32_t)x[j + 2]) - y[j + 2]*(int32_t)x[j + 3];
	y0x2 -= (int32_t) - (y[j]*(int32_t)x[j + 2]) - y[j + 1]*(int32_t)x[j + 3];
      }
      for (; j < targetNumSampleTuples - NUM_LP_COEFS; j++) {
        xx0 += x[j]*(int32_t)x[j];
        xx1 += x[j]*(int32_t)x[j + 1];
        x0x2 += x[j]*(int32_t)x[j + 2];

        yy0 += y[j]*(int32_t)y[j];
        yy1 += y[j]*(int32_t)y[j + 1];
        y0y2 += y[j]*(int32_t)y[j + 2];

	x2y2 += x[j + 2]*(int32_t)y[j + 2];
	y1x2 += y[j + 1]*(int32_t)x[j + 2];
	y0x2 += y[j]*(int32_t)x[j + 2];        
      }

      // 01234.............j 
      // PP++++++++++++++++   x0x0
      //  P++++++++++++++++P  x1x1
      //   ++++++++++++++++PP x2x2
      // 012345............j
      // PP++++++++++++++++   x0x1
      //  P++++++++++++++++P  x1x2
      // 012345............j
      // PP++++++++++++++++   x0x2

      int64_t x0x0 = x0x0Pre + xx0;
      int64_t x1x1 = x1x1Pre + xx0 + x[j]*(int32_t)x[j];
      int64_t x2x2 = xx0 + (x[j]*(int32_t)x[j] + x[j + 1]*(int32_t)x[j + 1]);
      int64_t x0x1 = x0x1Pre + xx1;
      int64_t x1x2 = x1x2Pre + xx1 + x[j]*(int32_t)x[j + 1];

      int64_t y0y0 = y0y0Pre + yy0;
      int64_t y1y1 = y1y1Pre + yy0 + y[j]*(int32_t)y[j];
      int64_t y0y1 = y0y1Pre + yy1;
      int64_t y1y2 = y1y2Pre + yy1 + y[j]*(int32_t)y[j + 1];

      // The above is an optimization of:
      /*
      for (; j < targetNumSampleTuples - NUM_LP_COEFS; j++) {
        x0x0 += x[j]*(int32_t)x[j];
	x0x1 += x[j]*(int32_t)x[j + 1];
	x0x2 += x[j]*(int32_t)x[j + 2];
	x1x1 += x[j + 1]*(int32_t)x[j + 1];
	x1x2 += x[j + 1]*(int32_t)x[j + 2];
	x2x2 += x[j + 2]*(int32_t)x[j + 2];
	y0y0 += y[j]*(int32_t)y[j];
	y0y1 += y[j]*(int32_t)y[j + 1];
	y0y2 += y[j]*(int32_t)y[j + 2];
	y1x2 += y[j + 1]*(int32_t)x[j + 2];
	y1y1 += y[j + 1]*(int32_t)y[j + 1];
	y1y2 += y[j + 1]*(int32_t)y[j + 2];
	y0x2 += y[j]*(int32_t)x[j + 2];
      }
      */
      
      float xDivisor = (float)x0x0*x1x1 - (float)x0x1*x0x1;
      float xc1f, xc2f;
      if (xDivisor == 0) {
	xc1f = 0;
	xc2f = 0;
      } else {
	float xInvDivisor = 1.0f/xDivisor;
	xc1f = ((float)x0x0*x1x2 - (float)x0x1*x0x2)*xInvDivisor;
	xc2f = ((float)x0x2*x1x1 - (float)x0x1*x1x2)*xInvDivisor;
      }
      //      float dev_xc1f = x0x1/(float)x0x0;
      //      printf("%f,%f,%f,", xc1f, xc2f, dev_xc1f);
      c.xc1 = saturate((int)round(xc1f*COEF_DIVISOR), C1_MIN + C1_BIAS, C1_MAX + C1_BIAS);
      c.xc2 = saturate((int)round(xc2f*COEF_DIVISOR), C2_MIN + C2_BIAS, C2_MAX + C2_BIAS);
      float ydivisor = 2.0f*y0y1*y0x2*y1x2 - (float)y0y1*y0y1*x2x2 + (float)y0y0*y1y1*x2x2 - (float)y0y0*y1x2*y1x2 - (float)y0x2*y0x2*y1y1;
      float yc1f, yc2f, yd0f;
      if (ydivisor == 0) {
	yc1f = 0;
	yc2f = 0;
	yd0f = 0;
      } else {
	float ydInvDivisor = 1.0f/ydivisor;
	yc1f = ((float)y0y0*y1y2*x2x2 - (float)y0y0*y1x2*x2y2 - (float)y0y1*y0y2*x2x2 + (float)y0y1*y0x2*x2y2 + (float)y0y2*y0x2*y1x2 - (float)y0x2*y0x2*y1y2)*ydInvDivisor;
	yc2f = ((float)y0y2*y1y1*x2x2 - (float)y0y2*y1x2*y1x2 - (float)y0y1*y1y2*x2x2 + (float)y0y1*y1x2*x2y2 - (float)y0x2*y1y1*x2y2 + (float)y0x2*y1y2*y1x2)*ydInvDivisor;
	yd0f = ((float)y0y0*y1y1*x2y2 - (float)y0y0*y1y2*y1x2 - (float)y0y1*y0y1*x2y2 + (float)y0y1*y0y2*y1x2 + (float)y0y1*y0x2*y1y2 - (float)y0y2*y0x2*y1y1)*ydInvDivisor;
      }
      c.yc1 = saturate((int)round(yc1f*COEF_DIVISOR), C1_MIN + C1_BIAS, C1_MAX + C1_BIAS);
      c.yc2 = saturate((int)round(yc2f*COEF_DIVISOR), C2_MIN + C2_BIAS, C2_MAX + C2_BIAS);
      c.yd0 = saturate((int)round(yd0f*COEF_DIVISOR), D0_MIN + D0_BIAS, D0_MAX + D0_BIAS);
 
      // Do linear prediction with the new coefficients
     
      xr.resetExpGolombLikeStats();
      ydr.resetExpGolombLikeStats();
      for (int i = NUM_LP_COEFS; i < targetNumSampleTuples; i++) {
	xr.s[i] = x[i] - predict(x[i - 2], c.xc2, x[i - 1], c.xc1);
	ydr.s[i] = y[i] - predict(y[i - 2], c.yc2, y[i - 1], c.yc1, x[i], c.yd0);
	xr.addToBitDepthCounts(xr.s[i]);
	ydr.addToBitDepthCounts(ydr.s[i]);
      }
      xr.expGolombLikeParameter = bestExpGolombLikeParameter16(xr.bitDepthCounts, xr.numBits, targetNumSampleTuples - NUM_LP_COEFS);
      ydr.expGolombLikeParameter = bestExpGolombLikeParameter16(ydr.bitDepthCounts, ydr.numBits, targetNumSampleTuples - NUM_LP_COEFS);
      xr.numBits += residualExpGolombLikeParameterEncodingNumBits[xr.expGolombLikeParameter - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER] + valueToExpGolombLikeNumBits16(c.xc1-C1_BIAS, C1_EXPGOLOMBLIKE_PARAMETER) + valueToExpGolombLikeNumBits16(c.xc2-C2_BIAS, C2_EXPGOLOMBLIKE_PARAMETER);
      ydr.numBits += residualExpGolombLikeParameterEncodingNumBits[ydr.expGolombLikeParameter - RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER] + valueToExpGolombLikeNumBits16(c.yc1-C1_BIAS, C1_EXPGOLOMBLIKE_PARAMETER) + valueToExpGolombLikeNumBits16(c.yc2-C2_BIAS, C2_EXPGOLOMBLIKE_PARAMETER) + valueToExpGolombLikeNumBits16(c.yd0-D0_BIAS, D0_EXPGOLOMBLIKE_PARAMETER);
      int xrydrNumBits = xr.numBits + ydr.numBits;
      chMode = CHMODE_INDEPENDENT_AND_DEPENDENT;
      numBits = xrydrNumBits;
      if (numBits > numAvailableBits) {
	//	printf("Early out\n");
        break;
      } 
      bestChMode = chMode;
      bestc = c;
      bestNumSampleTuples = targetNumSampleTuples;
      bestNumBits = numBits;
      bestxrExpGolombLikeParameter = xr.expGolombLikeParameter;
      bestydrExpGolombLikeParameter = ydr.expGolombLikeParameter;
      if (numBits +  2*(1 + RESIDUAL_EXPGOLOMBLIKE_MIN_PARAMETER) <= numAvailableBits) {
        for (int i = targetNumSampleTuples; i < BLOCK_MAX_NUM_SAMPLETUPLES; i++) {
          xr.s[i] = x[i] - predict(x[i - 2], c.xc2, x[i - 1], c.xc1);
          ydr.s[i] = y[i] - predict(y[i - 2], c.yc2, y[i - 1], c.yc1, x[i], c.yd0);
          // Could maybe (or not) do these more efficiently via totalExpGolombLikeNumBits16
          xrydrNumBits += valueToExpGolombLikeNumBits16(xr.s[i], xr.expGolombLikeParameter) + valueToExpGolombLikeNumBits16(ydr.s[i], ydr.expGolombLikeParameter);
          int candidateChMode = CHMODE_INDEPENDENT_AND_DEPENDENT;
          int candidateNumBits = xrydrNumBits;
          if (candidateNumBits <= numAvailableBits) {
            bestChMode = candidateChMode;
            bestNumSampleTuples = i + 1;
            bestNumBits = candidateNumBits;
          } else {
            // Possible todo: Could use new exp-Golomb-like parameterization here
            break;
          }
        }
      }
      targetNumSampleTuples = bestNumSampleTuples + 1;
    }

    // *** This could be done more smartly
    for (int i = NUM_LP_COEFS; i < bestNumSampleTuples; i++) {
      xr.s[i] = x[i] - predict(x[i - 2], bestc.xc2, x[i - 1], bestc.xc1);
      ydr.s[i] = y[i] - predict(y[i - 2], bestc.yc2, y[i - 1], bestc.yc1, x[i], bestc.yd0);
    }

    if (bestNumSampleTuples < minNumSampleTuples) {// *** Move this up for efficiency
      //      bestChMode = CHMODE_MSB;
    }
      
    
    // Write time stamp
    if (bestChMode == CHMODE_MSB) {
      for (; trueBitDepth >= 8; trueBitDepth--) {
	if (chModeMSBNumSampleTuples[trueBitDepth] >= minNumSampleTuples) break; 
      }
      bestNumSampleTuples = chModeMSBNumSampleTuples[trueBitDepth];
      // *** numBitsWritten is vague
    }
    timeStamp = bestNumSampleTuples; // Fake it! ***    
    writer.write(timeStamp, 8);
    // Write channel mode
    writer.write(bestChMode, 2);
    if (bestChMode == CHMODE_INDEPENDENT_AND_DEPENDENT) {
      // Write left channel warmup
      writer.writeExpGolombLike(x[0], 14);
      writer.writeExpGolombLike(x[1], 14);
      // Write right channel warmup
      writer.writeExpGolombLike(y[0], 14);
      writer.writeExpGolombLike(y[1], 14);
      // Write independent left channel exp-Golomb-like parameter and coefficients
      writer.writeResidualExpGolombLikeParameter(bestxrExpGolombLikeParameter);
      writer.writeExpGolombLike(bestc.xc1-C1_BIAS, C1_EXPGOLOMBLIKE_PARAMETER);
      writer.writeExpGolombLike(bestc.xc2-C2_BIAS, C2_EXPGOLOMBLIKE_PARAMETER);
      // Write dependent right channel exp-Golomb-like parameter and coefficients
      writer.writeResidualExpGolombLikeParameter(bestydrExpGolombLikeParameter);
      writer.writeExpGolombLike(bestc.yc1-C1_BIAS, C1_EXPGOLOMBLIKE_PARAMETER);
      writer.writeExpGolombLike(bestc.yc2-C2_BIAS, C2_EXPGOLOMBLIKE_PARAMETER);      
      writer.writeExpGolombLike(bestc.yd0-D0_BIAS, D0_EXPGOLOMBLIKE_PARAMETER);      
      // Write audio data residues
      for (int i = NUM_LP_COEFS; i < bestNumSampleTuples; i++) {
	writer.writeExpGolombLike(xr.s[i], bestxrExpGolombLikeParameter);
	writer.writeExpGolombLike(ydr.s[i], bestydrExpGolombLikeParameter);
      }
    } else { // bestChMode == CHMODE_MSB
      // Write true bit depth
      writer.write(trueBitDepth - TRUE_BITDEPTH_BIAS, 4);
      // Write raw PCM audio
      if (trueBitDepth == 16) {
        for (int i = 0; i < bestNumSampleTuples; i++) {
          writer.write(input[i*2 + 0], trueBitDepth);
          writer.write(input[i*2 + 1], trueBitDepth);
        }
      } else {
        for (int i = 0; i < bestNumSampleTuples; i++) {
          writer.write(input[i*2 + 0] >> (16 - trueBitDepth), trueBitDepth);
          writer.write(input[i*2 + 1] >> (16 - trueBitDepth), trueBitDepth);
        }
      }
    }
    numSampleTuplesWritten = bestNumSampleTuples;
    numBitsWritten = bestNumBits;
    return trueBitDepth;
  }
};
