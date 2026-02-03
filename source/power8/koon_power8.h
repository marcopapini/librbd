/*
 *  Component: koon_power8.h
 *  KooN (K-out-of-N) RBD management - POWER8 platform-specific implementation
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

#ifndef KOON_POWER8_H_
#define KOON_POWER8_H_


#include "../generic/rbd_internal_generic.h"
#include "../koon.h"


#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for POWER8 VSX instruction set */
void *rbdKooNFillWorkerVsx(struct rbdKooNFillData *data);
void *rbdKooNGenericWorkerVsx(struct rbdKooNGenericData *data);
void *rbdKooNIdenticalWorkerVsx(struct rbdKooNIdenticalData *data);

void rbdKooNRecursionV2dVsx(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepV2dVsx(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNIdenticalFailStepV2dVsx(struct rbdKooNIdenticalData *data, unsigned int time);
#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */


#endif /* KOON_POWER8_H_ */
