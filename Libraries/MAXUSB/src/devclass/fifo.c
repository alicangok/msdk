/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 * (now owned by Analog Devices, Inc.),
 * Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 * is proprietary to Analog Devices, Inc. and its licensors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <stdint.h>
#include "fifo.h"
#include "usb_hwopt.h"
#include "mxc_sys.h"

#if defined ( __ICCARM__ ) || defined( __GNUC__ ) || defined ( __CC_ARM )
#include <mxc_device.h>
#else
#include <inmaxq.h>
#endif

/****************************************************************************/
void fifo_init(fifo_t * fifo, void * mem, unsigned int length)
{
  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  fifo->rindex = 0;
  fifo->windex = 0;
  fifo->data = mem;
  fifo->length = length;

  MXC_SYS_Crit_Exit();
}

/****************************************************************************/
int fifo_put8(fifo_t * fifo, uint8_t element)
{
  // Check if FIFO is full
  if ( (fifo->windex == (fifo->rindex - 1)) ||
       ((fifo->rindex == 0) && (fifo->windex == (fifo->length - 1))) ) {
    return -1;
  }

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  // Put data into FIFO
  ((uint8_t*)(fifo->data))[fifo->windex] = element;

  // Increment pointer
  fifo->windex++;
  if (fifo->windex == fifo->length) {
    fifo->windex = 0;
  }

  MXC_SYS_Crit_Exit();

  return 0;
}

/****************************************************************************/
int fifo_get8(fifo_t * fifo, uint8_t * element)
{
  // Check if FIFO is empty
  if (fifo->rindex == fifo->windex)
    return -1;

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  //Get data from FIFO
  *element = ((uint8_t*)(fifo->data))[fifo->rindex];

  // Increment pointer
  fifo->rindex++;
  if (fifo->rindex == fifo->length) {
    fifo->rindex = 0;
  }

  MXC_SYS_Crit_Exit();

  return 0;
}

/****************************************************************************/
int fifo_put16(fifo_t * fifo, uint16_t element)
{
  // Check if FIFO is full
  if ( (fifo->windex == (fifo->rindex - 1)) ||
       ((fifo->rindex == 0) && (fifo->windex == (fifo->length - 1))) ) {
    return -1;
  }

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  // Put data into FIFO
  ((uint16_t*)(fifo->data))[fifo->windex] = element;

  // Increment pointer
  fifo->windex++;
  if (fifo->windex == fifo->length) {
    fifo->windex = 0;
  }

  MXC_SYS_Crit_Exit();

  return 0;
}

/****************************************************************************/
int fifo_get16(fifo_t * fifo, uint16_t * element)
{
  // Check if FIFO is empty
  if (fifo->rindex == fifo->windex)
    return -1;

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  // Get data from FIFO
  *element = ((uint16_t*)(fifo->data))[fifo->rindex];

  // Increment pointer
  fifo->rindex++;
  if (fifo->rindex == fifo->length) {
    fifo->rindex = 0;
  }

  MXC_SYS_Crit_Exit();

  return 0;
}

/****************************************************************************/
void fifo_clear(fifo_t * fifo)
{
  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  fifo->rindex = 0;
  fifo->windex = 0;

  MXC_SYS_Crit_Exit();
}

/****************************************************************************/
int fifo_empty(fifo_t * fifo)
{
  return (fifo->rindex == fifo->windex);
}

/****************************************************************************/
int fifo_full(fifo_t * fifo)
{
  int retval;

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  retval = ( (fifo->windex == (fifo->rindex - 1)) || ((fifo->rindex == 0) && (fifo->windex == (fifo->length - 1))) );

  MXC_SYS_Crit_Exit();

  return retval;
}

/****************************************************************************/
unsigned int fifo_level(fifo_t * fifo)
{
  uint16_t value;

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  if (fifo->windex >= fifo->rindex) {
    value = fifo->windex - fifo->rindex;
  } else {
    value = fifo->length - fifo->rindex + fifo->windex;
  }

  MXC_SYS_Crit_Exit();

  return value;
}

/****************************************************************************/
unsigned int fifo_remaining(fifo_t * fifo)
{
  uint16_t value;

  // atomic FIFO access
  MXC_SYS_Crit_Enter();

  if (fifo->rindex > fifo->windex) {
    value = fifo->rindex - fifo->windex - 1;
  } else {
    value = fifo->length - fifo->windex + fifo->rindex - 1;
  }

  MXC_SYS_Crit_Exit();

  return value;
}
