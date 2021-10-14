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
#include "microfrontend/lib/window.h"

#include <string.h>

#ifdef __ARM_FEATURE_MVE

#include <arm_mve.h>

static int16_t arm_win_process_samples_mve(const int16_t * pSrc,
                                const int16_t * pCoef, uint32_t blockSize, int16_t * pResult)
{
    int32_t         blkSize = blockSize;
    int16x8_t       curExtremValVec = vdupq_n_s16(0);
    int16_t         maxValue = 0;

    int16x8_t       vecSrc = vld1q(pSrc);
    pSrc += 8;

    do {
        mve_pred16_t    p = vctp16q(blkSize);
        int16x8_t       vecDst, vecCoef;

        vecCoef = vld1q_z(pCoef, p);

        /* long multiply + narrowing */
        vecDst = vuninitializedq_s16();
        vecDst = vqshrnbq_m_n_s32(vecDst, vmullbq_int(vecSrc, vecCoef), kFrontendWindowBits, p);
        vecDst = vqshrntq_m_n_s32(vecDst, vmulltq_int(vecSrc, vecCoef), kFrontendWindowBits, p);

        vecSrc = vld1q_z(pSrc, p);

        vst1q_p(pResult, vecDst, p);

        curExtremValVec = vmaxq_m(vecDst, vabsq(vecDst), curExtremValVec, p);

        blkSize -= 8;
        pSrc += 8;
        pCoef += 8;
        pResult += 8;

    }
    while (blkSize > 0);

    return (vmaxvq(maxValue, curExtremValVec));
}
#endif

int WindowProcessSamples(struct WindowState* state, const int16_t* samples,
                         size_t num_samples, size_t* num_samples_read) {
  const int size = state->size;

  // Copy samples from the samples buffer over to our local input.
  size_t max_samples_to_copy = state->size - state->input_used;
  if (max_samples_to_copy > num_samples) {
    max_samples_to_copy = num_samples;
  }
  memcpy(state->input + state->input_used, samples,
         max_samples_to_copy * sizeof(*samples));
  *num_samples_read = max_samples_to_copy;
  state->input_used += max_samples_to_copy;

  if (state->input_used < state->size) {
    // We don't have enough samples to compute a window.
    return 0;
  }

  // Apply the window to the input.
  const int16_t* coefficients = state->coefficients;
  const int16_t* input = state->input;
  int16_t* output = state->output;
  int i;
  int16_t max_abs_output_value = 0;
#ifndef __ARM_FEATURE_MVE

  for (i = 0; i < size; ++i) {
    int16_t new_value =
        (((int32_t)*input++) * *coefficients++) >> kFrontendWindowBits;
    *output++ = new_value;
    if (new_value < 0) {
      new_value = -new_value;
    }
    if (new_value > max_abs_output_value) {
      max_abs_output_value = new_value;
    }
  }
#else
   max_abs_output_value = arm_win_process_samples_mve(input, coefficients, size, output);
#endif

  // Shuffle the input down by the step size, and update how much we have used.
  memmove(state->input, state->input + state->step,
          sizeof(*state->input) * (state->size - state->step));
  state->input_used -= state->step;
  state->max_abs_output_value = max_abs_output_value;

  // Indicate that the output buffer is valid for the next stage.
  return 1;
}

void WindowReset(struct WindowState* state) {
  memset(state->input, 0, state->size * sizeof(*state->input));
  memset(state->output, 0, state->size * sizeof(*state->output));
  state->input_used = 0;
  state->max_abs_output_value = 0;
}
