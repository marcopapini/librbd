/*
 *  Component: hot_standby_amd64.h
 *  Hot Stand-by RBD management - amd64 platform-specific implementation
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

#ifndef HOT_STANDBY_AMD64_H_
#define HOT_STANDBY_AMD64_H_


#include "../generic/rbd_internal_generic.h"
#include "../hot_standby.h"


#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for amd64 AVX instruction set */
void *rbdHotStandbyWorkerAvx(struct rbdHotStandbyData *data);

void rbdHotStandbyStepV4dAvx(struct rbdHotStandbyData *data, unsigned int time);

/* Platform-specific functions for amd64 FMA3 instruction set */
void *rbdHotStandbyWorkerFma3(struct rbdHotStandbyData *data);

void rbdHotStandbyStepV4dFma3(struct rbdHotStandbyData *data, unsigned int time);
void rbdHotStandbyStepV2dFma3(struct rbdHotStandbyData *data, unsigned int time);
void rbdHotStandbyStepV1dFma3(struct rbdHotStandbyData *data, unsigned int time);

/* Platform-specific functions for amd64 AVX512F instruction set */
void *rbdHotStandbyWorkerAvx512f(struct rbdHotStandbyData *data);

void rbdHotStandbyStepVNdAvx512f(__mmask8 mask, struct rbdHotStandbyData *data, unsigned int time);
#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* HOT_STANDBY_AMD64_H_ */
