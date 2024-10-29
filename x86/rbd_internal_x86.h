/*
 *  Component: rbd_internal_x86_avx.h
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


#if CPU_X86_SSE2 != 0


#include <immintrin.h>


VARIABLE_TARGET("sse2") extern const __m128d v2dZeros;
VARIABLE_TARGET("sse2") extern const __m128d v2dOnes;
VARIABLE_TARGET("sse2") extern const __m128d v2dTwos;
#if CPU_X86_AVX != 0
VARIABLE_TARGET("avx") extern const __m256d v4dZeros;
VARIABLE_TARGET("avx") extern const __m256d v4dOnes;
VARIABLE_TARGET("avx") extern const __m256d v4dTwos;
#if CPU_X86_AVX512F != 0
VARIABLE_TARGET("avx512f") extern const __m512d v8dZeros;
VARIABLE_TARGET("avx512f") extern const __m512d v8dOnes;
VARIABLE_TARGET("avx512f") extern const __m512d v8dTwos;
#endif /* CPU_X86_AVX512F */
#endif /* CPU_X86_AVX */


FUNCTION_TARGET("sse2") __m128d capReliabilityV2dSse2(__m128d v2dR);
#if CPU_X86_AVX != 0
FUNCTION_TARGET("avx") __m256d capReliabilityV4dAvx(__m256d v4dR);
#endif /* CPU_X86_AVX */
#if CPU_X86_AVX512F != 0
FUNCTION_TARGET("avx512f") __m512d capReliabilityV8dAvx512f(__m512d v8dR);
#endif /* CPU_X86_AVX512F */


#endif /* CPU_X86_SSE2 */


#endif /* RBD_INTERNAL_X86_H_ */
