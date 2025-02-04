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

/**
 * @file    main.c
 * @brief   read and write sdhc
 * @details This example uses the sdhc to read and write to an sdhc card
 * You must have an SD card inserted int the card slot on the bottome right corner of the board.
 * Also the VDDIOH needs to have a voltage of 3.3V. To do this connect J11 and disconnect J8.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "nvic_table.h"
#include "led.h"
#include "tmr.h"
#include "gpio.h"
#include "sdhc.h"
#include "sdhc_lib.h"

/***** Definitions *****/
#define BLOCK_SIZE 512
#define BLOCK_COUNT 1024
#define MULTI_BLOCK_COUNT 512

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/***** Globals *****/
__attribute__((aligned(4))) uint8_t array[BLOCK_SIZE]; //Array to hold data read and written to card
__attribute__((aligned(4)))
uint8_t marray[BLOCK_SIZE * MULTI_BLOCK_COUNT]; //Array to hold data read and written to card

volatile int sdhc_flag = 1;
mxc_gpio_cfg_t SDPowerEnablePin = { MXC_GPIO3, MXC_GPIO_PIN_5, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE,
                                    MXC_GPIO_VSSEL_VDDIO };

/******************************************************************************/
//sdhc callback from async functions
void sdhc_cb(int error)
{
    sdhc_flag = error;
}

/******************************************************************************/
// When interrupt fires
void SDHC_IRQHandler(void)
{
    MXC_SDHC_Lib_Async_Handler();
}

/******************************************************************************/
int check_data(uint8_t *x, uint8_t expected, unsigned int length)
{
    while (length--) {
        if (*x++ != expected) {
            return -1;
        }
    }

    return 0;
}

/******************************************************************************/
//Sends write async to card and then does a read async to see if the right data was written
int async_transactions(unsigned int width)
{
    // initialize array
    memset(array, 0xfa, BLOCK_SIZE);

    sdhc_flag = 1;
    if (MXC_SDHC_Lib_WriteAsync(0, array, 1, width, sdhc_cb) != E_NO_ERROR) {
        return -1;
    }

    /* Wait for write to complete */
    while (sdhc_flag == 1) {}

    if (sdhc_flag == E_NO_ERROR) {
        printf("non-blocking write ok\n");

    } else {
        printf("non-blocking write failed\n");
        return sdhc_flag;
    }

    /* Clear data before reading back */
    memset(array, 0, BLOCK_SIZE);

    sdhc_flag = 1;
    if (MXC_SDHC_Lib_ReadAsync(array, 0, 1, width, sdhc_cb) != E_NO_ERROR) {
        return -1;
    }

    /* Wait for read to complete */
    while (sdhc_flag == 1) {}

    if (sdhc_flag == E_NO_ERROR) {
        printf("non-blocking read ok\n");

    } else {
        printf("non-blocking read failed\n");
        return sdhc_flag;
    }
    return check_data(array, 0xfa, BLOCK_SIZE);
}

/******************************************************************************/
//erases card with blocking functions
int erase(unsigned int width)
{
    int error;

    memset(array, 0, BLOCK_SIZE);

    //Write data from array to the card
    if ((error = MXC_SDHC_Lib_Write(0, array, 1, width)) == E_NO_ERROR) {
        printf("blocking erase ok\n");

    } else {
        printf("blocking erase failed\n");
        return error;
    }

    memset(array, 1, BLOCK_SIZE);

    //Read data from card and store in array
    if ((error = MXC_SDHC_Lib_Read(array, 0, 1, width)) == E_NO_ERROR) {
        printf("blocking erase read ok\n");

    } else {
        printf("blocking erase read failed\n");
        return error;
    }
    return check_data(array, 0, BLOCK_SIZE);
}

/******************************************************************************/
//Write and then read what was written to card using blocking methods.
int blocking_transactions(unsigned int width)
{
    unsigned int card_block;
    int error;

    for (card_block = 0; card_block < BLOCK_COUNT; card_block++) {
        /* Write a pattern to SD Card, read it back, check it */
        memset(array, (0xAF + card_block) % 256, BLOCK_SIZE);

        if ((error = MXC_SDHC_Lib_Write(card_block, array, 1, width)) != E_NO_ERROR) {
            printf("blocking write failed %d at block %u\n", error, card_block);
            while (1) {}
            return error;
        }

        memset(array, 0, BLOCK_SIZE);

        if ((error = MXC_SDHC_Lib_Read(array, card_block, 1, width)) != E_NO_ERROR) {
            printf("blocking read failed %d at block %u\n", error, card_block);
            return error;
        }

        if (check_data(array, (0xAF + card_block) % 256, BLOCK_SIZE)) {
            printf("data compare failed at block %u\n", card_block);
            return -1;
        }
    }

    printf("blocking read/write ok\n");
    return 0;
}

int multi_block_check(unsigned int width)
{
    int i, error;

    for (i = 0; i < MULTI_BLOCK_COUNT; i++) {
        memset(marray + (i * BLOCK_SIZE), (0x88 + i) % 256, BLOCK_SIZE);
    }

    if ((error = MXC_SDHC_Lib_Write(0, marray, MULTI_BLOCK_COUNT, width)) != E_NO_ERROR) {
        printf("blocking write failed %d\n", error);
        return error;
    }

    memset(marray, 0, BLOCK_SIZE * MULTI_BLOCK_COUNT);

    if ((error = MXC_SDHC_Lib_Read(marray, 0, MULTI_BLOCK_COUNT, width)) != E_NO_ERROR) {
        printf("blocking read failed %d\n", error);
        return error;
    }

    for (i = 0; i < MULTI_BLOCK_COUNT; i++) {
        if (check_data(marray + (i * BLOCK_SIZE), (0x88 + i) % 256, BLOCK_SIZE)) {
            printf("data compare failed\n");
            return -1;
        }
    }

    return 0;
}

/******************************************************************************/
int main(void)
{
    int fail = 0;
    mxc_sdhc_cfg_t cfg;
    int result;

    printf("\n\n***** " TOSTRING(TARGET) " SDHC Example *****\n");

    // Turn on Power to Card
    MXC_GPIO_Config(&SDPowerEnablePin);
    MXC_GPIO_OutClr(SDPowerEnablePin.port, SDPowerEnablePin.mask);
    // Set up Interupt
    NVIC_EnableIRQ(SDHC_IRQn);
    MXC_NVIC_SetVector(SDHC_IRQn, SDHC_IRQHandler);

    // Initialize SDHC peripheral
    cfg.bus_voltage = MXC_SDHC_Bus_Voltage_3_3;
    cfg.block_gap = 0;
    cfg.clk_div =
        0x0B0; // Maximum divide ratio, frequency must be < 400 kHz during Card Identification phase (SD Specification Part 1 Ch 6.6.6)
    MXC_SDHC_Init(&cfg);

    // wait for card to be inserted
    printf("Waiting for card.\n");
    while (!MXC_SDHC_Card_Inserted()) {}
    printf("Card inserted.\n");

    // set up card to get it ready for a transaction
    if (MXC_SDHC_Lib_InitCard(10) == E_NO_ERROR) {
        printf("Card Initialized.\n");
    } else {
        printf("No card response!\n");
        fail += 1;
    }

    if (MXC_SDHC_Lib_Get_Card_Type() == CARD_SDHC) {
        printf("Card type: SDHC\n");
    } else {
        printf("Card type: MMC/eMMC\n");
    }

    /* Configure for fastest possible clock, must not exceed 52 MHz for eMMC */
    if (SystemCoreClock > 96000000) {
        printf("SD clock ratio (at card) 4:1\n");
        MXC_SDHC_Set_Clock_Config(1);
    } else {
        printf("SD clock ratio (at card) 2:1\n");
        MXC_SDHC_Set_Clock_Config(0);
    }

    /*** 1-bit data bus ***/
    printf("--> 1-bit data bus example <--\n");

    if ((result = blocking_transactions(MXC_SDHC_LIB_SINGLE_DATA)) != 0) {
        printf("blocking error %d\n", result);
        fail += 1;
    } else {
        printf("Passed blocking\n");
    }

    if ((result = erase(MXC_SDHC_LIB_SINGLE_DATA)) != 0) {
        printf("Erase failed %d\n", result);
        fail += 1;
    } else {
        printf("Passed erase\n");
    }

    if ((result = async_transactions(MXC_SDHC_LIB_SINGLE_DATA)) != 0) {
        printf("async error %d\n", result);
        fail += 1;
    } else {
        printf("Passed async\n");
    }

    /*** 4-bit data bus ***/
    printf("--> 4-bit data bus example <--\n");

    if ((result = blocking_transactions(MXC_SDHC_LIB_QUAD_DATA)) != 0) {
        printf("blocking error %d\n", result);
        fail += 1;
    } else {
        printf("Passed blocking\n");
    }

    if ((result = erase(MXC_SDHC_LIB_QUAD_DATA)) != 0) {
        printf("Erase failed %d\n", result);
        fail += 1;
    } else {
        printf("Passed erase\n");
    }

    if ((result = async_transactions(MXC_SDHC_LIB_QUAD_DATA)) != 0) {
        printf("async error %d\n", result);
        fail += 1;
    } else {
        printf("Passed async\n");
    }

    printf("--> Blocking, 4-bit data bus, multi-block example <--\n");

    if (multi_block_check(MXC_SDHC_LIB_QUAD_DATA)) {
        printf(" FAIL \n");
        fail += 1;
    } else {
        printf(" PASS \n");
    }

    if (fail != 0) {
        printf("\nExample Failed\n");
        return E_FAIL;
    }

    printf("\nExample Succeeded\n");
    return E_NO_ERROR;
}
