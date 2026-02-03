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
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

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
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdParallelGenericStepVNdSve
 *
 * Generic Parallel RBD step function with AArch64 SVE
 *
 * Input:
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
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdParallelGenericStepVNdSve(struct rbdParallelData *data, unsigned int time)
{
    svbool_t pg;
    unsigned char component;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

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
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdParallelIdenticalStepVNdSve(struct rbdParallelData *data, unsigned int time)
{
    svbool_t pg;
    unsigned char component;
    svfloat64_t vNdU;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

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


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
