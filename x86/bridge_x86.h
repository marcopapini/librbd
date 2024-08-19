/*
 *  Component: bridge_x86.h
 *  Bridge RBD management - x86 platform-specific implementation
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

#ifndef BRIDGE_X86_H_
#define BRIDGE_X86_H_


#include "../rbd_internal.h"
#include "../bridge.h"


#if CPU_X86_AVX != 0
/* Platform-specific functions for x86 AVX instruction set */
void rbdBridgeGenericStepV4dAvx(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV4dAvx(struct rbdBridgeData *data, unsigned int time);

void rbdBridgeGenericStepV2dAvx(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV2dAvx(struct rbdBridgeData *data, unsigned int time);

/* Platform-specific functions for x86 prefetch */
void rbdBridgeGenericPrefetch(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalPrefetch(struct rbdBridgeData *data, unsigned int time);
#endif

#if CPU_X86_FMA != 0
/* Platform-specific functions for x86 FMA instruction set */
void rbdBridgeGenericStepV4dFma(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV4dFma(struct rbdBridgeData *data, unsigned int time);

void rbdBridgeGenericStepV2dFma(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV2dFma(struct rbdBridgeData *data, unsigned int time);
#endif /* CPU_X86_FMA */

#if CPU_X86_AVX512F != 0
/* Platform-specific functions for x86 AVX512F instruction set */
void rbdBridgeGenericStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time);
#endif /* CPU_X86_AVX512F */


#endif /* BRIDGE_X86_H_ */
