/*
 *  Component: bridge_riscv64.h
 *  Bridge RBD management - RISC-V 64bit platform-specific implementation
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

#ifndef BRIDGE_RISCV_H_
#define BRIDGE_RISCV_H_


#include "../generic/rbd_internal_generic.h"
#include "../bridge.h"


#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for RISC-V 64bit RVV instruction set */
void *rbdBridgeGenericWorkerRvv(struct rbdBridgeData *data);
void *rbdBridgeIdenticalWorkerRvv(struct rbdBridgeData *data);

void rbdBridgeGenericStepVNdRvv(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepVNdRvv(struct rbdBridgeData *data, unsigned int time);
#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */


#endif /* BRIDGE_RISCV_H_ */
