/*
 *  Component: rbd_internal_x86.c
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


#include "../generic/rbd_internal_generic.h"

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_x86.h"


VARIABLE_TARGET("sse2") const __m128d v2dZeros = {0.0, 0.0};
VARIABLE_TARGET("sse2") const __m128d v2dOnes = {1.0, 1.0};
VARIABLE_TARGET("sse2") const __m128d v2dTwos = {2.0, 2.0};


/**
 * capReliabilityV2dSse2
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with x86 SSE2 128bit
 *
 * Input:
 *      __m128d v2dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 2 values, double-precision FP)
 *      to the accepted bounds exploiting x86 SSE2 128bit
 *
 * Parameters:
 *      v2dR: Reliability
 *
 * Return (__m128d):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("sse2") __m128d capReliabilityV2dSse2(__m128d v2dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm_max_pd(_mm_min_pd(v2dOnes, v2dR), v2dZeros);
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
