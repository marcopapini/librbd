/*
 *  Component: rbd_internal_amd64.h
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

#ifndef RBD_INTERNAL_AMD64_H_
#define RBD_INTERNAL_AMD64_H_


#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)


#include <immintrin.h>
#include <limits.h>
#include <string.h>

#include "../x86/rbd_internal_x86.h"


struct rbdKooNRecursionData
{
    unsigned char comb[SCHAR_MAX + 1];      /* Array for the computation of KooN combinations */
    unsigned char buff[(UCHAR_MAX + 1) * sizeof(__m512d)];      /* Temporary buffer */
    double      *s1dR;                      /* Pointer to array of reliabilities - Scalar 1 double */
    __m128d     *v2dR;                      /* Pointer to array of reliabilities - Vector 2 double */
    __m256d     *v4dR;                      /* Pointer to array of reliabilities - Vector 4 double */
    __m512d     *v8dR;                      /* Pointer to array of reliabilities - Vector 8 double */
};


VARIABLE_TARGET("avx") extern const __m256d v4dZeros;
VARIABLE_TARGET("avx") extern const __m256d v4dOnes;
VARIABLE_TARGET("avx") extern const __m256d v4dTwos;
VARIABLE_TARGET("avx512f") extern const __m512d v8dZeros;
VARIABLE_TARGET("avx512f") extern const __m512d v8dOnes;
VARIABLE_TARGET("avx512f") extern const __m512d v8dTwos;


FUNCTION_TARGET("avx") __m256d capReliabilityV4dAvx(__m256d v4dR);
FUNCTION_TARGET("avx512f") __m512d capReliabilityV8dAvx512f(__m512d v8dR);


/**
 * initKooNRecursionData
 *
 * Initialize the provided RBD KooN Recursive Data
 *
 * Input:
 *      None
 *
 * Output:
 *      struct rbdKooNRecursionData *data
 *
 * Description:
 *  This function initializes the provided RBD KooN Recursive Data
 *
 * Parameters:
 *      data: RBD KooN Recursive Data to be initialized
 */
static inline ALWAYS_INLINE void initKooNRecursionData(struct rbdKooNRecursionData *data) {
    unsigned long long alignAddr;
    memset(data, 0, sizeof(struct rbdKooNRecursionData));
    alignAddr = ((unsigned long long)(&data->buff) + sizeof(__m512d) - 1) & ~(sizeof(__m512d) - 1);
    data->s1dR = (double *)alignAddr;
    data->v2dR = (__m128d *)alignAddr;
    data->v4dR = (__m256d *)alignAddr;
    data->v8dR = (__m512d *)alignAddr;
}

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
unsigned int amd64Sse2Supported(void);

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
unsigned int amd64AvxSupported(void);

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
unsigned int amd64Fma3Supported(void);

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
unsigned int amd64Avx512fSupported(void);

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
void retrieveAmd64CpuInfo(void);


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_AMD64_H_ */
