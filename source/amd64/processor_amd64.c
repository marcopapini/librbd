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
void retrieveAmd64CpuInfo(void)
{
    /**
     * Default processor info:
     * - no SSE2, no AVX, no FMA3, no AVX512F
     */
    amd64Cpu.sse2Supported = 0;
    amd64Cpu.avxSupported = 0;
    amd64Cpu.fma3Supported = 0;
    amd64Cpu.avx512fSupported = 0;
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
}

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
