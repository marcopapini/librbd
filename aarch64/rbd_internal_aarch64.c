/*
 *  Component: rbd_internal_aarch64.c
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


#include "../generic/rbd_internal_generic.h"


#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_aarch64.h"
#include <arm_acle.h>


VARIABLE_TARGET("arch=armv8-a") const float64x2_t v2dZeros = {0.0, 0.0};
VARIABLE_TARGET("arch=armv8-a") const float64x2_t v2dOnes = {1.0, 1.0};
VARIABLE_TARGET("arch=armv8-a") const float64x2_t v2dTwos = {2.0, 2.0};
VARIABLE_TARGET("arch=armv8-a") const float64x2_t v2dMinusTwos = {-2.0, -2.0};


/**
 * capReliabilityV2dNeon
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with AArch64 NEON 128bit
 *
 * Input:
 *      float64x2_t v2dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 2 values, double-precision FP)
 *      to the accepted bounds exploiting AArch64 NEON 128bit
 *
 * Parameters:
 *      v2dR: Reliability
 *
 * Return (float64x2_t):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") float64x2_t capReliabilityV2dNeon(float64x2_t v2dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return vminnmq_f64(vmaxnmq_f64(v2dZeros, v2dR), v2dOnes);
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
