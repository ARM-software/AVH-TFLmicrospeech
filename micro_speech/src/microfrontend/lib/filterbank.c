/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "microfrontend/lib/filterbank.h"

#include <string.h>

#include "microfrontend/lib/bits.h"

#ifdef __ARM_FEATURE_MVE

#include <arm_mve.h>
#include "arm_math.h"


#if (__ARM_FEATURE_MVE & 2)
#define INVSQRT_MAGIC_F32           0x5f3759df

__STATIC_INLINE f32x4_t visqrtf_f32(
    f32x4_t vecIn)
{
    int32x4_t       vecNewtonInit = vdupq_n_s32(INVSQRT_MAGIC_F32);
    f32x4_t         vecOneHandHalf = vdupq_n_f32(1.5f);
    f32x4_t         vecDst;
    f32x4_t         vecHalf;
    int32x4_t       vecTmpInt;
    f32x4_t         vecTmpFlt, vecTmpFlt1;


    vecHalf = vmulq_n_f32(vecIn, 0.500001f);

    /*
     * cast input float vector to integer and right shift by 1
     */
    vecTmpInt = vshrq_n_s32((int32x4_t) vecIn, 1);
    /*
     * INVSQRT_MAGIC - ((vec_q32_t)vecIn >> 1)
     */
    vecTmpInt = vsubq(vecNewtonInit, vecTmpInt);
    /*
     *------------------------------
     * 1st iteration
     *------------------------------
     * (1.5f-xhalf*x*x)
     */
    vecTmpFlt1 = vmulq((f32x4_t) vecTmpInt, (f32x4_t) vecTmpInt);
    vecTmpFlt1 = vmulq(vecTmpFlt1, vecHalf);
    vecTmpFlt1 = vsubq(vecOneHandHalf, vecTmpFlt1);
    /*
     * x = x*(1.5f-xhalf*x*x);
     */
    vecTmpFlt = vmulq((f32x4_t) vecTmpInt, vecTmpFlt1);

    /*
     *------------------------------
     * 2nd iteration
     *------------------------------
     */
    vecTmpFlt1 = vmulq(vecTmpFlt, vecTmpFlt);
    vecTmpFlt1 = vmulq(vecTmpFlt1, vecHalf);
    vecTmpFlt1 = vsubq(vecOneHandHalf, vecTmpFlt1);
    vecDst = vmulq(vecTmpFlt, vecTmpFlt1);
    /*
     * set negative values to NAN
     */
    vecDst = vdupq_m(vecDst, NAN, vcmpltq(vecIn, 0.0f));
    vecDst = vdupq_m(vecDst, INFINITY, vcmpeqq(vecIn, 0.0f));
    return vecDst;
}

__STATIC_FORCEINLINE f32x4_t vsqrtf_f32(
    f32x4_t vecIn)
{
    f32x4_t         vecDst;

    /* inverse square root unsing 2 newton iterations */
    vecDst = visqrtf_f32(vecIn);
    vecDst = vdupq_m(vecDst, 0.0f, vcmpeqq(vecIn, 0.0f));
    vecDst = vecDst * vecIn;
    return vecDst;
}

#endif


static void arm_cmplx_lmag_squared_q15(
  const int16_t * pSrc,
        int32_t * pDst,
        uint32_t numSamples)
{
    int32_t         blkSize = numSamples;
    int16x8_t       vecSrc;
    vecSrc = vld1q(pSrc);
    pSrc += 8;

    do {
        mve_pred16_t    p = vctp32q(blkSize);

        vst1q_p(pDst,
                vaddq_x(vmullbq_int(vecSrc, vecSrc), vmulltq_int(vecSrc, vecSrc), p), p);
        vecSrc = vld1q_z(pSrc, p);

        blkSize -= 4;
        pSrc += 8;
        pDst += 4;
    }
    while (blkSize > 0);
}

#endif

void FilterbankConvertFftComplexToEnergy(struct FilterbankState* state,
                                         struct complex_int16_t* fft_output,
                                         int32_t* energy) {
  const int end_index = state->end_index;
  int i;
  energy += state->start_index;
  fft_output += state->start_index;

#ifndef __ARM_FEATURE_MVE
  for (i = state->start_index; i < end_index; ++i) {
    const int32_t real = fft_output->real;
    const int32_t imag = fft_output->imag;
    fft_output++;
    const uint32_t mag_squared = (real * real) + (imag * imag);
    *energy++ = mag_squared;
  }
#else
    arm_cmplx_lmag_squared_q15(&fft_output->real, energy, end_index - state->start_index);
#endif
}

void FilterbankAccumulateChannels(struct FilterbankState* state,
                                  const int32_t* energy) {
  uint64_t* work = state->work;
  uint64_t weight_accumulator = 0;
  uint64_t unweight_accumulator = 0;

  const int16_t* channel_frequency_starts = state->channel_frequency_starts;
  const int16_t* channel_weight_starts = state->channel_weight_starts;
  const int16_t* channel_widths = state->channel_widths;

  int num_channels_plus_1 = state->num_channels + 1;
  int i;
#ifndef __ARM_FEATURE_MVE
  for (i = 0; i < num_channels_plus_1; ++i) {
    const int32_t* magnitudes = energy + *channel_frequency_starts++;
    const int16_t* weights = state->weights + *channel_weight_starts;
    const int16_t* unweights = state->unweights + *channel_weight_starts++;
    const int width = *channel_widths++;
    int j;
    for (j = 0; j < width; ++j) {
      weight_accumulator += *weights++ * ((uint64_t)*magnitudes);
      unweight_accumulator += *unweights++ * ((uint64_t)*magnitudes);
      ++magnitudes;
    }
    *work++ = weight_accumulator;
    weight_accumulator = unweight_accumulator;
    unweight_accumulator = 0;
  }
#else
  uint32_t* work32 = (uint32_t*)work;

  for (i = 0; i < num_channels_plus_1; ++i) {
    const int32_t* magnitudes = energy + *channel_frequency_starts++;
    const int16_t* weights = state->weights + *channel_weight_starts;
    const int16_t* unweights = state->unweights + *channel_weight_starts++;
    const int width = *channel_widths++;
    int j;

    for (j = 0; j < width/4; ++j) {
        weight_accumulator = vmlaldavaq(weight_accumulator, vld1q(magnitudes), vldrhq_s32(weights));
        unweight_accumulator = vmlaldavaq(unweight_accumulator, vld1q(magnitudes), vldrhq_s32(unweights));

        magnitudes += 4;
        weights+=4;
        unweights+=4;
    }

#if !(__ARM_FEATURE_MVE & 2)
    *work++ = weight_accumulator;
#else
    *work32++ = asrl(weight_accumulator, 16);;
#endif
    weight_accumulator = unweight_accumulator;
    unweight_accumulator = 0;
  }
#endif
}

static uint16_t Sqrt32(uint32_t num) {
  if (num == 0) {
    return 0;
  }
  uint32_t res = 0;
  int max_bit_number = 32 - MostSignificantBit32(num);
  max_bit_number |= 1;
  uint32_t bit = 1U << (31 - max_bit_number);
  int iterations = (31 - max_bit_number) / 2 + 1;
  while (iterations--) {
    if (num >= res + bit) {
      num -= res + bit;
      res = (res >> 1U) + bit;
    } else {
      res >>= 1U;
    }
    bit >>= 2U;
  }
  // Do rounding - if we have the bits.
  if (num > res && res != 0xFFFF) {
    ++res;
  }
  return res;
}

static uint32_t Sqrt64(uint64_t num) {
  // Take a shortcut and just use 32 bit operations if the upper word is all
  // clear. This will cause a slight off by one issue for numbers close to 2^32,
  // but it probably isn't going to matter (and gives us a big performance win).
  if ((num >> 32) == 0) {
    return Sqrt32((uint32_t)num);
  }
  uint64_t res = 0;
  int max_bit_number = 64 - MostSignificantBit64(num);
  max_bit_number |= 1;
  uint64_t bit = 1ULL << (63 - max_bit_number);
  int iterations = (63 - max_bit_number) / 2 + 1;
  while (iterations--) {
    if (num >= res + bit) {
      num -= res + bit;
      res = (res >> 1U) + bit;
    } else {
      res >>= 1U;
    }
    bit >>= 2U;
  }
  // Do rounding - if we have the bits.
  if (num > res && res != 0xFFFFFFFFLL) {
    ++res;
  }
  return res;
}

uint32_t* FilterbankSqrt1(struct FilterbankState* state, int scale_down_shift) {
  const int num_channels = state->num_channels;
  const uint64_t* work = state->work + 1;
  // Reuse the work buffer since we're fine clobbering it at this point to hold
  // the output.
  uint32_t* output = (uint32_t*)state->work;
  int i;
  for (i = 0; i < num_channels; ++i) {
    *output++ = Sqrt64(*work++) >> scale_down_shift;
  }
  return (uint32_t*)state->work;
}


uint32_t* FilterbankSqrt(struct FilterbankState* state, int scale_down_shift) {
  const int num_channels = state->num_channels;
  const uint64_t* work = state->work + 1;
  // Reuse the work buffer since we're fine clobbering it at this point to hold
  // the output.
  uint32_t* output = (uint32_t*)state->work;
  int i;

#if !(__ARM_FEATURE_MVE & 2)
  for (i = 0; i < num_channels; ++i) {
    *output++ = Sqrt64(*work++) >> scale_down_shift;
  }
#else
  const uint32_t* work32 = (uint32_t*)(state->work);//
  // jump over 1st bin
  work32 = work32 + 1;

  float32_t scale = powf(2.0f, 8-scale_down_shift);

  for (i = 0; i < num_channels/4; ++i) {
      int32x4_t vsrc = vld1q(work32);
      f32x4_t vsrcf = vcvtq_f32_s32(vsrc);
      f32x4_t vdst = vsqrtf_f32(vsrcf);

      vstrwq_u32(output, vcvtpq_s32_f32(vdst*scale));
      output+=4;
      work32+=4;
  }

#endif
  return (uint32_t*)state->work;
}

void FilterbankReset(struct FilterbankState* state) {
  memset(state->work, 0, (state->num_channels + 1) * sizeof(*state->work));
}
