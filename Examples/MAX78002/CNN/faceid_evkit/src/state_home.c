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

#include "keypad.h"
#include "state.h"
#include "utils.h"

#define X_START 56
#define Y_START 156
#define THICKNESS 4

/********************************** Type Defines  *****************************/
typedef void (*ScreenFunc)(void);

/************************************ VARIABLES ******************************/
#ifdef TFT_ENABLE
static void screen_faceID_home(void);

static text_t screen_msg[] = {
    // info
    { (char *)"FACEID DEMO", 11 },
    { (char *)"Start DEMO", 10 },
};

static int font = (int)&Liberation_Sans16x16[0];

/********************************* Static Functions **************************/
static void screen_faceID_home(void)
{
    MXC_TFT_SetBackGroundColor(4);

    MXC_TFT_PrintFont(50, 5, font, &screen_msg[0], NULL); // FACEID DEMO
    MXC_TFT_PrintFont(X_START + THICKNESS, Y_START + THICKNESS, font, &screen_msg[1],
                      NULL); // START DEMO
    // texts
    area_t left = { X_START, Y_START, 4, 4 + 26 };
    MXC_TFT_ClearArea(&left, 5);

    area_t right = { X_START + 120, Y_START, 4, 4 + 26 };
    MXC_TFT_ClearArea(&right, 5);

    area_t top = { X_START, Y_START, 120, 4 };
    MXC_TFT_ClearArea(&top, 5);

    area_t bottom = { X_START, Y_START + 26, 120, 4 };
    MXC_TFT_ClearArea(&bottom, 5);
#ifdef TS_ENABLE
    MXC_TS_RemoveAllButton();
    MXC_TS_AddButton(X_START, Y_START, X_START + 120, Y_START + 4 + 26 + 4, KEY_1);
#endif
}
#endif

static int init(void)
{
#ifdef TFT_ENABLE
    screen_faceID_home();
#endif
    return 0;
}

static int key_process(int key)
{
    switch (key) {
    case KEY_1:
        state_set_current(get_faceID_state());
        break;
    default:
        break;
    }

    return 0;
}

static State g_state = { "faceID_home", init, key_process, NULL, 0 };

/********************************* Public Functions **************************/
State *get_home_state(void)
{
    return &g_state;
}
