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


#if CPU_X86_AVX != 0


#include <immintrin.h>


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


extern const __m128d v2dZeros;
extern const __m128d v2dOnes;
extern const __m128d v2dTwos;
extern const __m256d v4dZeros;
extern const __m256d v4dOnes;
extern const __m256d v4dTwos;
#if CPU_X86_AVX512F != 0
extern const __m512d v8dZeros;
extern const __m512d v8dOnes;
extern const __m512d v8dTwos;
#endif /* CPU_X86_AVX512F */


__m128d capReliabilityV2dAvx(__m128d v2dR);
__m256d capReliabilityV4dAvx(__m256d v4dR);
#if CPU_X86_AVX512F != 0
__m512d capReliabilityV8dAvx512f(__m512d v8dR);
#endif /* CPU_X86_AVX512F */


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_AVX */


#endif /* RBD_INTERNAL_X86_H_ */
