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

/*
 * This header file was automatically generated for the faceid_aug_qat_best network from a template.
 * Please do not edit; instead, edit the template and regenerate.
 */

#ifndef __CNN_2_H__
#define __CNN_2_H__

/*
  SUMMARY OF OPS
  Hardware: 56,295,040 ops (55,234,560 macc; 1,052,800 comp; 7,680 add; 0 mul; 0 bitwise)
    Layer 0: 8,601,600 ops (8,294,400 macc; 307,200 comp; 0 add; 0 mul; 0 bitwise)
    Layer 1: 22,579,200 ops (22,118,400 macc; 460,800 comp; 0 add; 0 mul; 0 bitwise)
    Layer 2: 11,251,200 ops (11,059,200 macc; 192,000 comp; 0 add; 0 mul; 0 bitwise)
    Layer 3: 5,587,200 ops (5,529,600 macc; 57,600 comp; 0 add; 0 mul; 0 bitwise)
    Layer 4: 2,602,880 ops (2,580,480 macc; 22,400 comp; 0 add; 0 mul; 0 bitwise)
    Layer 5: 2,584,960 ops (2,580,480 macc; 4,480 comp; 0 add; 0 mul; 0 bitwise)
    Layer 6: 2,584,960 ops (2,580,480 macc; 4,480 comp; 0 add; 0 mul; 0 bitwise)
    Layer 7: 495,360 ops (491,520 macc; 3,840 comp; 0 add; 0 mul; 0 bitwise)
    Layer 8: 7,680 ops (0 macc; 0 comp; 7,680 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 176,048 bytes out of 442,368 bytes total (39.8%)
  Bias memory:   288 bytes out of 2,048 bytes total (14.1%)
*/

/* Number of outputs for this network */
#define CNN_2_NUM_OUTPUTS 512

/* Perform minimum accelerator initialization so it can be configured */
int cnn_2_init(void);

/* Configure accelerator for the given network */
int cnn_2_configure(void);

/* Load accelerator weights */
int cnn_2_load_weights_from_SD(void);

/* Verify accelerator weights (debug only) */
int cnn_2_verify_weights(void);

/* Load accelerator bias values (if needed) */
int cnn_2_load_bias(void);

/* Unload results from accelerator */
int cnn_2_unload(uint32_t *out_buf);

#endif // __CNN_2_H__
