/*
 *  Component: series_aarch64_sve.c
 *  Series RBD management - Optimized using AArch64 SVE instruction set
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
#include "../series_aarch64.h"


/**
 * rbdSeriesGenericWorkerSve
 *
 * Generic Series RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdSeriesGenericWorkerSve(struct rbdSeriesData *data)
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
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerSve
 *
 * Identical Series RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdSeriesIdenticalWorkerSve(struct rbdSeriesData *data)
{
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepVNdSve
 *
 * Generic Series RBD step function with AArch64 SVE
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdSeriesGenericStepVNdSve(struct rbdSeriesData *data, unsigned int time)
{
    svbool_t pg;
    unsigned char component;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = svld1(pg, &data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = svld1(pg, &data->reliabilities[(component * data->numTimes) + time]);
        vNdRes = svmul_f64_x(pg, vNdRes, vNdTmp);
    }

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdSeriesIdenticalStepVNdSve
 *
 * Identical Series RBD step function with AArch64 SVE
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdSeriesIdenticalStepVNdSve(struct rbdSeriesData *data, unsigned int time)
{
    svbool_t pg;
    unsigned char component;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Load reliability */
    vNdTmp = svld1(pg, &data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = vNdTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = svmul_f64_x(pg, vNdRes, vNdTmp);
    }

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
