/*
 *  Component: rbd_internal_aarch64.h
 *  Internal APIs used by RBD library - Optimized using AArch64 platform-specific instruction sets
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

#ifndef RBD_INTERNAL_AARCH64_H_
#define RBD_INTERNAL_AARCH64_H_


#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)


#include <arm_neon.h>


VARIABLE_TARGET("arch=armv8-a") extern const float64x2_t v2dZeros;
VARIABLE_TARGET("arch=armv8-a") extern const float64x2_t v2dOnes;
VARIABLE_TARGET("arch=armv8-a") extern const float64x2_t v2dTwos;
VARIABLE_TARGET("arch=armv8-a") extern const float64x2_t v2dMinusTwos;


FUNCTION_TARGET("arch=armv8-a") float64x2_t capReliabilityV2dNeon(float64x2_t v2dR);


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_AARCH64_H_ */
