
#include "audio_drv.h"

#include "fsl_sai_edma.h"
#include "fsl_dmamux.h"

/* Config: SAI */
#define SAI_INSTANCE          SAI1
#define SAI_DATA_ORDER        kSAI_DataMSB    /* LSB or MSB transmitted first       */
#define SAI_SYNC_MODE_TX      kSAI_ModeAsync  /* Synchronous or Asynchronous        */
#define SAI_SYNC_MODE_RX      kSAI_ModeSync   /* Synchronous or Asynchronous        */
#define SAI_MODE_MASTERSLAVE  kSAI_Master     /* Master or Slave (BCLK, frame sync) */
#define SAI_MODE_MONO         kSAI_Stereo     /* Left or Right Channel              */

/* Config: DMA for SAI */
#define SAI_DMA_ENABLE_TX     1U
#define SAI_DMA_CHANNEL_TX    0U
#define SAI_DMA_SOURCE_TX     kDmaRequestMuxSai1Tx

#define SAI_DMA_ENABLE_RX     1U
#define SAI_DMA_CHANNEL_RX    1U
#define SAI_DMA_SOURCE_RX     kDmaRequestMuxSai1Rx
