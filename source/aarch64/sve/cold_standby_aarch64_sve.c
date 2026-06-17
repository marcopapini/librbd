/*
 *  Component: cold_standby_aarch64_sve.c
 *  Cold Stand-by RBD management - Optimized using AArch64 SVE instruction set
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
#include "../cold_standby_aarch64.h"
#include "../integral_aarch64.h"


/**
 * rbdColdStandbyWorkerSve
 *
 * Cold Stand-by RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Cold Stand-by RBD system
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdColdStandbyWorkerSve(struct rbdColdStandbyData *data)
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
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + vectorSize);
        prefetchRead(data->primaryFailureDensity, 1, data->numTimes, time + vectorSize);
        prefetchRead(data->standbyReliabilityRev, 1, data->numTimes, time + vectorSize);
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepVNdSve(pg, data, time);
        /* Increment current time instant */
        time += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

#if !defined(COMPILER_VS)
/**
 * rbdColdStandbyStepVNdSve
 *
 * Cold Stand-by RBD step function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Cold Stand-by block
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdColdStandbyStepVNdSve(svbool_t pg, struct rbdColdStandbyData *data, unsigned int time)
{
    svfloat64_t vNdTmp;
    svfloat64_t vNdPri;
    svfloat64_t vNdRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    vNdPri = svld1(pg, &data->primaryReliability[time]);
    vNdRes = svdup_f64(data->pSwitch);
    vNdTmp = rbdIntegralColdStandbyVNdSve(pg, data, time);
    vNdRes = svmla_f64_x(pg, vNdPri, vNdRes, vNdTmp);

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
