/**
 * @file    lp.c
 * @brief   Low power functions
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
#include "lp.h"
#include "pwrseq_regs.h"
#include "gcr_regs.h"
#include "mxc_device.h"
#include "mxc_errors.h"
#include "mxc_pins.h"
#include "mxc_sys.h"
#include "flc.h"
#include "mxc_delay.h"

/***** Functions *****/
void MXC_LP_ClearWakeStatus(void)
{
    MXC_PWRSEQ->lp_wakefl = 0xFFFFFFFF;

    /* These flags are slow to clear, so block until they do */
    while (MXC_PWRSEQ->lp_wakefl & (MXC_PWRSEQ->lpwk_en)) {}
}

void MXC_LP_EnableSRAM3(void)
{
    MXC_PWRSEQ->lpmemsd &= ~MXC_F_PWRSEQ_LPMEMSD_SRAM3_OFF;
}

void MXC_LP_DisableSRAM3(void)
{
    MXC_PWRSEQ->lpmemsd |= MXC_F_PWRSEQ_LPMEMSD_SRAM3_OFF;
}

void MXC_LP_EnableSRAM2(void)
{
    MXC_PWRSEQ->lpmemsd &= ~MXC_F_PWRSEQ_LPMEMSD_SRAM2_OFF;
}

void MXC_LP_DisableSRAM2(void)
{
    MXC_PWRSEQ->lpmemsd |= MXC_F_PWRSEQ_LPMEMSD_SRAM2_OFF;
}

void MXC_LP_EnableSRAM1(void)
{
    MXC_PWRSEQ->lpmemsd &= ~MXC_F_PWRSEQ_LPMEMSD_SRAM1_OFF;
}

void MXC_LP_DisableSRAM1(void)
{
    MXC_PWRSEQ->lpmemsd |= MXC_F_PWRSEQ_LPMEMSD_SRAM1_OFF;
}

void MXC_LP_EnableSRAM0(void)
{
    MXC_PWRSEQ->lpmemsd &= ~MXC_F_PWRSEQ_LPMEMSD_SRAM0_OFF;
}

void MXC_LP_DisableSRAM0(void)
{
    MXC_PWRSEQ->lpmemsd |= MXC_F_PWRSEQ_LPMEMSD_SRAM0_OFF;
}

void MXC_LP_EnableICacheLightSleep(void)
{
    MXC_GCR->mem_ctrl |= (MXC_F_GCR_MEM_CTRL_ICACHE_RET);
}

void MXC_LP_DisableICacheLightSleep(void)
{
    MXC_GCR->mem_ctrl &= ~(MXC_F_GCR_MEM_CTRL_ICACHE_RET);
}

void MXC_LP_EnableSysRAM3LightSleep(void)
{
    MXC_GCR->mem_ctrl |= (MXC_F_GCR_MEM_CTRL_RAM3_LS);
}

void MXC_LP_DisableSysRAM3LightSleep(void)
{
    MXC_GCR->mem_ctrl &= ~(MXC_F_GCR_MEM_CTRL_RAM3_LS);
}

void MXC_LP_EnableSysRAM2LightSleep(void)
{
    MXC_GCR->mem_ctrl |= (MXC_F_GCR_MEM_CTRL_RAM2_LS);
}

void MXC_LP_DisableSysRAM2LightSleep(void)
{
    MXC_GCR->mem_ctrl &= ~(MXC_F_GCR_MEM_CTRL_RAM2_LS);
}

void MXC_LP_EnableSysRAM1LightSleep(void)
{
    MXC_GCR->mem_ctrl |= (MXC_F_GCR_MEM_CTRL_RAM1_LS);
}

void MXC_LP_DisableSysRAM1LightSleep(void)
{
    MXC_GCR->mem_ctrl &= ~(MXC_F_GCR_MEM_CTRL_RAM1_LS);
}

void MXC_LP_EnableSysRAM0LightSleep(void)
{
    MXC_GCR->mem_ctrl |= (MXC_F_GCR_MEM_CTRL_RAM0_LS);
}

void MXC_LP_DisableSysRAM0LightSleep(void)
{
    MXC_GCR->mem_ctrl &= ~(MXC_F_GCR_MEM_CTRL_RAM0_LS);
}

void MXC_LP_EnableRTCAlarmWakeup(void)
{
    MXC_GCR->pm |= MXC_F_GCR_PM_RTCWK_EN;
}

void MXC_LP_DisableRTCAlarmWakeup(void)
{
    MXC_GCR->pm &= ~MXC_F_GCR_PM_RTCWK_EN;
}

void MXC_LP_EnableGPIOWakeup(const mxc_gpio_cfg_t *wu_pins)
{
    MXC_GCR->pm |= MXC_F_GCR_PM_GPIOWK_EN;
    //switch(wu_pins->port)
    //{
    /*case 0:*/ MXC_PWRSEQ->lpwk_en |= wu_pins->mask; //break;
    //}
}

void MXC_LP_DisableGPIOWakeup(const mxc_gpio_cfg_t *wu_pins)
{
    //switch(wu_pins->port)
    //{
    /*  case 0:*/ MXC_PWRSEQ->lpwk_en &= ~wu_pins->mask; //break;
    //}

    if (MXC_PWRSEQ->lpwk_en == 0) {
        MXC_GCR->pm &= ~MXC_F_GCR_PM_GPIOWK_EN;
    }
}

void MXC_LP_EnterSleepMode(void)
{
    // Clear SLEEPDEEP bit
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    // Go into Sleep mode and wait for an interrupt to wake the processor
    __WFI();
}

void MXC_LP_EnterDeepSleepMode(void)
{
    // Set SLEEPDEEP bit
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // Auto-powerdown 96 MHz oscillator when in deep sleep
    MXC_GCR->pm |= MXC_F_GCR_PM_HFIOPD;
    // Go into Deepsleep mode and wait for an interrupt to wake the processor
    __WFI();
}

void MXC_LP_EnterBackupMode(void)
{
    MXC_GCR->pm &= ~MXC_F_GCR_PM_MODE;
    MXC_GCR->pm |= MXC_S_GCR_PM_MODE_BACKUP;
    while (1) {}
}

void MXC_LP_EnterShutdownMode(void)
{
    MXC_GCR->pm &= ~MXC_F_GCR_PM_MODE;
    MXC_GCR->pm |= MXC_S_GCR_PM_MODE_SHUTDOWN;
    while (1) {}
}

int MXC_LP_SetOperatingVoltage(mxc_lp_ovr_t ovr)
{
    uint32_t current_clock, div;
    int error;

    // Ensure part is operating from internal LDO for core power
    if (MXC_PWRSEQ->lp_ctrl & MXC_F_PWRSEQ_LP_CTRL_LDO_DIS) {
        return E_BAD_STATE;
    }

    // Select the 8KHz nanoring (no guarantee 32KHz is attached) as system clock source
    current_clock = MXC_GCR->clk_ctrl & MXC_F_GCR_CLK_CTRL_CLKSEL;
    if (current_clock == MXC_SYS_CLOCK_HIRC) {
        error = MXC_SYS_Clock_Select(MXC_SYS_CLOCK_NANORING);
        if (error != E_NO_ERROR) {
            return error;
        }
    }

    // Set flash wait state for any clock so its not to low after clock changes.
    MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                        (0x5UL << MXC_F_GCR_MEM_CTRL_FWS_POS);

    // Set the OVR bits
    MXC_PWRSEQ->lp_ctrl &= ~(MXC_F_PWRSEQ_LP_CTRL_OVR);
    MXC_PWRSEQ->lp_ctrl |= ovr;

    // Set LVE bit
    if (ovr == MXC_LP_OVR_0_9) {
        MXC_FLC->ctrl |= MXC_F_FLC_CTRL_LVE;

    } else {
        MXC_FLC->ctrl &= ~(MXC_F_FLC_CTRL_LVE);
    }

    // Revert the clock to original state if it was HIRC
    if (current_clock == MXC_SYS_CLOCK_HIRC) {
        error = MXC_SYS_Clock_Select(MXC_SYS_CLOCK_HIRC);
        if (error != E_NO_ERROR) {
            return error;
        }
    }

    // Update SystemCoreClock variable
    SystemCoreClockUpdate();

    // Get the clock divider
    div = (MXC_GCR->clk_ctrl & MXC_F_GCR_CLK_CTRL_PSC) >> MXC_F_GCR_CLK_CTRL_PSC_POS;

    // Set Flash Wait States
    if (ovr == MXC_LP_OVR_0_9) {
        if (div == 0) {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x2UL << MXC_F_GCR_MEM_CTRL_FWS_POS);

        } else {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x1UL << MXC_F_GCR_MEM_CTRL_FWS_POS);
        }

    } else if (ovr == MXC_LP_OVR_1_0) {
        if (div == 0) {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x2UL << MXC_F_GCR_MEM_CTRL_FWS_POS);

        } else {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x1UL << MXC_F_GCR_MEM_CTRL_FWS_POS);
        }

    } else {
        if (div == 0) {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x4UL << MXC_F_GCR_MEM_CTRL_FWS_POS);

        } else if (div == 1) {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x2UL << MXC_F_GCR_MEM_CTRL_FWS_POS);

        } else {
            MXC_GCR->mem_ctrl = (MXC_GCR->mem_ctrl & ~(MXC_F_GCR_MEM_CTRL_FWS)) |
                                (0x1UL << MXC_F_GCR_MEM_CTRL_FWS_POS);
        }
    }

    // Caller must perform peripheral reset

    return E_NO_ERROR;
}

void MXC_LP_EnableSRamRet0(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL0;
}

void MXC_LP_DisableSRamRet0(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL0;
}

void MXC_LP_EnableSRamRet1(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL1;
}

void MXC_LP_DisableSRamRet1(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL1;
}

void MXC_LP_EnableSRamRet2(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL2;
}

void MXC_LP_DisableSRamRet2(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL2;
}

void MXC_LP_EnableSRamRet3(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL3;
}

void MXC_LP_DisableSRamRet3(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_RAMRET_SEL3;
}

void MXC_LP_EnableBlockDetect(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_VCORE_DET_BYPASS;
}

void MXC_LP_DisableBlockDetect(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_VCORE_DET_BYPASS;
}

void MXC_LP_EnableRamRetReg(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_RETREG_EN;
}

void MXC_LP_DisableRamRetReg(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_RETREG_EN;
}

void MXC_LP_EnableFastWk(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_FAST_WK_EN;
}

void MXC_LP_DisableFastWk(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_FAST_WK_EN;
}

void MXC_LP_EnableBandGap(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_BG_OFF;
}

void MXC_LP_DisableBandGap(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_BG_OFF;
}

void MXC_LP_EnableVCorePORSignal(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_VCORE_POR_DIS;
}

void MXC_LP_DisableVCorePORSignal(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_VCORE_POR_DIS;
}

void MXC_LP_EnableLDO(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_LDO_DIS;
}

void MXC_LP_DisableLDO(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_LDO_DIS;
}

void MXC_LP_EnableVCoreSVM(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_VCORE_SVM_DIS;
}

void MXC_LP_DisableVCoreSVM(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_VCORE_SVM_DIS;
}

void MXC_LP_EnableVDDIOPorMonitoF(void)
{
    MXC_PWRSEQ->lp_ctrl &= ~MXC_F_PWRSEQ_LP_CTRL_VDDIO_POR_DIS;
}

void MXC_LP_DisableVDDIOPorMonitor(void)
{
    MXC_PWRSEQ->lp_ctrl |= MXC_F_PWRSEQ_LP_CTRL_VDDIO_POR_DIS;
}
