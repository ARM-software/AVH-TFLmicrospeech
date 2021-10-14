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

#include "Audio_IMXRT1050-EVKB.h"

/* DMA handles */
edma_handle_t DmaTxHandle = {0};
edma_handle_t DmaRxHandle = {0};

sai_edma_handle_t DmaSaiTxHandle;
sai_edma_handle_t DmaSaiRxHandle;

sai_handle_t SaiTxHandle = {0};
sai_handle_t SaiRxHandle = {0};

/* SAI transceiver config */
sai_transceiver_t SaiConfig;

/* Codec handle */
codec_handle_t CodecHandle;

static AUDIO_CB AudioCb = {0};

/**
  \fn          int32_t AudioDrv_Initialize (AudioDrv_Event_t cb_event)
  \brief       Initialize Audio Interface.
  \param[in]   cb_event Pointer to \ref AudioDrv_Event_t
  \return      return code
*/
int32_t AudioDrv_Initialize (AudioDrv_Event_t cb_event) {

  AudioCb.callback = cb_event;
  AudioCb.flags    = 0U;

  #if SAI_DMA_ENABLE_TX
    AudioCb.flags |= AUDIO_FLAGS_DMA_TX;
  #endif

  #if SAI_DMA_ENABLE_RX
    AudioCb.flags |= AUDIO_FLAGS_DMA_RX;
  #endif

  /* Initialize SAI */
  SAI_Init(SAI_INSTANCE);

  if ((AudioCb.flags & AUDIO_FLAGS_DMA_TX) != 0U) {
    /* Setup DMAMUX */
    DMAMUX_SetSource    (DMAMUX, SAI_DMA_CHANNEL_TX, (uint8_t)SAI_DMA_SOURCE_TX);
    DMAMUX_EnableChannel(DMAMUX, SAI_DMA_CHANNEL_TX);

    /* Initialize DMA handles */
    EDMA_CreateHandle(&DmaTxHandle, DMA0, SAI_DMA_CHANNEL_TX);

    SAI_TransferTxCreateHandleEDMA(SAI_INSTANCE, &DmaSaiTxHandle, DMA_Transmit_Cb, NULL, &DmaTxHandle);
  } else {
    SAI_TransferTxCreateHandle (SAI_INSTANCE, &SaiTxHandle, IRQ_Transmit_Cb, NULL);
  }

  if ((AudioCb.flags & AUDIO_FLAGS_DMA_RX) != 0U) {
    /* Setup DMAMUX */
    DMAMUX_SetSource    (DMAMUX, SAI_DMA_CHANNEL_RX, (uint8_t)SAI_DMA_SOURCE_RX);
    DMAMUX_EnableChannel(DMAMUX, SAI_DMA_CHANNEL_RX);

    /* Initialize DMA handles */
    EDMA_CreateHandle(&DmaRxHandle, DMA0, SAI_DMA_CHANNEL_RX);

    SAI_TransferRxCreateHandleEDMA(SAI_INSTANCE, &DmaSaiRxHandle, DMA_Receive_Cb,  NULL, &DmaRxHandle);
  } else {
    SAI_TransferRxCreateHandle (SAI_INSTANCE, &SaiRxHandle, IRQ_Receive_Cb, NULL);
  }

  /* Driver is initialized */
  AudioCb.flags |= AUDIO_FLAGS_INIT;

  return (AUDIO_DRV_OK);
}

/**
  \fn          int32_t AudioDrv_Uninitialize (void)
  \brief       De-initialize Audio Interface.
  \return      return code
*/
int32_t AudioDrv_Uninitialize (void) {
  int32_t status;

  if (AudioCb.status.tx_active == 1U) {
    /* Stop transmit DMA channel */
    SAI_TransferTerminateSendEDMA (SAI_INSTANCE, &DmaSaiTxHandle);
  }

  if (AudioCb.status.rx_active == 1U) {
    /* Stop receive DMA channel */
    SAI_TransferTerminateReceiveEDMA(SAI_INSTANCE, &DmaSaiRxHandle);
  }

  /* Clear all flags */
  AudioCb.flags = 0U;

  return (AUDIO_DRV_OK);
}

/**
  \fn          int32_t AudioDrv_Configure (uint32_t interface, uint32_t channels, uint32_t sample_bits, uint32_t sample_freq)
  \brief       Configure Audio Interface.
  \param[in]   interface   Audio Interface
  \param[in]   channels    Number of channels
  \param[in]   sample_bits Sample number of bits (8..32)
  \param[in]   sample_rate Sample rate (samples per second)
  \return      return code
*/
int32_t AudioDrv_Configure (uint32_t interface, uint32_t channels, uint32_t sample_bits, uint32_t sample_rate) {
  int32_t status;
  int32_t i;
  uint32_t clock_freq;
  sai_mono_stereo_t mode;

  if ((interface != AUDIO_DRV_INTERFACE_TX) && (interface != AUDIO_DRV_INTERFACE_RX)) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  if ((channels    <  1U) || (channels    > 32U) ||
      (sample_bits <  8U) || (sample_bits > 32U) ||
      (sample_rate == 0U)) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  if ((AudioCb.flags & AUDIO_FLAGS_INIT) == 0U) {
    /* Not initialized */
    return AUDIO_DRV_ERROR;
  }

  if (channels == 1U) {
    /* Set left or right mono mode */
    mode = SAI_MODE_MONO;
  }
  else if (channels == 2U) {
    /* Set stereo mode */
    mode = kSAI_Stereo;
  }
  else {
    return AUDIO_DRV_ERROR_UNSUPPORTED;
  }

    /* Set initial config */
  SAI_GetClassicI2SConfig(&SaiConfig, sample_bits, mode, 1U);

  if (interface == AUDIO_DRV_INTERFACE_RX) {
    /* Configure receiver */
    SaiConfig.syncMode    = SAI_SYNC_MODE_RX;
    SaiConfig.masterSlave = SAI_MODE_MASTERSLAVE;

    if ((AudioCb.flags & AUDIO_FLAGS_DMA_RX) != 0U) {
      SAI_TransferRxSetConfigEDMA (SAI_INSTANCE, &DmaSaiRxHandle, &SaiConfig);
    } else {
      SAI_TransferRxSetConfig (SAI_INSTANCE, &SaiRxHandle, &SaiConfig);
    }

    clock_freq = GetSaiClockFreq (SAI_INSTANCE);

    SAI_RxSetBitClockRate (SAI_INSTANCE, clock_freq, sample_rate, sample_bits, channels);
  //}
  //else /* if (interface == AUDIO_DRV_INTERFACE_TX) */ {
    /* Configure transmitter */
    SaiConfig.syncMode    = SAI_SYNC_MODE_TX;
    SaiConfig.masterSlave = SAI_MODE_MASTERSLAVE;

    if ((AudioCb.flags & AUDIO_FLAGS_DMA_TX) != 0U) {
      SAI_TransferTxSetConfigEDMA (SAI_INSTANCE, &DmaSaiTxHandle, &SaiConfig);
    } else {
      SAI_TransferTxSetConfig (SAI_INSTANCE, &SaiTxHandle, &SaiConfig);
    }

    clock_freq = GetSaiClockFreq (SAI_INSTANCE);

    SAI_TxSetBitClockRate (SAI_INSTANCE, clock_freq, sample_rate, sample_bits, channels);
  }

  /* Setup audio codec */
  status = Codec_Setup (clock_freq, sample_rate, sample_bits);

  return (status);
}

/**
  \fn          int32_t AudioDrv_SetBuf (uint32_t interface, void *buf, uint32_t block_count, uint32_t block_size)
  \brief       Set Audio Interface buffer.
  \param[in]   interface   Audio Interface
  \param[in]   buf         Pointer to buffer for audio data
  \param[in]   block_num   Number of blocks in buffer (must be 2^n)
  \param[in]   block_size  Block size in number of samples
  \return      return code
*/
int32_t AudioDrv_SetBuf (uint32_t interface, void *buf, uint32_t block_num, uint32_t block_size) {
  int32_t status;
  AUDIO_BUF *p;

  if ((interface != AUDIO_DRV_INTERFACE_TX) && (interface != AUDIO_DRV_INTERFACE_RX)) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  if (buf == NULL) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  status = AUDIO_DRV_OK;

  if (interface == AUDIO_DRV_INTERFACE_RX) {
    if (AudioCb.status.rx_active == 0U) {
      /* Set receive buffer */
      p = &AudioCb.rx_buf;

      p->data       = buf;
      p->block_num  = block_num;
      p->block_size = block_size;
    }
    else {
      status = AUDIO_DRV_ERROR_BUSY;
    }
  }
  else /* if (interface == AUDIO_DRV_INTERFACE_TX) */ {
    if (AudioCb.status.tx_active == 0U) {
      /* Set transmit buffer */
      p = &AudioCb.tx_buf;

      p->data       = buf;
      p->block_num  = block_num;
      p->block_size = block_size;
    }
    else {
      status = AUDIO_DRV_ERROR_BUSY;
    }
  }

  return (status);
}

/**
  \fn          int32_t AudioDrv_Control (uint32_t control)
  \brief       Control Audio Interface.
  \param[in]   control Operation
  \return      return code
*/
int32_t AudioDrv_Control (uint32_t control) {
  int32_t status;

  if ((AudioCb.flags & AUDIO_FLAGS_INIT) == 0U) {
    /* Not initialized */
    return AUDIO_DRV_ERROR;
  }

  if ((control & AUDIO_DRV_CONTROL_TX_ENABLE) != 0U) {
    AudioCb.status.tx_active = 1U;
    AudioCb.tx_cnt           = 0U;

    status = TriggerSend (AudioCb.tx_buf.block_num);
  }
  else if ((control & AUDIO_DRV_CONTROL_RX_ENABLE) != 0U) {
    AudioCb.status.rx_active = 1U;
    AudioCb.rx_cnt           = 0U;

    status = TriggerReceive (AudioCb.rx_buf.block_num);
  }
  else {
    status = AUDIO_DRV_OK;

    if ((control & AUDIO_DRV_CONTROL_TX_DISABLE) != 0U) {
      AudioCb.status.tx_active = 0U;

      SAI_TransferTerminateSendEDMA (SAI_INSTANCE, &DmaSaiTxHandle);
    }
    else if ((control & AUDIO_DRV_CONTROL_RX_DISABLE) != 0U) {
      AudioCb.status.rx_active = 0U;
      
      SAI_TransferTerminateReceiveEDMA (SAI_INSTANCE, &DmaSaiRxHandle);
    }
    else {
      status = AUDIO_DRV_ERROR_UNSUPPORTED;
    }
  }

  return (status);
}

/**
  \fn          uint32_t AudioDrv_GetTxCount (void)
  \brief       Get transmitted block count.
  \return      number of transmitted blocks
*/
uint32_t AudioDrv_GetTxCount (void) {
  return (AudioCb.tx_cnt);
}

/**
  \fn          uint32_t AudioDrv_GetRxCount (void)
  \brief       Get received block count.
  \return      number of received blocks
*/
uint32_t AudioDrv_GetRxCount (void) {
return (AudioCb.rx_cnt);
}

/**
  \fn          AudioDrv_Status_t AudioDrv_GetStatus (void)
  \brief       Get Audio Interface status.
  \return      \ref AudioDrv_Status_t
*/
AudioDrv_Status_t AudioDrv_GetStatus (void) {
  return (AudioCb.status);
}

/**
  IRQ receive callback
*/
static void IRQ_Receive_Cb (I2S_Type *base, sai_handle_t *handle, status_t status, void *userData) {

  if (status == kStatus_SAI_RxIdle) {
    /* Increment receiver block count */
    AudioCb.rx_cnt++;

    /* Set new receive buffer */
    TriggerReceive(1U);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback (AUDIO_DRV_EVENT_RX_DATA);
    }
  }
  else {
    // kStatus_SAI_QueueFull, kStatus_SAI_RxBusy, kStatus_SAI_RxError
  }
  
}

/**
  IRQ transmit callback
*/
static void IRQ_Transmit_Cb (I2S_Type *base, sai_handle_t *handle, status_t status, void *userData) {

  if (status == kStatus_SAI_TxIdle) {
    /* Increment transmitter block count */
    AudioCb.tx_cnt++;

    /* Set new transmit buffer */
    TriggerSend(1U);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback (AUDIO_DRV_EVENT_TX_DATA);
    }
  }
  else {
    // kStatus_SAI_QueueFull, kStatus_SAI_TxBusy, kStatus_SAI_TxError
  }
}

/**
  DMA receive callback
*/
uint32_t Rx_QueueFull = 0;
static void DMA_Receive_Cb (I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData) {

  if (status == kStatus_SAI_RxIdle) {
    /* Increment receiver block count */
    AudioCb.rx_cnt++;

    /* Set new receive buffer */
    TriggerReceive(1U);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback (AUDIO_DRV_EVENT_RX_DATA);
    }
  }
  else {
    // kStatus_SAI_QueueFull, kStatus_SAI_RxBusy, kStatus_SAI_RxError
    Rx_QueueFull++;
  }
}

/**
  DMA transmit callback
*/
uint32_t Tx_QueueFull = 0;
static void DMA_Transmit_Cb (I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData) {

  if (status == kStatus_SAI_TxIdle) {
    /* Increment transmitter block count */
    AudioCb.tx_cnt++;

    /* Set new transmit buffer */
    TriggerSend(1U);

    /* Call application callback function */
    if (AudioCb.callback != NULL) {
      AudioCb.callback (AUDIO_DRV_EVENT_TX_DATA);
    }
  }
  else {
    // kStatus_SAI_QueueFull, kStatus_SAI_TxBusy, kStatus_SAI_TxError
    Tx_QueueFull++;
  }
}

/**
  Get SAI instance clock frequency.
*/
static uint32_t GetSaiClockFreq (I2S_Type *sai_instance) {
  uint32_t clk;
  uint32_t clk_pred;
  uint32_t clk_podf;

  /* Get audio pll clk */
  clk = CLOCK_GetFreq (kCLOCK_AudioPllClk);

  if (sai_instance == SAI1) {
    /* Get clock pre-divider (1 - 8) */
    clk_pred = CLOCK_GetDiv (kCLOCK_Sai1PreDiv);
    /* Get clock divider (1 - 64) */
    clk_podf = CLOCK_GetDiv (kCLOCK_Sai1Div);
  }
  else {
    clk = 0U;
  }

  return (clk / (clk_pred + 1U) / (clk_podf + 1U));
}

/**
  Get I2C instance clock frequency.
*/
static uint32_t GetI2CClockFreq (LPI2C_Type *i2c_instance) {
  uint32_t clk;
  uint32_t clk_div;
  uint32_t clk_podf;

  /* Get USB1 pll clk */
  clk = CLOCK_GetFreq (kCLOCK_Usb1PllClk);

  /* Set static divider value */
  clk_div = 8U;

  if (i2c_instance == LPI2C1) {
    /* Get clock divider */
    clk_podf = CLOCK_GetDiv (kCLOCK_Lpi2cDiv);
  }
  else {
    clk = 0U;
  }

  return (clk / clk_div / (clk_podf + 1U));
}


/**
  Setup audio codec.
*/
static int32_t Codec_Setup (uint32_t mclk_freq, uint32_t sample_rate, uint32_t bit_depth) {
  int32_t status;
  wm8960_config_t cfg;
  codec_config_t codec_cfg;
  LPI2C_Type *i2c_instances[] = LPI2C_BASE_PTRS;
  LPI2C_Type *i2c;

  /* Set codec type and config structure */
  codec_cfg.codecDevType   = kCODEC_WM8960;
  codec_cfg.codecDevConfig = &cfg;
  
  /* Configure I2C */
  i2c = i2c_instances[WM8960_INSTANCE_I2C];

  cfg.i2cConfig.codecI2CSourceClock = GetI2CClockFreq(i2c);
  cfg.i2cConfig.codecI2CInstance    = WM8960_INSTANCE_I2C;
  cfg.slaveAddress                  = WM8960_ADDRESS_I2C;

  /* Configure SAI bus mode */
  cfg.bus          = WM8960_BUS_PROTOCOL;
  cfg.master_slave = WM8960_MODE_MASTER;

  /* Configure audio data route */
  cfg.route            = WM8960_DATA_ROUTE;
  cfg.leftInputSource  = WM8960_INPUT_LEFT;
  cfg.rightInputSource = WM8960_INPUT_RIGHT;
  cfg.playSource       = WM8960_MIXER_SOURCE;

  /* Configure audio data format */
  cfg.format.mclk_HZ    = mclk_freq;
  cfg.format.sampleRate = sample_rate;
  cfg.format.bitWidth   = bit_depth;

  /* Setup codec */
  if (CODEC_Init (&CodecHandle, &codec_cfg) == kStatus_Success) {
    status = AUDIO_DRV_OK;
  } else {
    /* Codec setup failed */
    status = AUDIO_DRV_ERROR;
  }

  return (status);
}

/**
  Enqueue data blocks for send transfer and start transferring.

  \param[in]  n   max number of blocks to enqueue for transfer
*/
static int32_t TriggerSend (uint32_t n) {
  status_t status;
  int32_t rval;
  sai_transfer_t xfer;
  AUDIO_BUF *buf = &AudioCb.tx_buf;

  while (n--) {
    xfer.data     = &buf->data[buf->block_size * buf->block_idx];
    xfer.dataSize = buf->block_size;

    buf->block_idx = (buf->block_idx + 1U) % buf->block_num;

    if (AudioCb.flags & AUDIO_FLAGS_DMA_TX) {
      /* Start DMA transfer */
      status = SAI_TransferSendEDMA(SAI_INSTANCE, &DmaSaiTxHandle, &xfer);
    }
    else {
      /* Start IRQ transfer */
      status = SAI_TransferSendNonBlocking(SAI_INSTANCE, &SaiTxHandle, &xfer);
    }

    if (status == kStatus_SAI_QueueFull) {
      status = kStatus_Success;

      break;
    }
  }

  if (status == kStatus_Success) {
    rval = AUDIO_DRV_OK;
  }
  else {
    rval = AUDIO_DRV_ERROR;

    AudioCb.status.tx_active = 0U;
  }

  return (rval);
}

/**
   Enqueue data blocks for receive transfer and start transferring.

  \param[in]  n   max number of blocks to receive
*/
static int32_t TriggerReceive (uint32_t n) {
  status_t status;
  int32_t rval;
  sai_transfer_t xfer;
  AUDIO_BUF *buf = &AudioCb.rx_buf;

  while (n--) {
    xfer.data     = &buf->data[buf->block_size * buf->block_idx];
    xfer.dataSize = buf->block_size;

    buf->block_idx = (buf->block_idx + 1U) % buf->block_num;

    if (AudioCb.flags & AUDIO_FLAGS_DMA_RX) {
      /* Start DMA transfer */
      status = SAI_TransferReceiveEDMA(SAI_INSTANCE, &DmaSaiRxHandle, &xfer);
    }
    else {
      /* Start IRQ transfer */
      status = SAI_TransferReceiveNonBlocking(SAI_INSTANCE, &SaiRxHandle, &xfer);
    }

    if (status == kStatus_SAI_QueueFull) {
      status = kStatus_Success;

      break;
    }
  }

  if (status == kStatus_Success) {
    rval = AUDIO_DRV_OK;
  }
  else {
    rval = AUDIO_DRV_ERROR;

    AudioCb.status.rx_active = 0U;
  }

  return (rval);
}
