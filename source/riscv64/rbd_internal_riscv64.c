/*
 *  Component: rbd_internal_riscv64.c
 *  Internal APIs used by RBD library - Optimized using RISC-V 64bit platform-specific instruction sets
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


#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_riscv64.h"
#include <riscv_vector.h>


/**
 * capReliabilityVNdRvv
 *
 * Cap reliability to accepted bounds [0.0, 1.0] with RISC-V 64bit RVV
 *
 * Input:
 *      vfloat64m1_t vNdR
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (vector of N values, double-precision FP)
 *      to the accepted bounds exploiting RISC-V 64bit RVV
 *
 * Parameters:
 *      vNdR: Reliability
 *      vl: Vector Length
 *
 * Return (vfloat64m1_t):
 *  Reliability within accepted bounds
 */
HIDDEN FUNCTION_TARGET("arch=+v") vfloat64m1_t capReliabilityVNdRvv(vfloat64m1_t vR, unsigned long int vl)
{
    /* Cap computed reliability to accepted bounds [0, 1] */
    return __riscv_vfmin_vf_f64m1(__riscv_vfmax_vf_f64m1(vR, 0.0, vl), 1.0, vl);
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
