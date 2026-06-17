/*
 *  Component: failure_density_aarch64_neon.c
 *  Failure Density computation for RBD management - Optimized using AArch64 NEON instruction set
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


#include "../../generic/rbd_internal_generic.h"

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../failure_density_aarch64.h"


/**
 * rbdFailureDensityWorkerNeon
 *
 * Failure Density Worker function with AArch64 NEON instruction set
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
 *  This function implements the Failure Density Worker with AArch64 NEON instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("+simd") void rbdFailureDensityWorkerNeon(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    float64x2_t v2dR;
    float64x2_t v2dTmp;
    float64x2_t v2dDT;
    float64x1_t v1dR;
    float64x1_t v1dTmp;
    float64x1_t v1dDT;
    unsigned int idx;

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    v2dDT = vdupq_n_f64(deltaT);
    idx = 1;
    while ((idx + V2D) <= numTimes) {
        /* Compute the Failure Density function */
        v2dR = vld1q_f64(&reliability[idx]);
        v2dTmp = vld1q_f64(&reliability[idx - 1]);
        v2dR = vsubq_f64(v2dTmp, v2dR);
        v2dR = vdivq_f64(v2dR, v2dDT);
        v2dR = vmaxnmq_f64(v2dZeros, v2dR);
        vst1q_f64(&failureDensity[idx], v2dR);
        /* Increment current time instant */
        idx += V2D;
    }
    /* Is 1 time instant remaining? */
    if (idx < numTimes) {
        /* Compute the Failure Density function */
        v1dDT = vdup_n_f64(deltaT);
        v1dR = vld1_f64(&reliability[idx]);
        v1dTmp = vld1_f64(&reliability[idx - 1]);
        v1dR = vsub_f64(v1dTmp, v1dR);
        v1dR = vdiv_f64(v1dR, v1dDT);
        v1dR = vmaxnm_f64(v1dZeros, v1dR);
        vst1_f64(&failureDensity[idx], v1dR);
    }
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
