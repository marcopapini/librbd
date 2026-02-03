/*
 *  Component: parallel_aarch64.h
 *  Parallel RBD management - AArch64 platform-specific implementation
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

#ifndef PARALLEL_AARCH64_H_
#define PARALLEL_AARCH64_H_


#include "../generic/rbd_internal_generic.h"
#include "../parallel.h"


#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for AArch64 NEON instruction set */
void *rbdParallelGenericWorkerNeon(struct rbdParallelData *data);
void *rbdParallelIdenticalWorkerNeon(struct rbdParallelData *data);

void rbdParallelGenericStepV2dNeon(struct rbdParallelData *data, unsigned int time);
void rbdParallelIdenticalStepV2dNeon(struct rbdParallelData *data, unsigned int time);

/* Platform-specific functions for AArch64 SVE instruction set */
void *rbdParallelGenericWorkerSve(struct rbdParallelData *data);
void *rbdParallelIdenticalWorkerSve(struct rbdParallelData *data);

void rbdParallelGenericStepVNdSve(struct rbdParallelData *data, unsigned int time);
void rbdParallelIdenticalStepVNdSve(struct rbdParallelData *data, unsigned int time);
#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */


#endif /* PARALLEL_AARCH64_H_ */
