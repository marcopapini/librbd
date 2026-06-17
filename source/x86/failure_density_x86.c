/*
 *  Component: failure_density_x86.c
 *  Failure Density computation for RBD management - x86 platform-specific implementation
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

#if defined(ARCH_X86) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_x86.h"
#include "failure_density_x86.h"
#include "../failure_density.h"


/**
 * rbdFailureDensityWorker
 *
 * Failure Density Worker function with x86 platform-specific instruction sets
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
 *  This function implements the Failure Density Worker exploiting x86 platform-specific instruction sets.
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
    if (x86Sse2Supported()) {
        rbdFailureDensityWorkerSse2(reliability, failureDensity, numTimes, deltaT);
        return;
    }

    rbdFailureDensityWorkerNoarch(reliability, failureDensity, numTimes, deltaT);
}

#endif /* defined(ARCH_X86) && CPU_ENABLE_SIMD != 0 */
