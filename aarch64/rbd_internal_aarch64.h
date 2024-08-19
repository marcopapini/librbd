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


#if CPU_AARCH64_NEON != 0


#include <arm_neon.h>


/* Save GCC target and optimization options and add ARM v8-A architecture */
#pragma GCC push_options
#pragma GCC target ("arch=armv8-a")



extern const float64x2_t v2dZeros;
extern const float64x2_t v2dOnes;
extern const float64x2_t v2dTwos;
extern const float64x2_t v2dMinusTwos;


float64x2_t capReliabilityV2dNeon(float64x2_t v2dR);


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_AARCH64_NEON */


#endif /* RBD_INTERNAL_AARCH64_H_ */
