/*
 *  Component: cold_standby_aarch64.h
 *  Cold Stand-by RBD management - AArch64 platform-specific implementation
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

#ifndef COLD_STANDBY_AARCH64_H_
#define COLD_STANDBY_AARCH64_H_


#include "../generic/rbd_internal_generic.h"
#include "../cold_standby.h"


#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for AArch64 NEON instruction set */
void *rbdColdStandbyWorkerNeon(struct rbdColdStandbyData *data);

void rbdColdStandbyStepV2dNeon(struct rbdColdStandbyData *data, unsigned int time);
void rbdColdStandbyStepV1dNeon(struct rbdColdStandbyData *data, unsigned int time);

/* Platform-specific functions for AArch64 SVE instruction set */
void *rbdColdStandbyWorkerSve(struct rbdColdStandbyData *data);

#if !defined(COMPILER_VS)
void rbdColdStandbyStepVNdSve(svbool_t pg, struct rbdColdStandbyData *data, unsigned int time);
#endif /* !defined(COMPILER_VS) */
#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */


#endif /* COLD_STANDBY_AARCH64_H_ */
