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
 * @file    st7735s.h
 * @brief   Sitronix ST7735S LCD controller driver
 *          
 */

#ifndef LIBRARIES_MISCDRIVERS_DISPLAY_ST7735S_H_
#define LIBRARIES_MISCDRIVERS_DISPLAY_ST7735S_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DISP_HOR_RES 128
#define DISP_VER_RES 128
#define DRAW_BUF_SIZE (DISP_HOR_RES * DISP_VER_RES) / 10
#define LINEBUF_SIZE (3 * DISP_HOR_RES)

/* Register definitions taken from CrystalFontz
 * CFAF128128B1-0145T_SPI_Demo_Code.ino by Brent A. Crosby
 */
#define ST7735S_NOP (0x00)
#define ST7735S_SWRESET (0x01)
#define ST7735S_SLPOUT (0x11)
#define ST7735S_DISPON (0x29)
#define ST7735S_CASET (0x2A)
#define ST7735S_RASET (0x2B)
#define ST7735S_RAMWR (0x2C)
#define ST7735S_RAMRD (0x2E)
#define ST7735S_MADCTL (0x36)
#define ST7735S_COLMOD (0x3A)
#define ST7735S_FRMCTR1 (0xB1)
#define ST7735S_FRMCTR2 (0xB2)
#define ST7735S_FRMCTR3 (0xB3)
#define ST7735S_INVCTR (0xB4)
#define ST7735S_PWCTR1 (0xC0)
#define ST7735S_PWCTR2 (0xC1)
#define ST7735S_PWCTR3 (0xC2)
#define ST7735S_PWCTR4 (0xC3)
#define ST7735S_PWCTR5 (0xC4)
#define ST7735S_VMCTR1 (0xC5)
#define ST7735S_GAMCTRP1 (0xE0)
#define ST7735S_GAMCTRN1 (0xE1)

typedef struct {
    uint8_t cmd; /* Controller command # */
    uint8_t delay; /* Delay in ms after executing this command/data sequence */
    uint8_t len; /* Length of data */
    uint8_t *data;
} st7735s_regcfg_t;

typedef struct {
    /* Platform-specific delay function */
    void (*delayfn)(uint32_t ms);
    /* Platform-specific transmit function */
    int (*sendfn)(uint8_t *cmd, uint32_t cmd_len, uint8_t *data, uint32_t data_len);
    /* Panel-specific configuration */
    st7735s_regcfg_t *regcfg;
    /* Number of elements in the regcfg array */
    unsigned int ncfgs;
} st7735s_cfg_t;

int st7735s_init(st7735s_cfg_t *cfg);
int st7735s_pixel(uint32_t x, uint32_t y, uint32_t z);
int st7735s_write_pixels(uint8_t *data, unsigned int len);
int st7735s_xyloc(uint8_t row, uint8_t col);

#ifdef __cplusplus
}
#endif

#endif // LIBRARIES_MISCDRIVERS_DISPLAY_ST7735S_H_
