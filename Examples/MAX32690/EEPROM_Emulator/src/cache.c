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

#include "cache.h"

#include <stdio.h>
#include <string.h>
#include "flc.h"

int cache_init(cache_t *cache, uint32_t init_addr)
{
    int err;

    if (cache == NULL) {
        return E_NULL_PTR;
    } else if (init_addr < MXC_FLASH0_MEM_BASE ||
               init_addr > (MXC_FLASH0_MEM_BASE + MXC_FLASH0_MEM_SIZE)) {
        return E_BAD_PARAM;
    }

    // Configure FLC
    err = MXC_FLC_Init();
    if (err != E_NO_ERROR) {
        printf("Failed to initialize flash controller.\n");
        return err;
    }

    // Get starting address of flash page
    init_addr -= init_addr % MXC_FLASH0_PAGE_SIZE;

    // Initialize cache values and starting address
    memcpy(cache->cache, (void *)init_addr, MXC_FLASH0_PAGE_SIZE);
    cache->start_addr = init_addr;
    cache->end_addr = init_addr + MXC_FLASH0_PAGE_SIZE;
    cache->dirty = false;

    return E_NO_ERROR;
}

int cache_refresh(cache_t *cache, uint32_t next_addr)
{
    int err;

    if (cache == NULL) {
        return E_NULL_PTR;
    } else if (next_addr < MXC_FLASH0_MEM_BASE ||
               next_addr >= (MXC_FLASH0_MEM_BASE + MXC_FLASH0_MEM_SIZE)) {
        return E_BAD_PARAM;
    }

    // If cache contents modified, store it back to flash
    if (cache->dirty) {
        // Erase flash page before copying cache contents to it
        err = MXC_FLC_PageErase(cache->start_addr);
        if (err != E_NO_ERROR) {
            return err;
        }

        // Copy contents of cache to erase flash page
        err = MXC_FLC_Write(cache->start_addr, MXC_FLASH0_PAGE_SIZE, (uint32_t *)cache->cache);
        if (err != E_NO_ERROR) {
            return err;
        }
    }

    // Get starting address of flash page
    next_addr &= ~(MXC_FLASH0_PAGE_SIZE - 1);

    // Initialize cache values and starting address
    memcpy(cache->cache, (void *)next_addr, MXC_FLASH0_PAGE_SIZE);
    cache->start_addr = next_addr;
    cache->end_addr = next_addr + MXC_FLASH0_PAGE_SIZE;
    cache->dirty = false;

    return E_NO_ERROR;
}

int cache_write_back(cache_t *cache)
{
    int err;

    // Erase flash page before copying cache contents to it
    err = MXC_FLC_PageErase(cache->start_addr);
    if (err != E_NO_ERROR) {
        return err;
    }

    // Copy contents of cache to erase flash page
    err = MXC_FLC_Write(cache->start_addr, MXC_FLASH0_PAGE_SIZE, (uint32_t *)cache->cache);
    if (err != E_NO_ERROR) {
        return err;
    }

    cache->dirty = false;

    return err;
}
