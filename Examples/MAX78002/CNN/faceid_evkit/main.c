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
 * @brief   FaceID EvKit Demo
 *
 * @details
 *
 */

#define S_MODULE_NAME "main"

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "board.h"
#include "mxc.h"
//#include "mxc_device.h"
#include "mxc_delay.h"
#include "camera.h"
#include "state.h"
#include "icc.h"
#include "rtc.h"
#include "lp.h"
#include "cnn.h"

#include "MAXCAM_Debug.h"
#include "faceID.h"
#include "embedding_process.h"
#include "keypad.h"

// *****************************************************************************
void WUT_IRQHandler()
{
    MXC_WUT_IntClear();
}

uint32_t ticks_1;
uint32_t ticks_2;
mxc_wut_cfg_t cfg;

int main(void)
{
    /* TFT_Demo Example */
    int key;
    State *state;
    int ret = 0;
    int slaveAddress;
    int id;
    int dma_channel;

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC0);

    /* Set system clock to 100 MHz */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    MXC_GCR->ipll_ctrl |= MXC_F_GCR_IPLL_CTRL_EN; // Enable IPLL
    SystemCoreClockUpdate();

    printf("Waiting...\n");

    // DO NOT DELETE THIS LINE:
    MXC_Delay(SEC(2)); // Let debugger interrupt if needed

    // Enable peripheral, enable CNN interrupt, turn on CNN clock
    // CNN clock: 50 MHz div 1
    cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_IPLL, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);

    cnn_init(); // Bring CNN state machine into consistent state
    cnn_load_weights(); // Load CNN kernels
    cnn_load_bias(); // Load CNN bias
    cnn_configure(); // Configure CNN state machine

    if (init_database() < 0) {
        PR_ERR("Could not initialize the database");
        return -1;
    }

    /* Initialize RTC */
    MXC_RTC_Init(0, 0);
    MXC_RTC_Start();

    // Initialize DMA for camera interface
    MXC_DMA_Init();
    dma_channel = MXC_DMA_AcquireChannel();

    PR_DEBUG("\n\nFaceID Evkit Demo\n");

    // Initialize the camera driver.
    camera_init(CAMERA_FREQ);

    // Obtain the I2C slave address of the camera.
    slaveAddress = camera_get_slave_address();
    PR_DEBUG("Camera I2C slave address is %02x\n", slaveAddress);

    // Obtain the product ID of the camera.
    ret = camera_get_product_id(&id);

    if (ret != STATUS_OK) {
        PR_ERR("Error returned from reading camera id. Error %d\n", ret);
        return -1;
    }

    PR_DEBUG("Camera Product ID is %04x\n", id);

    // Obtain the manufacture ID of the camera.
    ret = camera_get_manufacture_id(&id);

    if (ret != STATUS_OK) {
        PR_ERR("Error returned from reading camera id. Error %d\n", ret);
        return -1;
    }

    PR_DEBUG("Camera Manufacture ID is %04x\n", id);

    // Setup the camera image dimensions, pixel format and data acquiring details.
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, USE_DMA,
                       dma_channel);

    if (ret != STATUS_OK) {
        PR_ERR("Error returned from setting up camera. Error %d\n", ret);
        return -1;
    }

#ifdef TFT_ENABLE
    camera_write_reg(0x0c, 0x56); //camera vertical flip=0
    /* Initialize TFT display */
#ifdef TFT_ADAFRUIT
    MXC_TFT_Init(MXC_SPI0, 1, NULL, NULL);
#else
    MXC_TFT_Init(NULL, NULL);
#endif
    MXC_TFT_SetRotation(ROTATE_180);
    MXC_TFT_SetBackGroundColor(4);
    MXC_TFT_SetForeGroundColor(WHITE); // set font color to white
#endif

#ifdef TS_ENABLE
    /* Initialize Touch Screen controller */
    MXC_TS_Init();
    MXC_TS_Start();
#endif

    /* Display Home page */
    state_init();

#ifdef LP_MODE_ENABLE
    /* Get ticks based on milliseconds */
    MXC_WUT_GetTicks(500, MXC_WUT_UNIT_MILLISEC, &ticks_1);
    MXC_WUT_GetTicks(100, MXC_WUT_UNIT_MILLISEC, &ticks_2);
    /* Configure structure for one shot timer to trigger in a number of ticks */
    cfg.mode = MXC_WUT_MODE_ONESHOT;
    cfg.cmp_cnt = ticks_1;
    /* Init WakeUp Timer */
    MXC_WUT_Init(MXC_WUT_PRES_1);
    /* Config WakeUp Timer */
    MXC_WUT_Config(&cfg);
    /* Enable Alarm wakeup by WUT */
    MXC_LP_EnableWUTAlarmWakeup();
    /* Enable WakeUp Timer interrupt */
    NVIC_EnableIRQ(WUT_IRQn);
#endif

#ifndef TS_ENABLE
    key = KEY_1;
#endif

    while (1) { //TFT Demo
        /* Get current screen state */
        state = state_get_current();
#ifdef TS_ENABLE
        /* Check pressed touch screen key */
        key = MXC_TS_GetKey();
#endif

        if (key > 0) {
            state->prcss_key(key);
        }
    }

    return 0;
}
