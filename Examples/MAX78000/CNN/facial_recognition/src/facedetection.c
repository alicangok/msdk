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
#include <string.h>

#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc.h"
#include "utils.h"
#include "camera.h"
#include "facedetection.h"
#include "post_process.h"
#include "utils.h"
#include "MAXCAM_Debug.h"
#include "cnn_1.h"
#ifdef BOARD_FTHR_REVA
#include "tft_ili9341.h"
#endif
#include "led.h"
#include "lp.h"
#include "tft_utils.h"

#define S_MODULE_NAME "facedetection"
/************************************ VARIABLES ******************************/
volatile uint32_t cnn_time; // Stopwatch
int reload_facedet = 1;
extern int g_dma_channel_tft;
static void run_cnn_1(int x_offset, int y_offset);

int face_detection(void)
{
    // Capture the image
    camera_start_capture_image();
    /* Sleep until camera interrupt */
    MXC_LP_EnterSleepMode();

#define PRINT_TIME 1
#if (PRINT_TIME == 1)
    /* Get current time */
    uint32_t process_time = utils_get_time_ms();
    uint32_t total_time = utils_get_time_ms();
#endif

    if (reload_facedet) {
        // Power off CNN after unloading result to clear all CNN registers.
        // It's needed to load and run other CNN model
        cnn_disable();

        // Enable CNN peripheral, enable CNN interrupt, turn on CNN clock
        // CNN clock: 50 MHz div 1
        cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
        /* Configure CNN 1 to detect a face */
        cnn_1_init(); // Bring CNN state machine into consistent state
        cnn_1_load_weights(); // Load CNN kernels
        cnn_1_load_bias(); // Load CNN bias
        cnn_1_configure(); // Configure CNN state machine
    }

    /* Check for received image */
    if (camera_is_image_rcv()) {
#if (PRINT_TIME == 1)
        process_time = utils_get_time_ms();
#endif

#ifdef IMAGE_TO_UART
        break;
#endif

        /* Run CNN on time on original and shifted images */
        run_cnn_1(0, 0);

#if (PRINT_TIME == 1)
        PR_INFO("Process Time Total : %dms", utils_get_time_ms() - process_time);
#endif

#if (PRINT_TIME == 1)
        PR_INFO("Capture Time : %dms", process_time - total_time);
        PR_INFO("Total Time : %dms", utils_get_time_ms() - total_time);
        total_time = utils_get_time_ms();
#endif
    }

    return 0;
}

static void run_cnn_1(int x_offset, int y_offset)
{
    uint8_t *raw;
    uint32_t imgLen;
    uint32_t w, h;
    /* Get current time */
    uint32_t pass_time = 0;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

#ifdef TFT_ENABLE
#ifdef BOARD_FTHR_REVA
    pass_time = utils_get_time_ms();

    MXC_TFT_Stream(X_START, Y_START, IMAGE_XRES, IMAGE_YRES);

    setup_dma_tft((uint32_t *)raw);

    // Wait to complete image capture
    while (camera_is_image_rcv() == 0)
        ;

    // Send a first half of captured image to TFT (Max DMA = 32KB)
    start_tft_dma((uint32_t *)raw);
    // Wait for DMA to finish
    while ((MXC_DMA->ch[g_dma_channel_tft].status & MXC_F_DMA_STATUS_STATUS)) {
        ;
    }

    setup_dma_tft((uint32_t *)(raw + IMAGE_XRES * IMAGE_YRES));

    // Send a second half of captured image to TFT
    start_tft_dma((uint32_t *)(raw + IMAGE_XRES * IMAGE_YRES));
    PR_DEBUG("TFT time : %dms", utils_get_time_ms() - pass_time);
#endif
#endif

    cnn_start();

    //PR_INFO("CNN initialization time : %d", utils_get_time_ms() - pass_time);

    uint8_t *data = raw;

    pass_time = utils_get_time_ms();

    for (int i = y_offset; i < HEIGHT_DET + y_offset; i++) {
        data = raw + ((IMAGE_H - (WIDTH_DET)) / 2) * IMAGE_W * BYTE_PER_PIXEL;
        data += (((IMAGE_W - (HEIGHT_DET)) / 2) + i) * BYTE_PER_PIXEL;
        for (int j = x_offset; j < WIDTH_DET + x_offset; j++) {
            uint8_t ur, ug, ub;
            int8_t r, g, b;
            uint32_t number;

            ub = (uint8_t)(data[j * BYTE_PER_PIXEL * IMAGE_W + 1] << 3);
            ug = (uint8_t)((data[j * BYTE_PER_PIXEL * IMAGE_W] << 5) |
                           ((data[j * BYTE_PER_PIXEL * IMAGE_W + 1] & 0xE0) >> 3));
            ur = (uint8_t)(data[j * BYTE_PER_PIXEL * IMAGE_W] & 0xF8);
            b = ub - 128;
            g = ug - 128;
            r = ur - 128;

            // Loading data into the CNN fifo
            while (((*((volatile uint32_t *)0x50000004) & 1)) != 0)
                ; // Wait for FIFO 0

            number = 0x00FFFFFF & ((((uint8_t)b) << 16) | (((uint8_t)g) << 8) | ((uint8_t)r));

            *((volatile uint32_t *)0x50000008) = number; // Write FIFO 0
        }
    }

    int cnn_load_time = utils_get_time_ms() - pass_time;
    PR_DEBUG("FaceDet CNN load data time : %dms", cnn_load_time);

    pass_time = utils_get_time_ms();

    // Disable Deep Sleep mode
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    // CNN interrupt wakes up CPU from sleep mode
    while (cnn_time == 0) {
        asm volatile("wfi"); // Sleep and wait for CNN interrupt
    }

    PR_DEBUG("FaceDet CNN wait time : %dms", utils_get_time_ms() - pass_time);

    get_priors();
    localize_objects();
}
