/**
 * @file       spimss.c
 * @brief      This file contains the function implementations for the
 *             Serial Peripheral Interface (SPIMSS) peripheral module.
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

/* **** Includes **** */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_assert.h"
#include "mxc_sys.h"
#include "spimss_reva.h"
#include "mxc_lock.h"

/**
 * @ingroup spimss
 * @{
 */

/* **** Definitions **** */

/* **** Globals **** */
typedef struct {
    spimss_reva_req_t *req;
} spimss_reva_req_state_t;

static spimss_reva_req_state_t states[MXC_SPIMSS_INSTANCES];

/* **** Functions **** */
static int MXC_SPIMSS_RevA_TransSetup(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req,
                                      int master);
static uint32_t MXC_SPIMSS_RevA_MasterTransHandler(mxc_spimss_reva_regs_t *spi,
                                                   spimss_reva_req_t *req);
static uint32_t MXC_SPIMSS_RevA_TransHandler(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req);
static uint32_t MXC_SPIMSS_RevA_SlaveTransHandler(mxc_spimss_reva_regs_t *spi,
                                                  spimss_reva_req_t *req);

/* ************************************************************************** */
int MXC_SPIMSS_RevA_Init(mxc_spimss_reva_regs_t *spi, unsigned mode, unsigned freq)
{
    int spi_num;
    unsigned int spimss_clk;
    unsigned int pol, pha; // Polarity and phase of the clock (SPI mode)

    spi_num = MXC_SPIMSS_GET_IDX((mxc_spimss_regs_t *)spi);
    states[spi_num].req = NULL;
    spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_ENABLE); // Keep the SPI Disabled (This is the SPI Start)

    // Set the bit rate
    spimss_clk = PeripheralClock;
    spi->brg = (spimss_clk / freq) >> 1;

    // Set the mode
    pol = mode >> 1; // Get the polarity out of the mode input value
    pha = mode & 1; // Get the phase out of the mode input value

    spi->ctrl = (spi->ctrl & ~(MXC_F_SPIMSS_REVA_CTRL_CLKPOL)) |
                (pol << MXC_F_SPIMSS_REVA_CTRL_CLKPOL_POS); // polarity

    spi->ctrl = (spi->ctrl & ~(MXC_F_SPIMSS_REVA_CTRL_PHASE)) |
                (pha << MXC_F_SPIMSS_REVA_CTRL_PHASE_POS); // phase

    spi->int_fl &= ~(MXC_F_SPIMSS_REVA_INT_FL_IRQ);

    return E_NO_ERROR;
}
/* ************************************************************************* */
int MXC_SPIMSS_RevA_Shutdown(mxc_spimss_reva_regs_t *spi)
{
    int spi_num;
    spimss_reva_req_t *temp_req;

    // Disable and turn off the SPI transaction.
    spi->ctrl = 0; // Interrupts, SPI transaction all turned off
    spi->int_fl = 0;
    spi->mode = 0;

    // Reset FIFO counters
    spi->dma &= ~(MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT | MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT);

    // Call all of the pending callbacks for this SPI
    spi_num = MXC_SPIMSS_GET_IDX((mxc_spimss_regs_t *)spi);
    if (states[spi_num].req != NULL) {
        // Save the request
        temp_req = states[spi_num].req;

        // Unlock this SPI
        MXC_FreeLock((uint32_t *)&states[spi_num].req);

        // Callback if not NULL
        if (temp_req->callback != NULL) {
            temp_req->callback(temp_req, E_SHUTDOWN);
        }
    }

    spi->int_fl = 0;

    return E_NO_ERROR;
}

/* ************************************************************************** */
int MXC_SPIMSS_RevA_TransSetup(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req, int master)
{
    int spi_num;

    spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_ENABLE); // Make sure the Initiation
        // of SPI Start is disabled.

    spi->mode |= MXC_F_SPIMSS_REVA_MODE_TX_LJ; // Making sure data is left
        // justified.

    if ((req->tx_data == NULL) && (req->rx_data == NULL)) {
        return -1;
    }

    spi_num = MXC_SPIMSS_GET_IDX((mxc_spimss_regs_t *)spi);
    MXC_ASSERT(spi_num >= 0);

    if (req->len == 0) {
        return 0;
    }

    req->tx_num = 0;
    req->rx_num = 0;

    if (MXC_GetLock((uint32_t *)&states[spi_num].req, (uint32_t)req) != E_NO_ERROR) {
        return E_BUSY;
    }

    if (master) { // Enable master mode
        spi->ctrl |= MXC_F_SPIMSS_REVA_CTRL_MMEN; // SPI configured as master.
        spi->mode |= MXC_F_SPIMSS_REVA_CTRL_MMEN; // SSEL pin is an output.
    } else { // Enable slave mode
        spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_MMEN); // SPI configured as slave.
        spi->mode &= ~(MXC_F_SPIMSS_REVA_CTRL_MMEN); // SSEL pin is an input.
    }

    // Setup the character size

    if (req->bits < 16) {
        MXC_SETFIELD(spi->mode, MXC_F_SPIMSS_REVA_MODE_NUMBITS,
                     req->bits << MXC_F_SPIMSS_REVA_MODE_NUMBITS_POS);

    } else {
        MXC_SETFIELD(spi->mode, MXC_F_SPIMSS_REVA_MODE_NUMBITS,
                     0 << MXC_F_SPIMSS_REVA_MODE_NUMBITS_POS);
    }

    // Setup the slave select
    spi->mode |= MXC_F_SPIMSS_REVA_MODE_SSV; // Assert a high on Slave Select,
        // to get the line ready for active low later

    // Clear the TX and RX FIFO
    spi->dma |= (MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CLR | MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CLR);

    return E_NO_ERROR;
}

/* ************************************************************************** */
void MXC_SPIMSS_RevA_Handler(mxc_spimss_reva_regs_t *spi) // From the IRQ
{
    int spi_num;
    uint32_t flags;
    unsigned int int_enable;

    flags = spi->int_fl;
    spi->int_fl = flags;
    spi->int_fl |= 0x80; // clear interrupt

    spi_num = MXC_SPIMSS_GET_IDX((mxc_spimss_regs_t *)spi);

    int_enable = 0;
    if (states[spi_num].req != NULL) {
        if ((spi->ctrl & MXC_F_SPIMSS_REVA_CTRL_MMEN) >> MXC_F_SPIMSS_REVA_CTRL_MMEN_POS) {
            int_enable = MXC_SPIMSS_RevA_MasterTransHandler(spi, states[spi_num].req);

        } else {
            int_enable = MXC_SPIMSS_RevA_SlaveTransHandler(spi, states[spi_num].req);
        }
    }

    if (int_enable == 1) {
        spi->ctrl |= (MXC_F_SPIMSS_REVA_CTRL_IRQE);
    }
}

/* ************************************************************************** */
int MXC_SPIMSS_RevA_MasterTrans(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    int error;

    if ((error = MXC_SPIMSS_RevA_TransSetup(spi, req, 1)) != E_NO_ERROR) {
        return error;
    }

    req->callback = NULL;

    spi->mode &= ~(MXC_F_SPIMSS_REVA_MODE_SSV); // This will assert the Slave Select.
    spi->ctrl |= MXC_F_SPIMSS_REVA_CTRL_ENABLE; // Enable/Start SPI

    while (MXC_SPIMSS_RevA_MasterTransHandler(spi, req) != 0) {}

    spi->mode |= MXC_F_SPIMSS_REVA_MODE_SSV;

    spi->ctrl &=
        ~(MXC_F_SPIMSS_REVA_CTRL_ENABLE); // Last of the SPIMSS value has been transmitted...
    // stop the transmission...
    return E_NO_ERROR;
}

/* ************************************************************************** */
int MXC_SPIMSS_RevA_SlaveTrans(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    int error;

    if ((error = MXC_SPIMSS_RevA_TransSetup(spi, req, 0)) != E_NO_ERROR) {
        return error;
    }

    while (MXC_SPIMSS_RevA_SlaveTransHandler(spi, req) != 0) {
        spi->ctrl |= MXC_F_SPIMSS_REVA_CTRL_ENABLE; // Enable/Start SPI
        while ((spi->int_fl & MXC_F_SPIMSS_REVA_INT_FL_TXST) == MXC_F_SPIMSS_REVA_INT_FL_TXST) {}
    }

    spi->ctrl &=
        ~(MXC_F_SPIMSS_REVA_CTRL_ENABLE); // Last of the SPIMSS value has been transmitted...
    // stop the transmission...
    return E_NO_ERROR;
}

/* ************************************************************************** */
int MXC_SPIMSS_RevA_MasterTransAsync(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    int error;
    uint8_t int_enable;

    // Clear state for next transaction
    MXC_SPIMSS_AbortAsync((mxc_spimss_req_t *)req);

    if ((error = MXC_SPIMSS_RevA_TransSetup(spi, req, 1)) != E_NO_ERROR) {
        return error;
    }

    int_enable = MXC_SPIMSS_RevA_MasterTransHandler(spi, req);

    spi->mode ^= MXC_F_SPIMSS_REVA_MODE_SSV; // This will assert the Slave Select.

    spi->ctrl |= MXC_F_SPIMSS_REVA_CTRL_ENABLE; // Enable/Start SPI

    if (int_enable == 1) {
        spi->ctrl |= (MXC_F_SPIMSS_REVA_CTRL_IRQE | MXC_F_SPIMSS_REVA_CTRL_STR);
    }

    return E_NO_ERROR;
}

/* ************************************************************************** */
int MXC_SPIMSS_RevA_SlaveTransAsync(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    int error;
    uint8_t int_enable;
    if ((error = MXC_SPIMSS_RevA_TransSetup(spi, req, 0)) != E_NO_ERROR) {
        return error;
    }

    int_enable = MXC_SPIMSS_RevA_SlaveTransHandler(spi, req);

    spi->ctrl |= MXC_F_SPIMSS_REVA_CTRL_ENABLE; // Enable/Start SPI

    if (int_enable == 1) { // Trigger a SPI Interrupt
        spi->ctrl |= (MXC_F_SPIMSS_REVA_CTRL_IRQE);
    }

    return E_NO_ERROR;
}

/* ************************************************************************** */
uint32_t MXC_SPIMSS_RevA_MasterTransHandler(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    unsigned start_set = 0;
    uint32_t retval;

    if (!start_set) {
        start_set = 1;
        retval = MXC_SPIMSS_RevA_TransHandler(spi, req);
    }

    return retval;
}

/* ************************************************************************** */
uint32_t MXC_SPIMSS_RevA_SlaveTransHandler(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    return MXC_SPIMSS_RevA_TransHandler(spi, req);
}

/* ************************************************************************** */
uint32_t MXC_SPIMSS_RevA_TransHandler(mxc_spimss_reva_regs_t *spi, spimss_reva_req_t *req)
{
    unsigned tx_avail, rx_avail;
    int remain, spi_num;
    uint32_t int_en = 0;
    uint32_t length = req->len;

    spi_num = MXC_SPIMSS_GET_IDX((mxc_spimss_regs_t *)spi);

    // Read the RX FIFO
    if (req->rx_data != NULL) {
        // Wait for there to be data in the RX FIFO
        rx_avail = ((spi->dma & MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT) >>
                    MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT_POS);
        if ((length - req->rx_num) < rx_avail) {
            rx_avail = (length - req->rx_num);
        }

        // Read from the FIFO
        while (rx_avail) {
            // Don't read less than 2 bytes if we are using greater than 8 bit characters
            if (req->bits > 8) {
                ((uint16_t *)req->rx_data)[req->rx_num++] = spi->data;
                rx_avail -= 1;

            } else {
                ((uint8_t *)req->rx_data)[req->rx_num++] = spi->data;
                rx_avail -= 1;
            }
            rx_avail = ((spi->dma & MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT) >>
                        MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT_POS);
            if ((length - req->rx_num) < rx_avail) {
                rx_avail = (length - req->rx_num);
            }
        }

        remain = length - req->rx_num;

        if (remain) {
            if (remain > MXC_SPIMSS_FIFO_DEPTH) {
                spi->dma = ((spi->dma & ~MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT) |
                            ((2) << MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT_POS));
            } else {
                spi->dma = ((spi->dma & ~MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT) |
                            ((remain - 1) << MXC_F_SPIMSS_REVA_DMA_RX_FIFO_CNT_POS));
            }

            int_en = 1;
        }

        // Break out if we've received all the bytes and we're not transmitting
        if ((req->tx_data == NULL) && (req->rx_num == length)) {
            spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_IRQE | MXC_F_SPIMSS_REVA_CTRL_STR);
            int_en = 0;
            MXC_FreeLock((uint32_t *)&states[spi_num].req);
            // Callback if not NULL
            if (req->callback != NULL) {
                req->callback(req, E_NO_ERROR);
            }
        }
    }
    // Note:- spi->dma shows the FIFO TX count and FIFO RX count in
    // Words, while the calculation below is in bytes.
    if (req->tx_data != NULL) {
        if (req->tx_num < length) {
            // Calculate how many bytes we can write to the FIFO (tx_avail holds that value)
            tx_avail =
                MXC_SPIMSS_FIFO_DEPTH - (((spi->dma & MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT) >>
                                          MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT_POS)); // in bytes

            if ((length - req->tx_num) < tx_avail) {
                tx_avail = (length - req->tx_num); // This is for the last spin
            }
            if (req->bits > 8) {
                tx_avail &= ~(unsigned)0x1;
            }
            // Write the FIFO
            while (tx_avail) {
                if (req->bits > 8) {
                    spi->data = ((uint16_t *)req->tx_data)[req->tx_num++];

                    tx_avail -= 1;
                } else {
                    spi->data = ((uint8_t *)req->tx_data)[req->tx_num++];
                    tx_avail -= 1;
                }
            }
        }

        remain = length - req->tx_num;

        // If there are values remaining to be transmitted, this portion will get
        // executed and int_en set, to indicate that this must spin and come back again...
        if (remain) {
            if (remain >
                MXC_SPIMSS_FIFO_DEPTH) { //  more tx rounds will happen... Transfer the maximum,
                spi->dma = ((spi->dma & ~MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT) |
                            ((MXC_SPIMSS_FIFO_DEPTH) << MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT_POS));
            } else { // only one more tx round will be done... Transfer whatever remains,
                spi->dma = ((spi->dma & ~MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT) |
                            ((remain) << MXC_F_SPIMSS_REVA_DMA_TX_FIFO_CNT_POS));
            }
            int_en = 1; // This will act as a trigger for the next round...
        }

        // Break out if we've transmitted all the bytes and not receiving
        if ((req->rx_data == NULL) && (req->tx_num == length)) {
            spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_IRQE | MXC_F_SPIMSS_REVA_CTRL_STR);
            int_en = 0;
            MXC_FreeLock((uint32_t *)&states[spi_num].req);
            // Callback if not NULL
            if (req->callback != NULL) {
                req->callback(req, E_NO_ERROR);
            }
        }
    }

    // Break out once we've transmitted and received all of the data
    if ((req->rx_num == length) && (req->tx_num == length)) {
        spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_IRQE | MXC_F_SPIMSS_REVA_CTRL_STR);
        int_en = 0;
        MXC_FreeLock((uint32_t *)&states[spi_num].req);
        // Callback if not NULL
        if (req->callback != NULL) {
            req->callback(req, E_NO_ERROR);
        }
    }

    return int_en;
}

/* ************************************************************************* */
int MXC_SPIMSS_RevA_AbortAsync(spimss_reva_req_t *req)
{
    int spi_num;
    mxc_spimss_reva_regs_t *spi;

    // Check the input parameters
    if (req == NULL) {
        return E_BAD_PARAM;
    }

    // Find the request, set to NULL
    for (spi_num = 0; spi_num < MXC_SPIMSS_INSTANCES; spi_num++) {
        if (req == states[spi_num].req) {
            spi = (mxc_spimss_reva_regs_t *)MXC_SPIMSS_GET_SPI(spi_num);

            // Disable interrupts, clear the flags
            spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_IRQE | MXC_F_SPIMSS_REVA_CTRL_STR);

            // Disable and turn off the SPI transaction.
            spi->ctrl &= ~(MXC_F_SPIMSS_REVA_CTRL_ENABLE);

            // Unlock this SPI
            MXC_FreeLock((uint32_t *)&states[spi_num].req);

            // Callback if not NULL
            if (req->callback != NULL) {
                req->callback(req, E_ABORT);
            }
            return E_NO_ERROR;
        }
    }

    return E_BAD_PARAM;
}
/**@} end of group spimss */
