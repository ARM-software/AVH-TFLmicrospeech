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

#include "audio_provider.h"

#include "micro_features/micro_model_settings.h"
#include "audio_drv.h"

#define AUDIO_BLOCK_NUM         (4)
#define AUDIO_BLOCK_SIZE        (3200)
#define AUDIO_BUFFER_SIZE       (AUDIO_BLOCK_NUM*AUDIO_BLOCK_SIZE)

namespace {
bool    g_is_audio_initialized = false;
bool    g_is_audio_ready = false;
#ifdef __FVP_PY
__attribute__((section(".ARM.__at_0x9FFF0000")))
#endif
__attribute__((aligned(4)))
int16_t g_audio_buf [AUDIO_BUFFER_SIZE/2];
int16_t g_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

static void AudioEvent (uint32_t event) {
  if (!g_is_audio_ready) {
    g_is_audio_ready = true;
    return;
  }
  if (event & AUDIO_DRV_EVENT_RX_DATA) {
    g_latest_audio_timestamp += ((AUDIO_BLOCK_SIZE/2) * 1000) / kAudioSampleFrequency;
  }
}

static int32_t AudioDrv_Setup(void) {
  int32_t ret;

  ret = AudioDrv_Initialize(AudioEvent);
  if (ret != 0) {
    return ret;
  }

  ret = AudioDrv_Configure(AUDIO_DRV_INTERFACE_RX,
                           1U,  /* single channel */
                           16U, /* 16 sample bits */
                           static_cast<uint32_t>(kAudioSampleFrequency));
  if (ret != 0) {
    return ret;
  }

  ret = AudioDrv_SetBuf(AUDIO_DRV_INTERFACE_RX,
                        g_audio_buf, AUDIO_BLOCK_NUM, AUDIO_BLOCK_SIZE);
  if (ret != 0) {
    return ret;
  }

  ret = AudioDrv_Control(AUDIO_DRV_CONTROL_RX_ENABLE);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter,
                             int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {

  if (!g_is_audio_initialized) {
    int32_t ret = AudioDrv_Setup();
    if (ret != 0) {
      return kTfLiteError;
    }
    g_is_audio_initialized = true;
  }

  const int start_sample     = (start_ms    * kAudioSampleFrequency) / 1000;
  const int duration_samples = (duration_ms * kAudioSampleFrequency) / 1000;
  for (int i = 0; i < duration_samples; ++i) {
    const int sample_index = (start_sample + i) % (AUDIO_BUFFER_SIZE/2);
    g_audio_data[i] = g_audio_buf[sample_index];
  }
  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_audio_data;

  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
  return g_latest_audio_timestamp;
}
