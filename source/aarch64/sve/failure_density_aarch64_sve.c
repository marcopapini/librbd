/*
 *  Component: failure_density_aarch64_sve.c
 *  Failure Density computation for RBD management - Optimized using AArch64 NEON instruction set
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

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../failure_density_aarch64.h"


/**
 * rbdFailureDensityWorkerSve
 *
 * Failure Density Worker function with AArch64 SVE instruction set
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
 *  This function implements the Failure Density Worker with AArch64 SVE instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdFailureDensityWorkerSve(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
#if !defined(COMPILER_VS)
    svfloat64_t vNdR;
    svfloat64_t vNdZeros;
    svfloat64_t vNdTmp;
    svfloat64_t vNdDT;
    svbool_t pg;
    unsigned int idx;
    unsigned int vectorSize;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(0);

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    vNdZeros = svdup_n_f64(0.0);
    vNdDT = svdup_n_f64(deltaT);
    idx = 1;
    while (idx < numTimes) {
        pg = svwhilelt_b64(idx, numTimes);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Compute the Failure Density function */
        vNdR = svld1(pg, &reliability[idx]);
        vNdTmp = svld1(pg, &reliability[idx - 1]);
        vNdR = svsub_f64_x(pg, vNdTmp, vNdR);
        vNdR = svdiv_f64_x(pg, vNdR, vNdDT);
        vNdR = svmaxnm_f64_x(pg, vNdZeros, vNdR);
        svst1(pg, &failureDensity[idx], vNdR);
        /* Increment current time instant */
        idx += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
