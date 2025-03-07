/*
 *  Component: rbd_internal_amd64.c
 *  Internal APIs used by RBD library - Optimized using amd64 platform-specific instruction sets
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_amd64.h"


VARIABLE_TARGET("avx") const __m256d v4dZeros = {0.0, 0.0, 0.0, 0.0};
VARIABLE_TARGET("avx") const __m256d v4dOnes = {1.0, 1.0, 1.0, 1.0};
VARIABLE_TARGET("avx") const __m256d v4dTwos = {2.0, 2.0, 2.0, 2.0};
VARIABLE_TARGET("avx512f") const __m512d v8dZeros = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
VARIABLE_TARGET("avx512f") const __m512d v8dOnes = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
VARIABLE_TARGET("avx512f") const __m512d v8dTwos = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};


/**
 * capReliabilityV4dAvx
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with amd64 AVX 256bit
 *
 * Input:
 *      __m256d v4dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 4 values, double-precision FP)
 *      to the accepted bounds exploiting amd64 AVX 256bit
 *
 * Parameters:
 *      v4dR: Reliability
 *
 * Return (__m256d):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("avx") __m256d capReliabilityV4dAvx(__m256d v4dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm256_max_pd(_mm256_min_pd(v4dOnes, v4dR), v4dZeros);
}

/**
 * capReliabilityV8dAvx512f
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with amd64 AVX512F 512bit
 *
 * Input:
 *      __m512d v8dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 8 values, double-precision FP)
 *      to the accepted bounds exploiting amd64 AVX512F 512bit
 *
 * Parameters:
 *      v8dR: Reliability
 *
 * Return (__m512d):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("avx512f") __m512d capReliabilityV8dAvx512f(__m512d v8dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return _mm512_max_pd(_mm512_min_pd(v8dOnes, v8dR), v8dZeros);
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
