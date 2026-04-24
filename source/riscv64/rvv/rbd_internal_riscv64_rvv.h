/*
 *  Component: rbd_internal_riscv64_rvv.h
 *  Internal APIs used by RBD library - Optimized using RISC-V 64bit RVV instruction set
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

#ifndef RBD_INTERNAL_RISCV64_RVV_H_
#define RBD_INTERNAL_RISCV64_RVV_H_


#include "../../generic/rbd_internal_generic.h"


#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
/**
 * minimumRiscv64Rvv
 *
 * Compute minimum between two numbers with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      int a
 *      int b
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the minimum between two numbers exploiting RISC-V 64bit RVV instruction set
 *
 * Parameters:
 *      a: first value for minimum computation
 *      b: second value for minimum computation
 *
 * Return (int):
 *  minimum value
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int minimumRiscv64Rvv(int a, int b) {
    return (a <= b) ? a : b;
}

/**
 * floorDivisionRiscv64Rvv
 *
 * Compute floor value of division with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the floor value of the requested division exploiting RISC-V 64bit RVV instruction set
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Floor value of division
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int floorDivisionRiscv64Rvv(int dividend, int divisor) {
    return (dividend / divisor);
}

/**
 * ceilDivisionRiscv64Rvv
 *
 * Compute ceil value of division with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the ceil value of the requested division exploiting RISC-V 64bit RVV instruction set
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Ceil value of division
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int ceilDivisionRiscv64Rvv(int dividend, int divisor) {
    return floorDivisionRiscv64Rvv(dividend + divisor - 1, divisor);
}
#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_RISCV64_RVV_H_ */
