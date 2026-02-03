/*
 *  Component: rbd_internal_power8.c
 *  Internal APIs used by RBD library - Optimized using POWER8 platform-specific instruction sets
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


#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_power8.h"


VARIABLE_TARGET("vsx") const double64x2 v2dZeros = {0.0, 0.0};
VARIABLE_TARGET("vsx") const double64x2 v2dOnes = {1.0, 1.0};
VARIABLE_TARGET("vsx") const double64x2 v2dTwos = {2.0, 2.0};


/**
 * capReliabilityV2dVsx
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with POWER8 VSX 128bit
 *
 * Input:
 *      double64x2 v2dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of 2 values, double-precision FP)
 *      to the accepted bounds exploiting POWER8 VSX 128bit
 *
 * Parameters:
 *      v2dR: Reliability
 *
 * Return (double64x2):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("vsx") double64x2 capReliabilityV2dVsx(double64x2 v2dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    return vec_min(vec_max(v2dZeros, v2dR), v2dOnes);
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
