/*
 *  Component: cold_standby_amd64.h
 *  Cold Stand-by RBD management - amd64 platform-specific implementation
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

#ifndef COLD_STANDBY_AMD64_H_
#define COLD_STANDBY_AMD64_H_


#include "../generic/rbd_internal_generic.h"
#include "../cold_standby.h"


#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for amd64 AVX instruction set */
void *rbdColdStandbyWorkerAvx(struct rbdColdStandbyData *data);

void rbdColdStandbyStepV4dAvx(struct rbdColdStandbyData *data, unsigned int time);
void rbdColdStandbyStepV1dAvx(struct rbdColdStandbyData *data, unsigned int time);

/* Platform-specific functions for amd64 FMA3 instruction set */
void *rbdColdStandbyWorkerFma3(struct rbdColdStandbyData *data);

void rbdColdStandbyStepV4dFma3(struct rbdColdStandbyData *data, unsigned int time);
void rbdColdStandbyStepV2dFma3(struct rbdColdStandbyData *data, unsigned int time);
void rbdColdStandbyStepV1dFma3(struct rbdColdStandbyData *data, unsigned int time);

/* Platform-specific functions for amd64 AVX512F instruction set */
void *rbdColdStandbyWorkerAvx512f(struct rbdColdStandbyData *data);

void rbdColdStandbyStepVNdAvx512f(__mmask8 mask, struct rbdColdStandbyData *data, unsigned int time);
#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* COLD_STANDBY_AMD64_H_ */
