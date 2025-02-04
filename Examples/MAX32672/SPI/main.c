/**
 * @file    main.c
 * @brief   SPI Master Demo
 * @details Shows Master loopback demo for QSPI0
 *          Read the printf() for instructions
 */

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

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "nvic_table.h"
#include "uart.h"
#include "spi.h"
#include "dma.h"

/***** Preprocessors *****/
#define MASTERSYNC 1
#define MASTERASYNC 0
#define MASTERDMA 0

#if (!(MASTERSYNC || MASTERASYNC || MASTERDMA))
#error "You must set either MASTERSYNC or MASTERASYNC or MASTERDMA to 1."
#endif
#if ((MASTERSYNC && MASTERASYNC) || (MASTERASYNC && MASTERDMA) || (MASTERDMA && MASTERSYNC))
#error "You must select either MASTERSYNC or MASTERASYNC or MASTERDMA, not all 3."
#endif

/***** Definitions *****/
#define DATA_LEN 100 // Words
#define DATA_VALUE 0xA5A5 // This is for master mode only...
#define VALUE 0xFFFF
#define SPI_SPEED 100000 // Bit Rate

#define SPI MXC_SPI1
#define SPI_IRQ SPI1_IRQn

/***** Globals *****/
uint16_t rx_data[DATA_LEN];
uint16_t tx_data[DATA_LEN];
volatile int SPI_FLAG;
volatile uint8_t DMA0_FLAG, DMA1_FLAG;

/***** Functions *****/
#if MASTERASYNC
void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);
}
#endif // MASTERASYNC

#if MASTERDMA
void DMA0_IRQHandler(void)
{
    MXC_DMA_Handler();
    DMA0_FLAG = 1;
}

void DMA1_IRQHandler(void)
{
    MXC_DMA_Handler();
    DMA1_FLAG = 1;
}
#endif // MASTERDMA

void SPI_Callback(mxc_spi_req_t *req, int error)
{
    SPI_FLAG = error;
}

int main(void)
{
    int i, j, retVal;
    uint16_t temp;
    mxc_spi_req_t req;

    printf("\n**************************** SPI MASTER TEST *************************\n");
    printf("This example configures the SPI to send data between the MISO (P0.14 - AIN6) and\n");
    printf("MOSI (P0.15 - AIN7) pins.  Connect these two pins together.  This demo shows SPI\n");
    printf("sending different bit sizes each run through. \n");

    printf("\nThis demo shows Asynchronous, Synchronous and DMA transaction for SPI1\n");

    // Initialize the peripheral
    retVal = MXC_SPI_Init(SPI, 1, 0, 1, 0, SPI_SPEED);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI INITIALIZATION ERROR\n");
        return retVal;
    }

    // Set SPI bus width
    retVal = MXC_SPI_SetWidth(SPI, SPI_WIDTH_STANDARD);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI SET WIDTH ERROR: %d\n", retVal);
        return retVal;
    }

    // Run sample SPI transaction for character sizes 2-16
    for (i = 2; i < 17; i++) {
        // Initialize Data Buffers
        for (j = 0; j < DATA_LEN; j++) {
            tx_data[j] = DATA_VALUE;
        }
        memset(rx_data, 0x0, DATA_LEN * sizeof(uint16_t));

        // Set next character size to test
        retVal = MXC_SPI_SetDataSize(SPI, i);
        if (retVal != E_NO_ERROR) {
            printf("\nSPI SET DATASIZE ERROR: %d\n", retVal);
            return retVal;
        }

        // Setup SPI Transaction Request
        req.spi = SPI;
        req.txData = (uint8_t *)tx_data;
        req.rxData = (uint8_t *)rx_data;
        req.txLen = DATA_LEN;
        req.rxLen = DATA_LEN;
        req.ssIdx = 0;
        req.ssDeassert = 1;
        req.txCnt = 0;
        req.rxCnt = 0;
        req.completeCB = (spi_complete_cb_t)SPI_Callback;
        SPI_FLAG = 1;

#if MASTERSYNC
        MXC_SPI_MasterTransaction(&req);
#endif

#if MASTERASYNC
        MXC_NVIC_SetVector(SPI_IRQ, SPI_IRQHandler);
        NVIC_EnableIRQ(SPI_IRQ);
        MXC_SPI_MasterTransactionAsync(&req);

        while (SPI_FLAG == 1) {}

#endif

#if MASTERDMA
        MXC_DMA_ReleaseChannel(0);
        MXC_DMA_ReleaseChannel(1);

        DMA0_FLAG = 0;
        DMA1_FLAG = 0;

        NVIC_EnableIRQ(DMA0_IRQn);
        NVIC_EnableIRQ(DMA1_IRQn);
        MXC_SPI_MasterTransactionDMA(&req);

        while (DMA0_FLAG == 0 || DMA1_FLAG == 0) {}
#endif

        uint8_t bits = MXC_SPI_GetDataSize(SPI);

        for (j = 0; j < DATA_LEN; j++) {
            if (bits <= 8) {
                if (j < (DATA_LEN / 2)) {
                    temp = VALUE >> (16 - bits);
                    temp = (temp << 8) | temp;
                    temp &= DATA_VALUE;
                    tx_data[j] = temp;
                } else if (j == (DATA_LEN / 2) && DATA_LEN % 2 == 1) {
                    temp = VALUE >> (16 - bits);
                    temp &= DATA_VALUE;
                    tx_data[j] = temp;
                } else {
                    tx_data[j] = 0x0000;
                }
            } else {
                temp = VALUE >> (16 - bits);
                temp &= DATA_VALUE;
                tx_data[j] = temp;
            }
        }

        // Compare Sent data vs Received data
        if (memcmp(rx_data, tx_data, sizeof(tx_data)) != 0) {
            printf("\n-->%2d Bits Transaction Failed\n", i);
            return E_BAD_STATE;
        } else {
            printf("\n-->%2d Bits Transaction Successful\n", i);
        }
    }

    retVal = MXC_SPI_Shutdown(SPI);
    if (retVal != E_NO_ERROR) {
        printf("\n-->SPI SHUTDOWN ERROR: %d\n", retVal);
        return retVal;
    }

    return E_NO_ERROR;
}
