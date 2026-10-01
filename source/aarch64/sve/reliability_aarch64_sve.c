/*
 *  Component: reliability_aarch64_sve.c
 *  Reliability RBD management - Optimized using AArch64 SVE instruction set
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
#include "../reliability_aarch64.h"



#if !defined(COMPILER_VS)
/**
 * rbdUnreliabilityWorkerSve
 *
 * Unreliability RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdUnreliabilityWorkerSve(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    unsigned int vectorSize;
    svbool_t pg;
    svfloat64_t vNdRes;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(0);

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        pg = svwhilelt_b64(time, data->numTimes);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + vectorSize);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + vectorSize);
        /* Compute unreliability at current time instant */
        vNdRes = svld1(pg, &data->reliability[time]);
        vNdRes = svsub_f64_x(pg, svdup_f64(1.0), vNdRes);
        svst1(pg, &data->unreliability[time], capReliabilityVNdSve(pg, vNdRes));
        /* Increment current time instant */
        time += vectorSize;
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerSve
 *
 * Reliability Sum RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdReliabilitySumWorkerSve(struct rbdReliabilityData *data)
{
    unsigned int time;
    unsigned int vectorSize;
    svbool_t pg;
    svfloat64_t vNdRes;
    svfloat64_t vNdTmp;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(0);

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        pg = svwhilelt_b64(time, data->numTimes);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + vectorSize);
        prefetchRead(data->r2, 1, data->numTimes, time + vectorSize);
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Compute reliability sum at current time instant */
        vNdRes = svld1(pg, &data->r1[time]);
        vNdTmp = svld1(pg, &data->r2[time]);
        vNdRes = svadd_f64_x(pg, vNdRes, vNdTmp);
        svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
        /* Increment current time instant */
        time += vectorSize;
    }

    return NULL;
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
