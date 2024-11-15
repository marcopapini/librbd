/*
 *  Component: rbd_internal_amd64_avx.h
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


VARIABLE_TARGET("sse2") extern const __m128d v2dZeros;
VARIABLE_TARGET("sse2") extern const __m128d v2dOnes;
VARIABLE_TARGET("sse2") extern const __m128d v2dTwos;
VARIABLE_TARGET("avx") extern const __m256d v4dZeros;
VARIABLE_TARGET("avx") extern const __m256d v4dOnes;
VARIABLE_TARGET("avx") extern const __m256d v4dTwos;
VARIABLE_TARGET("avx512f") extern const __m512d v8dZeros;
VARIABLE_TARGET("avx512f") extern const __m512d v8dOnes;
VARIABLE_TARGET("avx512f") extern const __m512d v8dTwos;


FUNCTION_TARGET("sse2") __m128d capReliabilityV2dSse2(__m128d v2dR);
FUNCTION_TARGET("avx") __m256d capReliabilityV4dAvx(__m256d v4dR);
FUNCTION_TARGET("avx512f") __m512d capReliabilityV8dAvx512f(__m512d v8dR);


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_AMD64_H_ */
