/*
 *  Component: failure_density_noarch.c
 *  Failure Density computation for RBD management - Platform-independent implementation
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


#include "../generic/rbd_internal_generic.h"

#include "../failure_density.h"


#if defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0
/**
 * rbdFailureDensityWorker
 *
 * Failure Density Worker function
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
 *  This function implements the Failure Density Worker.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN void rbdFailureDensityWorker(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    rbdFailureDensityWorkerNoarch(reliability, failureDensity, numTimes, deltaT);
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdFailureDensityWorkerNoarch
 *
 * Failure Density Worker function with platform-independent instruction sets
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
 *  This function implements the Failure Density Worker with platform-independent instruction sets.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN void rbdFailureDensityWorkerNoarch(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    double s1dR;
    unsigned int idx;

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    for (idx = 1; idx < numTimes; ++idx) {
        /* Compute the Failure Density function */
        s1dR = (reliability[idx - 1] - reliability[idx]) / deltaT;
        failureDensity[idx] = fmax(0.0, s1dR);
    }
}
