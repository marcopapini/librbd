/*
 *  Component: hot_standby_riscv64.h
 *  Hot Stand-by RBD management - RISC-V 64bit platform-specific implementation
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

#ifndef HOT_STANDBY_RISCV_H_
#define HOT_STANDBY_RISCV_H_


#include "../generic/rbd_internal_generic.h"
#include "../hot_standby.h"


#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for RISC-V 64bit RVV instruction set */
void *rbdHotStandbyWorkerRvv(struct rbdHotStandbyData *data);

void rbdHotStandbyStepVNdRvv(struct rbdHotStandbyData *data, unsigned int time, unsigned long int vl);
#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */


#endif /* HOT_STANDBY_RISCV_H_ */
