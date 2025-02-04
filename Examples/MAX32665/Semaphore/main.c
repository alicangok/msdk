
/**
 * @file    	main.c
 * @brief   	Semaphore example
 * @details 	Press button to overwrite a global variable. If someone is already writing to it, deny the right
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
#include "mxc_device.h"
#include "mxc_errors.h"
#include "pb.h"
#include "sema.h"
#include "mxc_sys.h"
#include "board.h"

/***** Definitions *****/
#define SW1 0
#define SW2 1

/***** Globals *****/
volatile int A_active = 0;
volatile int B_active = 0;

int shared_variable = 0;

/***** Functions *****/
void PB_AWrites(void *idx)
{
    int retval;
    //First check if A is already writing
    if (!A_active) {
        //Check if B is writing
        retval = MXC_SEMA_CheckSema(0);
        if (retval == E_NO_ERROR) {
            if (MXC_SEMA_GetSema(0) == E_NO_ERROR) {
                printf("A acquired semaphore!\n");
                printf("A started writing...\n");
                shared_variable = 100;
                printf("var = %d\n", shared_variable);
                A_active = 1;
            }
        } else if (retval == E_BUSY) {
            printf("A can't write right now.\n\n");
            printf("var = %d\n", shared_variable);
            return;
        }
    } else {
        A_active = !A_active;
        //Semaphore should be busy...
        retval = MXC_SEMA_CheckSema(0);
        if (retval == E_BUSY) {
            printf("A stopped writing.\n");
            MXC_SEMA_FreeSema(0);
            printf("A dropped the semaphore...\n");
        } else {
            printf("Something went wrong.\n");
            return;
        }
    }

    printf("\n");

    return;
}

void PB_BWrites(void *idx)
{
    int retval;
    //First check if B is already writing
    if (!B_active) {
        //Check if A is writing
        retval = MXC_SEMA_CheckSema(0);
        if (retval == E_NO_ERROR) {
            if (MXC_SEMA_GetSema(0) == E_NO_ERROR) {
                printf("B acquired semaphore!\n");
                printf("B started writing...\n");
                shared_variable = 200;
                printf("var = %d\n", shared_variable);
                B_active = 1;
            }
        } else if (retval == E_BUSY) {
            printf("B can't write right now.\n\n");
            printf("var = %d\n", shared_variable);
            return;
        }
    } else {
        B_active = !B_active;
        //Semaphore should be busy...
        retval = MXC_SEMA_CheckSema(0);
        if (retval == E_BUSY) {
            printf("B stopped writing.\n");
            MXC_SEMA_FreeSema(0);
            printf("B dropped the semaphore...\n");
        } else {
            printf("Something went wrong.\n");
            return;
        }
    }

    printf("\n");

    return;
}

// *****************************************************************************
int main(void)
{
    printf("***** Semaphore Example *****\n");
    printf("Push Button 0 = A tries to write\n");
#if !defined(BOARD_FTHR)
    printf("Push Button 1 = B tries to write\n");
#endif
    printf("\n");

    MXC_SEMA_Init();

    if (MXC_SEMA_GetSema(0) == E_NO_ERROR) {
        printf("Semaphore acquired.\n");
    }

    if (MXC_SEMA_CheckSema(0) == E_NO_ERROR) {
        printf("Semaphore free.\n");
    } else {
        printf("Semaphore locked.\n");
    }

    MXC_SEMA_FreeSema(0);

    if (MXC_SEMA_CheckSema(0) == E_NO_ERROR) {
        printf("Semaphore free.\n");
    } else {
        printf("Semaphore locked.\n");
    }

    PB_RegisterCallback(SW1, PB_AWrites);
#if !defined(BOARD_FTHR)
    PB_RegisterCallback(SW2, PB_BWrites);
#endif

    printf("\nExample running.\n");
    while (1) {}
}
