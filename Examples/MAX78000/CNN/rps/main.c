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

// pt-rps82
// Created using ./ai8xize.py --verbose --log --test-dir pytorch --prefix pt-rps82 --checkpoint-file trained/ai85-rps82-chw.pth.tar --config-file networks/rps-chw.yaml --softmax --embedded-code --device MAX78000 --compact-data --mexpress --timer 0 --display-checkpoint --fifo

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mxc.h"
#include "cnn.h"
#include "sampledata.h"

volatile uint32_t cnn_time; // Stopwatch

void fail(void)
{
    printf("\n*** FAIL ***\n\n");
    while (1) {}
}

// Data input: CHW 3x64x64 (12288 bytes total / 4096 bytes per channel):
static const uint32_t input_0[] = SAMPLE_INPUT_0;
static const uint32_t input_1[] = SAMPLE_INPUT_1;
static const uint32_t input_2[] = SAMPLE_INPUT_2;
void load_input(void)
{
    // This function loads the sample data input -- replace with actual data

    int i;
    const uint32_t *in0 = input_0;
    const uint32_t *in1 = input_1;
    const uint32_t *in2 = input_2;

    for (i = 0; i < 1024; i++) {
        while (((*((volatile uint32_t *)0x50000004) & 1)) != 0) {}
        // Wait for FIFO 0
        *((volatile uint32_t *)0x50000008) = *in0++; // Write FIFO 0
        while (((*((volatile uint32_t *)0x50000004) & 2)) != 0) {}
        // Wait for FIFO 1
        *((volatile uint32_t *)0x5000000c) = *in1++; // Write FIFO 1
        while (((*((volatile uint32_t *)0x50000004) & 4)) != 0) {}
        // Wait for FIFO 2
        *((volatile uint32_t *)0x50000010) = *in2++; // Write FIFO 2
    }
}

// Expected output of layer 6 for pt-rps82 given the sample input
int check_output(void)
{
    if ((*((volatile uint32_t *)0x50401000)) != 0x00079c32)
        return CNN_FAIL; // 0,0,0
    if ((*((volatile uint32_t *)0x50401004)) != 0xfffae676)
        return CNN_FAIL; // 0,0,1
    if ((*((volatile uint32_t *)0x50401008)) != 0xfff657b6)
        return CNN_FAIL; // 0,0,2

    return CNN_OK;
}

// Classification layer:
static int32_t ml_data[CNN_NUM_OUTPUTS];
static q15_t ml_softmax[CNN_NUM_OUTPUTS];

void softmax_layer(void)
{
    cnn_unload((uint32_t *)ml_data);
    softmax_q17p14_q15((const q31_t *)ml_data, CNN_NUM_OUTPUTS, ml_softmax);
}

int main(void)
{
    int i;
    int digs, tens;

    MXC_ICC_Enable(MXC_ICC0); // Enable cache

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    printf("Waiting...\n");

    // DO NOT DELETE THIS LINE:
    MXC_Delay(SEC(2)); // Let debugger interrupt if needed

    // Enable peripheral, enable CNN interrupt, turn on CNN clock
    // CNN clock: 50 MHz div 1
    cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);

    printf("\n*** CNN Inference Test ***\n");

    cnn_init(); // Bring state machine into consistent state
    cnn_load_weights(); // Load kernels
    cnn_load_bias();
    cnn_configure(); // Configure state machine
    cnn_start(); // Start CNN processing
    load_input(); // Load data input via FIFO

    while (cnn_time == 0) __WFI(); // Wait for CNN

    //if (check_output() != CNN_OK) fail();
    softmax_layer();

    printf("\n*** PASS ***\n\n");

#ifdef CNN_INFERENCE_TIMER
    printf("Approximate data loading and inference time: %u us\n\n", cnn_time);
#endif

    cnn_disable(); // Shut down CNN clock, disable peripheral

    printf("Classification results:\n");
    for (i = 0; i < CNN_NUM_OUTPUTS; i++) {
        digs = (1000 * ml_softmax[i] + 0x4000) >> 15;
        tens = digs % 10;
        digs = digs / 10;
        printf("[%7d] -> Class %d: %d.%d%%\n", ml_data[i], i, digs, tens);
    }

    return 0;
}

/*
  SUMMARY OF OPS
  Hardware: 12,546,560 ops (12,314,112 macc; 232,448 comp; 0 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 59,568 bytes out of 442,368 bytes total (13%)
  Bias memory:   3 bytes out of 2,048 bytes total (0%)
*/
