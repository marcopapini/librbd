/*
 *  Component: rbd_internal_x86.h
 *  Internal APIs used by RBD library - Optimized using x86 platform-specific instruction sets
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

#ifndef RBD_INTERNAL_X86_H_
#define RBD_INTERNAL_X86_H_


#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)


#include <immintrin.h>


VARIABLE_TARGET("sse2") extern const __m128d v2dZeros;
VARIABLE_TARGET("sse2") extern const __m128d v2dOnes;
VARIABLE_TARGET("sse2") extern const __m128d v2dTwos;


FUNCTION_TARGET("sse2") __m128d capReliabilityV2dSse2(__m128d v2dR);

#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */


#if defined(ARCH_X86) && (CPU_ENABLE_SIMD != 0)

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
unsigned int x86Sse2Supported(void);

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
void retrieveX86CpuInfo(void);

#endif /* defined(ARCH_X86) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_X86_H_ */
