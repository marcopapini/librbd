/*
 *  Component: failure_density_power8_vsx.c
 *  Failure Density computation for RBD management - Optimized using POWER8 VSX instruction set
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

#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_power8.h"
#include "../failure_density_power8.h"


/**
 * rbdFailureDensityWorkerVsx
 *
 * Failure Density Worker function with POWER8 VSX instruction set
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
 *  This function implements the Failure Density Worker with POWER8 VSX instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdFailureDensityWorkerVsx(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    double64x2 v2dR;
    double64x2 v2dTmp;
    double64x2 v2dDT;
    unsigned int idx;

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    v2dDT = vec_splats(deltaT);
    idx = 1;
    while ((idx + V2D) <= numTimes) {
        /* Compute the Failure Density function */
        v2dR = vectorLoad(&reliability[idx]);
        v2dTmp = vectorLoad(&reliability[idx - 1]);
        v2dR = vec_sub(v2dTmp, v2dR);
        v2dR = vec_div(v2dR, v2dDT);
        v2dR = vec_max(v2dZeros, v2dR);
        vectorStore(&failureDensity[idx], v2dR);
        /* Increment current time instant */
        idx += V2D;
    }
    /* Is 1 time instant remaining? */
    if (idx < numTimes) {
        /* Compute the Failure Density function */
        v2dDT = vec_promote(deltaT, 0);
        v2dR = vec_promote(reliability[idx], 0);
        v2dTmp = vec_promote(reliability[idx - 1], 0);
        v2dR = vec_sub(v2dTmp, v2dR);
        v2dR = vec_div(v2dR, v2dDT);
        v2dR = vec_max(v2dZeros, v2dR);
        failureDensity[idx] = vec_extract(v2dR, 0);
    }
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
