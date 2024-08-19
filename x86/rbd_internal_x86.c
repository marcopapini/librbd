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


#include "../rbd_internal.h"

#if CPU_X86_AVX != 0
#include "rbd_internal_x86.h"


/* Save GCC target and optimization options and add x86 AVX instruction set */
#pragma GCC push_options
#pragma GCC target ("avx")
#if CPU_X86_FMA != 0
/* Add x86 FMA instruction set */
#pragma GCC target ("fma")
#endif /* CPU_X86_FMA */
#if CPU_X86_AVX512F != 0
/* Add x86 AVX512F instruction set */
#pragma GCC target ("avx512f")
#endif /* CPU_X86_AVX512F */


const __m128d v2dZeros = {0.0, 0.0};
const __m128d v2dOnes = {1.0, 1.0};
const __m128d v2dTwos = {2.0, 2.0};
const __m256d v4dZeros = {0.0, 0.0, 0.0, 0.0};
const __m256d v4dOnes = {1.0, 1.0, 1.0, 1.0};
const __m256d v4dTwos = {2.0, 2.0, 2.0, 2.0};
#if CPU_X86_AVX512F != 0
const __m512d v8dZeros = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
const __m512d v8dOnes = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
const __m512d v8dTwos = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
#endif /* CPU_X86_AVX512F */


/**
 * capReliabilityV2dAvx
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with x86 AVX 128bit
 *
 * Input:
 *      __m128d v2dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 2 values, double-precision FP)
 *      to the accepted bounds exploiting x86 AVX 128bit
 *
 * Parameters:
 *      v2dR: Reliability
 *
 * Return (__m128d):
 *  Reliability within accepted bounds
 */
__attribute__((visibility ("hidden"))) __m128d capReliabilityV2dAvx(__m128d v2dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm_max_pd(_mm_min_pd(v2dOnes, v2dR), v2dZeros);
}

/**
 * capReliabilityV4dAvx
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with x86 AVX 256bit
 *
 * Input:
 *      __m256d v4dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 4 values, double-precision FP)
 *      to the accepted bounds exploiting x86 AVX 256bit
 *
 * Parameters:
 *      v4dR: Reliability
 *
 * Return (__m256d):
 *  Reliability within accepted bounds
 */
__attribute__((visibility ("hidden"))) __m256d capReliabilityV4dAvx(__m256d v4dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm256_max_pd(_mm256_min_pd(v4dOnes, v4dR), v4dZeros);
}

#if CPU_X86_AVX512F != 0
/**
 * capReliabilityV8dAvx512f
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with x86 AVX512F 512bit
 *
 * Input:
 *      __m512d v8dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 8 values, double-precision FP)
 *      to the accepted bounds exploiting AVX512F 512bit
 *
 * Parameters:
 *      v8dR: Reliability
 *
 * Return (__m512d):
 *  Reliability within accepted bounds
 */
__attribute__((visibility ("hidden"))) __m512d capReliabilityV8dAvx512f(__m512d v8dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm512_max_pd(_mm512_min_pd(v8dOnes, v8dR), v8dZeros);
}
#endif /* CPU_X86_AVX512F */


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_AVX */
