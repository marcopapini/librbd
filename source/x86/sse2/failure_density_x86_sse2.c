/*
 *  Component: failure_density_x86_sse2.c
 *  Failure Density computation for RBD management - Optimized using x86 SSE2 instruction set
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

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_x86.h"
#include "../failure_density_x86.h"


/**
 * rbdFailureDensityWorkerSse2
 *
 * Failure Density Worker function with x86 SSE2 instruction set
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
 *  This function implements the Failure Density Worker with x86 SSE2 instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdFailureDensityWorkerSse2(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    __m128d v2dR;
    __m128d v2dTmp;
    __m128d v2dDT;
    unsigned int idx;

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    v2dDT = _mm_load1_pd(&deltaT);
    idx = 1;
    while ((idx + V2D) <= numTimes) {
        /* Compute the Failure Density function */
        v2dR = _mm_loadu_pd(&reliability[idx]);
        v2dTmp = _mm_loadu_pd(&reliability[idx - 1]);
        v2dR = _mm_sub_pd(v2dTmp, v2dR);
        v2dR = _mm_div_pd(v2dR, v2dDT);
        v2dR = _mm_max_pd(v2dR, v2dZeros);
        _mm_storeu_pd(&failureDensity[idx], v2dR);
        /* Increment current time instant */
        idx += V2D;
    }
    /* Is 1 time instant remaining? */
    if (idx < numTimes) {
        /* Compute the Failure Density function */
        v2dR = _mm_load_sd(&reliability[idx]);
        v2dTmp = _mm_load_sd(&reliability[idx - 1]);
        v2dR = _mm_sub_sd(v2dTmp, v2dR);
        v2dR = _mm_div_sd(v2dR, v2dDT);
        v2dR = _mm_max_sd(v2dR, v2dZeros);
        _mm_store_sd(&failureDensity[idx], v2dR);
    }
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
