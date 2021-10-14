/*---------------------------------------------------------------------------
 * Copyright (c) 2021 Arm Limited (or its affiliates).
 * All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include "audio_drv.h"

#include "fsl_sai_edma.h"

#include "fsl_wm8960.h"
#include "fsl_codec_common.h"
#include "fsl_codec_adapter.h"

#include "Config_Audio.h"
#include "Config_WM8960.h"

/* Definitions */
#define AUDIO_FLAGS_INIT      (1U << 0)
#define AUDIO_FLAGS_DMA_TX    (1U << 2)
#define AUDIO_FLAGS_DMA_RX    (1U << 3)

typedef struct audio_buf_s {
  void    *data;
  uint32_t block_num;
  uint32_t block_size;
  uint32_t block_idx;
} AUDIO_BUF;

typedef struct audio_cb_s {
  AudioDrv_Event_t  callback; /* Audio criver callback   */
  AudioDrv_Status_t status;   /* Audio driver status     */
  AUDIO_BUF tx_buf;           /* Transmit buffer info    */
  AUDIO_BUF rx_buf;           /* Receive buffer info     */
  uint32_t  rx_cnt;           /* Receiver block count    */
  uint32_t  tx_cnt;           /* Transmitter block count */
  uint8_t   flags;            /* Audio driver flags      */
} AUDIO_CB;

static void IRQ_Receive_Cb (I2S_Type *base, sai_handle_t *handle, status_t status, void *userData);
static void IRQ_Transmit_Cb (I2S_Type *base, sai_handle_t *handle, status_t status, void *userData);
static void DMA_Receive_Cb  (I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);
static void DMA_Transmit_Cb (I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);

static uint32_t GetSaiClockFreq (I2S_Type *sai_instance);
static int32_t Codec_Setup (uint32_t mclk_freq, uint32_t sample_rate, uint32_t bit_depth);
static int32_t TriggerReceive (uint32_t n);
static int32_t TriggerSend    (uint32_t n);
