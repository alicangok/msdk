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
 * @brief   MAX78000 Feather Facial Recognition Demo
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
#include "mxc_device.h"
#include "mxc_delay.h"
#include "camera.h"
#include "icc.h"
#include "rtc.h"
#include "cnn_1.h"
#ifdef BOARD_FTHR_REVA
#include "tft_ili9341.h"
#endif
#include "MAXCAM_Debug.h"
#include "facedetection.h"
#include "post_process.h"
#include "faceID.h"
#include "embedding_process.h"
#include "utils.h"

#define CONSOLE_BAUD 115200

extern void SD_Init(void);
extern volatile uint8_t face_detected;
extern int reload_faceid;
extern int reload_facedet;
extern int8_t prev_decision;
mxc_uart_regs_t *CommUart;
#ifdef TFT_ENABLE
area_t area = { 50, 290, 180, 30 };
#endif

// *****************************************************************************
int main(void)
{
    int ret = 0;
    int slaveAddress;
    int id;
    int dma_channel;
    int undetect_count = 0;
    mxc_uart_regs_t *ConsoleUart;

#ifdef BOARD_FTHR_REVA
    // Wait for PMIC 1.8V to become available, about 180ms after power up.
    MXC_Delay(200000);
    /* Enable camera power */
    Camera_Power(POWER_ON);
#endif
    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC0);

    /* Set system clock to 100 MHz */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    ConsoleUart = MXC_UART_GET_UART(CONSOLE_UART);

    if ((ret = MXC_UART_Init(ConsoleUart, CONSOLE_BAUD, MXC_UART_IBRO_CLK)) != E_NO_ERROR) {
        PR_ERR("UART1 Init Error: %d\n", ret);
        return ret;
    }

    PR_DEBUG("\n\nMAX78000 Feather Facial Recognition Demo\n");

    // Initialize FaceID embeddings database
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

    // double camera PCLK speed
    camera_write_reg(0x11, 0x80);

#ifdef ROTATE_FEATHER_BOARD
    camera_set_hmirror(0);
#else
    camera_set_vflip(1); // for DMA TFT
#endif

#ifdef TFT_ENABLE
#ifdef BOARD_FTHR_REVA
    /* Initialize TFT display */
    MXC_TFT_Init(MXC_SPI0, 1, NULL, NULL);
#ifdef ROTATE_FEATHER_BOARD
    MXC_TFT_SetRotation(ROTATE_0);
#else
    MXC_TFT_SetRotation(ROTATE_270); // for DMA TFT
#endif
    MXC_TFT_SetBackGroundColor(4);
    MXC_TFT_SetForeGroundColor(WHITE); // set font color to white
    MXC_TFT_Rectangle(X_START - 4, Y_START - 4, X_START + IMAGE_XRES + 4, Y_START + IMAGE_YRES + 4,
                      FRAME_GREEN);
#endif
#endif

    /* Initilize SD card */
    SD_Init();
    uint32_t t1 = utils_get_time_ms();

    while (1) {
        if (face_detected == 0) {
            // run face detection
            face_detection();
            undetect_count++;
        } else // face is detected
        {
            PR_DEBUG("Face Detected");
            // run face id
            face_id();
            undetect_count = 0;

            if (reload_faceid) {
                // redo face id
                face_detected = 0;
            }
            // reload weights for next face detection
            reload_facedet = 1;
        }

#ifdef TFT_ENABLE
        if (undetect_count > 5) {
            MXC_TFT_SetRotation(ROTATE_180);
            MXC_TFT_ClearArea(&area, 4);
            MXC_TFT_SetRotation(ROTATE_270);
            undetect_count = 0;
            prev_decision = -5;
        }
#endif

        PR_DEBUG("\nTotal time: %dms", utils_get_time_ms() - t1);
        t1 = utils_get_time_ms();
    }

    return 0;
}
