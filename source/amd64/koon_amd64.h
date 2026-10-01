/*
 *  Component: koon_amd64.c
 *  KooN (K-out-of-N) RBD management - amd64 platform-specific implementation
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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
/* Platform-specific functions for amd64 AVX instruction set */
void *rbdKooNFillWorkerAvx(struct rbdKooNFillData *data);
void *rbdKooNGenericShannonWorkerAvx(struct rbdKooNGenericShannonData *data);
void *rbdKooNBddWorkerAvx(struct rbdKooNBddData *data);
void *rbdKooNIdenticalWorkerAvx(struct rbdKooNIdenticalData *data);

/* Platform-specific functions for amd64 FMA3 instruction set */
void *rbdKooNGenericShannonWorkerFma3(struct rbdKooNGenericShannonData *data);
void *rbdKooNBddWorkerFma3(struct rbdKooNBddData *data);
void *rbdKooNIdenticalWorkerFma3(struct rbdKooNIdenticalData *data);

/* Platform-specific functions for amd64 AVX512F instruction set */
void *rbdKooNFillWorkerAvx512f(struct rbdKooNFillData *data);
void *rbdKooNGenericShannonWorkerAvx512f(struct rbdKooNGenericShannonData *data);
void *rbdKooNBddWorkerAvx512f(struct rbdKooNBddData *data);
void *rbdKooNIdenticalWorkerAvx512f(struct rbdKooNIdenticalData *data);
#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* KOON_AMD64_H_ */
