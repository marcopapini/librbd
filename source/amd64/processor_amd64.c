/*
 *  Component: processor_amd64.c
 *  Retrieval of CPU-related info - amd64 platform-specific implementation
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


#if defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0

#include "../compiler/compiler.h"

#if defined(COMPILER_VS)
#include <intrin.h>

#define SSE2_ID             1       /* cpuid identifier to retrieve SSE2 support */
#define SSE2_REG            3       /* Register identifier (EDX) to retrieve SSE2 support */
#define SSE2_BIT            26      /* Bit to retrieve to retrieve SSE2 support */
#define AVX_ID              1       /* cpuid identifier to retrieve AVX support */
#define AVX_REG             2       /* Register identifier (ECX) to retrieve AVX support */
#define AVX_BIT             28      /* Bit to retrieve to retrieve AVX support */
#define FMA3_ID             1       /* cpuid identifier to retrieve FMA3 support */
#define FMA3_REG            3       /* Register identifier (EDX) to retrieve FMA3 support */
#define FMA3_BIT            12      /* Bit to retrieve to retrieve FMA3 support */
#define AVX512F_ID          7       /* cpuid identifier to retrieve AVX512F support */
#define AVX512F_REG         2       /* Register identifier (ECX) to retrieve AVX512F support */
#define AVX512F_BIT         16      /* Bit to retrieve to retrieve AVX512F support */

#if (SSE2_ID != AVX_ID) || (SSE2_ID != FMA3_ID)
#error "Wrong configuration of cpuid IDs"
#endif
#endif


struct amd64Cpu
{
    unsigned int sse2Supported;     /* amd64 SSE2 instruction set supported */
    unsigned int avxSupported;      /* amd64 AVX instruction set supported */
    unsigned int fma3Supported;     /* amd64 FMA3 instruction set supported */
    unsigned int avx512fSupported;  /* amd64 AVX512F instruction set supported */
};


static struct amd64Cpu amd64Cpu;

/**
 * amd64Sse2Supported
 *
 * SSE2 instruction set supported by the amd64 system
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
HIDDEN unsigned int amd64Sse2Supported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return x86 SSE2 instruction set supported by the system */
    return amd64Cpu.sse2Supported;
}

/**
 * amd64AvxSupported
 *
 * AVX instruction set supported by the amd64 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of AVX instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if AVX instruction set is available, 0 otherwise
 */
HIDDEN unsigned int amd64AvxSupported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return amd64 AVX instruction set supported by the system */
    return amd64Cpu.avxSupported;
}

/**
 * amd64Fma3Supported
 *
 * FMA3 instruction set supported by the amd64 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of FMA3 instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if FMA3 instruction set is available, 0 otherwise
 */
HIDDEN unsigned int amd64Fma3Supported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return amd64 FMA3 instruction set supported by the system */
    return amd64Cpu.fma3Supported;
}

/**
 * amd64Avx512fSupported
 *
 * AVX512F instruction set supported by the amd64 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of AVX512F instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if AVX512F instruction set is available, 0 otherwise
 */
HIDDEN unsigned int amd64Avx512fSupported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return amd64 AVX512F instruction set supported by the system */
    return amd64Cpu.avx512fSupported;
}

/**
 * retrieveAmd64CpuInfo
 *
 * Retrieve amd64-specific CPU info (supported instruction sets)
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
 *  - If AVX instruction set is supported
 *  - If FMA3 instruction set is supported
 *  - If AVX512F instruction set is supported
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN void retrieveAmd64CpuInfo(void)
{
#if defined(COMPILER_VS)
    int cpuInfo[4];
    int nIds;
#endif

    /**
     * Default processor info:
     * - no SSE2, no AVX, no FMA3, no AVX512F
     */
    amd64Cpu.sse2Supported = 0;
    amd64Cpu.avxSupported = 0;
    amd64Cpu.fma3Supported = 0;
    amd64Cpu.avx512fSupported = 0;

#if defined(COMPILER_VS)
    /* Calling __cpuid with Function ID 0x0 gets the highest valid function ID. */
    __cpuid(cpuInfo, 0);
    nIds = cpuInfo[0];

    if (nIds >= SSE2_ID) {
        /* Calling __cpuidex with Function ID SSE2_ID gets availability of SSE2, AVX, FMA3. */
        __cpuidex(cpuInfo, SSE2_ID, 0);
        if (((cpuInfo[SSE2_REG] >> SSE2_BIT) & 0x1) != 0) {
            amd64Cpu.sse2Supported = 1;
            if (((cpuInfo[AVX_REG] >> AVX_BIT) & 0x1) != 0) {
                amd64Cpu.avxSupported = 1;
                if (((cpuInfo[FMA3_REG] >> FMA3_BIT) & 0x1) != 0) {
                    amd64Cpu.fma3Supported = 1;
                    if (nIds >= AVX512F_ID) {
                        /* Calling __cpuidex with Function ID AVX512F_ID gets availability of AVX512F. */
                        __cpuidex(cpuInfo, AVX512F_ID, 0);
                        if (((cpuInfo[AVX512F_REG] >> AVX512F_BIT) & 0x1) != 0) {
                            amd64Cpu.avx512fSupported = 1;
                        }
                    }
                }
            }
        }
    }
#else
    if (__builtin_cpu_supports("sse2") > 0) {
        amd64Cpu.sse2Supported = 1;
        if (__builtin_cpu_supports("avx") > 0) {
            amd64Cpu.avxSupported = 1;
            if (__builtin_cpu_supports("fma") > 0) {
                amd64Cpu.fma3Supported = 1;
                if (__builtin_cpu_supports("avx512f") > 0) {
                    amd64Cpu.avx512fSupported = 1;
                }
            }
        }
    }
#endif
}

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
