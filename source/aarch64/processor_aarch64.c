/*
 *  Component: processor_aarch64.c
 *  Retrieval of CPU-related info - AArch64 platform-specific implementation
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2024 by Marco Papini <papini.m@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "../generic/rbd_internal_generic.h"


#if defined(ARCH_AARCH64) && CPU_ENABLE_SIMD != 0

#include "../os/os.h"

#if defined(OS_LINUX)
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <asm/hwcap.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#elif defined(OS_MACOS)
#include <sys/sysctl.h>
#endif


struct aarch64Cpu
{
    unsigned int sveSupported;      /* AArch64 SVE instruction set supported */
};


static struct aarch64Cpu aarch64Cpu;

/**
 * aarch64SveSupported
 *
 * SVE instruction set supported by the AArch64 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of SVE instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if SVE instruction set is available, 0 otherwise
 */
HIDDEN unsigned int aarch64SveSupported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return AArch64 SVE instruction set supported by the system */
    return aarch64Cpu.sveSupported;
}

/**
 * retrieveAarch64CpuInfo
 *
 * Retrieve AArch64-specific CPU info (supported instruction sets)
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - If SVE instruction set is supported
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN void retrieveAarch64CpuInfo(void)
{
#if defined(OS_MACOS)
    int val;
    size_t len;
#endif

    /**
     * Default processor info:
     * - no SVE
     */
    aarch64Cpu.sveSupported = 0;

#if defined(OS_LINUX)
    if (getauxval(AT_HWCAP) & HWCAP_SVE) {
        if (prctl(PR_SVE_GET_VL) > 0) {
            aarch64Cpu.sveSupported = 1;
        }
    }
#elif defined(OS_WINDOWS)
    if (IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)) {
        aarch64Cpu.sveSupported = 1;
    }
#elif defined(OS_MACOS)
#if 0 /* Currently MacOS does not support SVE */
    val = 0;
    len = sizeof(val);
    sysctlbyname("hw.optional.arm.FEAT_SVE", &val, &len, NULL, 0);
    if (val != 0) {
        aarch64Cpu.sveSupported = 1;
    }
#endif
#endif
}

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
