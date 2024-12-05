/*
 *  Component: bridge_amd64.h
 *  Bridge RBD management - amd64 platform-specific implementation
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

#ifndef BRIDGE_AMD64_H_
#define BRIDGE_AMD64_H_


#include "../generic/rbd_internal_generic.h"
#include "../bridge.h"

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for amd64 AVX instruction set */
void rbdBridgeGenericStepV4dAvx(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV4dAvx(struct rbdBridgeData *data, unsigned int time);

/* Platform-specific functions for amd64 FMA3 instruction set */
void rbdBridgeGenericStepV4dFma3(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV4dFma3(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeGenericStepV2dFma3(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV2dFma3(struct rbdBridgeData *data, unsigned int time);

/* Platform-specific functions for amd64 AVX512F instruction set */
void rbdBridgeGenericStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time);
#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */


#endif /* BRIDGE_AMD64_H_ */
