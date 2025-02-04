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
#ifndef __CAMERA_UTIL_H__
#define __CAMERA_UTIL_H__

#define CAMERA_FREQ (8.330 * 1000 * 1000)

#define BYTE_PER_PIXEL 2

#if defined(CAMERA_OV7692)

#define IMAGE_XRES 352
#define IMAGE_YRES 352

#define STREAM_ENABLE // If enabled, camera is setup in streaming mode to send the image line by line
// to TFT, or serial port as they are captured. Otherwise, it buffers the entire image first and
// then sends to TFT or serial port.
// With serial port set at 900kbps, it can stream for up to 80x80 with OV5642 camera in stream mode.
// or 176x144 when stream mode is disabled.
// It can display on TFT up to 176x144 if stream mode is disabled, or 320x240 if enabled.
#endif

#define X_START 0
#define Y_START 0

#define TFT_W 320
#define TFT_H 240

//#define PATTERN_GEN //  Replaces captured image with internally generated pattern

//#define RGB565      // Enables RGB565 camera settings, otherwise RGB888

void process_img(void);
int initialize_camera(void);
void load_input_camera(void);
void run_camera(void);
void dump_cnn(void);
void display_camera(void);

#define tft_dma_display(x, y, w, h, data)                                     \
    MXC_TFT_Stream(x, y, w, h);                                               \
    setup_dma_tft((uint32_t *)data, w *h * 2);                                \
    start_tft_dma((uint32_t *)data, w *h * 2);                                \
    while ((MXC_DMA->ch[g_dma_channel_tft].status & MXC_F_DMA_STATUS_STATUS)) \
        ;
#endif
