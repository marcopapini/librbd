/*
 *  Component: parallel_aarch64_sve.c
 *  Parallel RBD management - Optimized using AArch64 SVE instruction set
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
#include "../parallel_aarch64.h"


/**
 * rbdParallelGenericWorkerSve
 *
 * Generic Parallel RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdParallelGenericWorkerSve(struct rbdParallelData *data)
{
#if !defined(COMPILER_VS)
    unsigned int batchSize;
    unsigned int time;
    unsigned int vectorSize;
    unsigned int timeEnd;
    svbool_t pg;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivision(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimum(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        pg = svwhilelt_b64(time, timeEnd);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + vectorSize);
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepVNdSve(pg, data, time);
        /* Increment current time instant */
        time += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

/**
 * rbdParallelIdenticalWorkerSve
 *
 * Identical Parallel RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdParallelIdenticalWorkerSve(struct rbdParallelData *data)
{
#if !defined(COMPILER_VS)
    unsigned int batchSize;
    unsigned int time;
    unsigned int vectorSize;
    unsigned int timeEnd;
    svbool_t pg;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivision(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimum(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        pg = svwhilelt_b64(time, timeEnd);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + vectorSize);
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepVNdSve(pg, data, time);
        /* Increment current time instant */
        time += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

#if !defined(COMPILER_VS)
/**
 * rbdParallelGenericStepVNdSve
 *
 * Generic Parallel RBD step function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdParallelGenericStepVNdSve(svbool_t pg, struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRes;

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = svld1(pg, &data->reliabilities[(0 * data->numTimes) + time]);
    vNdRes = svsub_f64_x(pg, svdup_f64(1.0), vNdRes);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = svld1(pg, &data->reliabilities[(component * data->numTimes) + time]);
        vNdRes = svmls_f64_x(pg, vNdRes, vNdRes, vNdTmp);
    }
    vNdRes = svsub_f64_x(pg, svdup_f64(1.0), vNdRes);

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdParallelIdenticalStepVNdSve
 *
 * Identical Parallel RBD step function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdParallelIdenticalStepVNdSve(svbool_t pg, struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    svfloat64_t vNdU;
    svfloat64_t vNdRes;

    /* Load unreliability */
    vNdU = svld1(pg, &data->reliabilities[time]);
    vNdU = svsub_f64_x(pg, svdup_f64(1.0), vNdU);

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = vNdU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = svmul_f64_x(pg, vNdRes, vNdU);
    }
    vNdRes = svsub_f64_x(pg, svdup_f64(1.0), vNdRes);

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
