/*
 *  Component: koon_amd64.c
 *  KooN (K-out-of-N) RBD management - amd64 platform-specific implementation
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

#ifndef KOON_AMD64_H_
#define KOON_AMD64_H_


#include "../generic/rbd_internal_generic.h"
#include "../koon.h"


#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for x86 SSE2 instruction set */
void rbdKooNGenericSuccessStepV2dSse2(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepV2dSse2(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionV2dSse2(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV2dSse2(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNIdenticalFailStepV2dSse2(struct rbdKooNIdenticalData *data, unsigned int time);

/* Platform-specific functions for amd64 AVX instruction set */
void rbdKooNGenericSuccessStepV4dAvx(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepV4dAvx(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionV4dAvx(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV4dAvx(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNIdenticalFailStepV4dAvx(struct rbdKooNIdenticalData *data, unsigned int time);

/* Platform-specific functions for amd64 FMA instruction set */
void rbdKooNGenericSuccessStepV4dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepV4dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionV4dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV4dFma(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNGenericSuccessStepV2dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepV2dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionV2dFma(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV2dFma(struct rbdKooNIdenticalData *data, unsigned int time);

/* Platform-specific functions for amd64 AVX512F instruction set */
void rbdKooNGenericSuccessStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV8dAvx512f(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNIdenticalFailStepV8dAvx512f(struct rbdKooNIdenticalData *data, unsigned int time);
#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* KOON_AMD64_H_ */
