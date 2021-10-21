/*---------------------------------------------------------------------------
 * Copyright (c) 2020-2021 Arm Limited (or its affiliates). All rights reserved.
 *
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

#include "main.h"
#include "main_functions.h"

#include "cmsis_os2.h"

static const osThreadAttr_t app_main_attr = {
  .stack_size = 4096U
};

static osThreadId_t app_main_tid;

#ifdef __EVENT_DRIVEN
void AudioRxEvent (void) {
  osThreadFlagsSet(app_main_tid, 0x00000001U);
}
#endif

/*---------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
static void app_main (void *argument) {
  (void)argument;

  setup();
  for (;;) {
    loop();
#ifdef __EVENT_DRIVEN
    osThreadFlagsWait(0x00000001U, osFlagsWaitAny, osWaitForever);
#endif
  }
}

/*---------------------------------------------------------------------------
 * Application initialization
 *---------------------------------------------------------------------------*/
void app_initialize (void) {
  app_main_tid = osThreadNew(app_main, NULL, &app_main_attr);
}
