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
#ifndef EXAMPLES_MAX78002_CSI2_SRC_SRAM_FASTSPI_H_
#define EXAMPLES_MAX78002_CSI2_SRC_SRAM_FASTSPI_H_

#include "fastspi_config.h"

static volatile bool g_tx_done = 0;
static volatile bool g_rx_done = 0;
static volatile bool g_master_done = 0;

static const mxc_gpio_cfg_t spi_ss_pin = { .port = SPI_SS_PORT,
                                           .mask = SPI_SS_PIN,
                                           .func = MXC_GPIO_FUNC_ALT1,
                                           .pad = MXC_GPIO_PAD_WEAK_PULL_UP,
                                           .vssel = MXC_GPIO_VSSEL_VDDIOH };

static const mxc_gpio_cfg_t spi_pins = { .port = SPI_PINS_PORT,
                                         .mask = SPI_PINS_MASK,
                                         .func = MXC_GPIO_FUNC_ALT1,
                                         .pad = MXC_GPIO_PAD_NONE,
                                         .vssel = MXC_GPIO_VSSEL_VDDIOH };

// TODO(Jake):  Generalize to multiple SPI instances
int spi_init();
int spi_transmit(uint8_t *src, uint32_t txlen, uint8_t *dest, uint32_t rxlen, bool deassert,
                 bool use_dma, bool block);

#endif // EXAMPLES_MAX78002_CSI2_SRC_SRAM_FASTSPI_H_
