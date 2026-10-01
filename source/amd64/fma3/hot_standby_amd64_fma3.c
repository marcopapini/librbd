/*
 *  Component: hot_standby_amd64_fma3.c
 *  Hot Stand-by RBD management - Optimized using amd64 FMA3 instruction set
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../hot_standby_amd64.h"
#include "../integral_amd64.h"


static void rbdHotStandbyStepV4dFma3(struct rbdHotStandbyData *data, unsigned int time);
static void rbdHotStandbyStepV2dFma3(struct rbdHotStandbyData *data, unsigned int time);
static void rbdHotStandbyStepV1dFma3(struct rbdHotStandbyData *data, unsigned int time);


/**
 * rbdHotStandbyWorkerFma3
 *
 * Hot Stand-by RBD Worker function with amd64 FMA3 instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting amd64 FMA3 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerFma3(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV4dFma3(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV1dFma3(data, time);
    }

    return NULL;
}

/**
 * rbdHotStandbyStepV4dFma3
 *
 * Hot Stand-by RBD step function with amd64 FMA3 256bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting amd64 FMA3 256bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
static FUNCTION_TARGET("fma") void rbdHotStandbyStepV4dFma3(struct rbdHotStandbyData *data, unsigned int time)
{
    __m256d v4dTmp;
    __m256d v4dPri;
    __m256d v4dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v4dPri = _mm256_loadu_pd(&data->primaryReliability[time]);
    v4dTmp = _mm256_loadu_pd(&data->standbyReliability[time]);
    v4dRes = rbdIntegralHotStandbyV4dFma3(data, time);
    v4dRes = _mm256_fmadd_pd(v4dRes, v4dTmp, v4dPri);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdHotStandbyStepV2dFma3
 *
 * Hot Stand-by RBD step function with amd64 FMA3 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting amd64 FMA3 128bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
static FUNCTION_TARGET("fma") void rbdHotStandbyStepV2dFma3(struct rbdHotStandbyData *data, unsigned int time)
{
    __m128d v2dTmp;
    __m128d v2dPri;
    __m128d v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = _mm_loadu_pd(&data->primaryReliability[time]);
    v2dTmp = _mm_loadu_pd(&data->standbyReliability[time]);
    v2dRes = rbdIntegralHotStandbyV2dFma3(data, time);
    v2dRes = _mm_fmadd_pd(v2dRes, v2dTmp, v2dPri);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dAvx(v2dRes));
}

/**
 * rbdHotStandbyStepV1dFma3
 *
 * Hot Stand-by RBD step function with amd64 FMA3 64bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting amd64 FMA3 64bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
static FUNCTION_TARGET("fma") void rbdHotStandbyStepV1dFma3(struct rbdHotStandbyData *data, unsigned int time)
{
    __m128d v2dTmp;
    __m128d v2dPri;
    __m128d v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = _mm_load_sd(&data->primaryReliability[time]);
    v2dTmp = _mm_load_sd(&data->standbyReliability[time]);
    v2dRes = _mm256_castpd256_pd128(rbdIntegralHotStandbyV1dFma3(data, time));
    v2dRes = _mm_fmadd_sd(v2dRes, v2dTmp, v2dPri);

    /* Cap the computed reliability and set it into output array */
    _mm_store_sd(&data->output[time], capReliabilityV2dAvx(v2dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
