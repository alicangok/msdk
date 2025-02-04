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

#ifndef _FACEID_H_
#define _FACEID_H_

#define CAMERA_FREQ (10 * 1000 * 1000)
#define LP_MODE_ENABLE
#define LP_MODE 4 // 0:NO SLEEP, 1:SLEEP, 2:LPM, 3:UPM, 4:STANDBY, 5:BACKUP, 6:POWERDOWN
#define LP_TIME 500 //ms
// Mailboxes
#define ARM_MAILBOX_SIZE 1
#define RISCV_MAILBOX_SIZE 16
// Mailbox commands
#define START_FACEID 1
#define STOP_FACEID 2
#define IMAGE_READY 3
#define RESULT_READY 4

#define CAPTURE_X 50
#define CAPTURE_Y 290
#define SKIP_X 60
#define SKIP_Y 290
#define RUN_X 160
#define RUN_Y 290
#define BACK_X 0
#define BACK_Y 280

#define IMAGE_XRES 170 //200
#define IMAGE_YRES 130 //150

#define HEIGHT 160
#define WIDTH 120
#define THICKNESS 4
#define IMAGE_H 130 //150
#define IMAGE_W 170 //200
#define FRAME_COLOR 0x535A

#define X_START 45
#define Y_START 30

#define BYTE_PER_PIXEL 2

#define X_IMAGE_START 45
#define Y_IMAGE_START 30
#define LOW_LIGHT_THRESHOLD 20
#define FAST_FIFO // if defined, it uses fast fifo instead of fifo
// Data input: HWC (little data): 160x120x3
#define DATA_SIZE_IN (160 * 120 * 3)

#endif // _FACEID_H_
