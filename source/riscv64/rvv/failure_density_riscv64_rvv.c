/*
 *  Component: failure_density_riscv64_rvv.c
 *  Failure Density computation for RBD management - Optimized using RISC-V 64bit RVV instruction set
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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


#include "../../generic/rbd_internal_generic.h"

#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_riscv64.h"
#include "../failure_density_riscv64.h"


/**
 * rbdFailureDensityWorkerRvv
 *
 * Failure Density Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      double *reliability
 *      unsigned int numTimes
 *      double deltaT
 *
 * Output:
 *      double *failureDensity
 *
 * Description:
 *  This function implements the Failure Density Worker with RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdFailureDensityWorkerRvv(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdTmp;
    unsigned int idx;
    unsigned long int vl;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(0);

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    idx = 1;
    while (idx < numTimes) {
        vl = __riscv_vsetvl_e64m1(numTimes - idx);
        /* Compute the Failure Density function */
        vNdR = __riscv_vle64_v_f64m1(&reliability[idx], vl);
        vNdTmp = __riscv_vle64_v_f64m1(&reliability[idx - 1], vl);
        vNdR = __riscv_vfsub_vv_f64m1(vNdTmp, vNdR, vl);
        vNdR = __riscv_vfdiv_vf_f64m1(vNdR, deltaT, vl);
        vNdR = __riscv_vfmax_vf_f64m1(vNdR, 0.0, vl);
        __riscv_vse64_v_f64m1(&failureDensity[idx], vNdR, vl);
        /* Increment current time instant */
        idx += vl;
    }
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
