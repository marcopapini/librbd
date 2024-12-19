/*
 *  Component: processor_x86.c
 *  Retrieval of CPU-related info - x86 platform-specific implementation
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


#if defined(ARCH_X86) && CPU_ENABLE_SIMD != 0

#include "../compiler/compiler.h"

#if defined(COMPILER_VS)
#include <intrin.h>

#define SSE2_ID             1       /* cpuid identifier to retrieve SSE2 support */
#define SSE2_REG            3       /* Register identifier (EDX) to retrieve SSE2 support */
#define SSE2_BIT            26      /* Bit to retrieve to retrieve SSE2 support */
#endif


struct x86Cpu
{
    unsigned int sse2Supported;     /* x86 SSE2 instruction set supported */
};


static struct x86Cpu x86Cpu;

/**
 * x86Sse2Supported
 *
 * SSE2 instruction set supported by the x86 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of SSE2 instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if SSE2 instruction set is available, 0 otherwise
 */
HIDDEN unsigned int x86Sse2Supported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return x86 SSE2 instruction set supported by the system */
    return x86Cpu.sse2Supported;
}

/**
 * retrieveX86CpuInfo
 *
 * Retrieve x86-specific CPU info (supported instruction sets)
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - If SSE2 instruction set is supported
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN void retrieveX86CpuInfo(void)
{
#if defined(COMPILER_VS)
    int cpuInfo[4];
    int nIds;
#endif

    /**
     * Default processor info:
     * - no SSE2
     */
    x86Cpu.sse2Supported = 0;

#if defined(COMPILER_VS)
    /* Calling __cpuid with Function ID 0x0 gets the highest valid function ID. */
    __cpuid(cpuInfo, 0);
    nIds = cpuInfo[0];

    if (nIds >= SSE2_ID) {
        /* Calling __cpuidex with Function ID SSE2_ID gets availability of SSE2. */
        __cpuidex(cpuInfo, SSE2_ID, 0);
        if ((cpuInfo[SSE2_REG] >> SSE2_BIT) != 0) {
            x86Cpu.sse2Supported = 1;
        }
    }
#else
    if (__builtin_cpu_supports("sse2") > 0) {
        x86Cpu.sse2Supported = 1;
    }
#endif
}

#endif /* defined(ARCH_X86) && CPU_ENABLE_SIMD != 0 */
