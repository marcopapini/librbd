/*
 *  Component: hot_standby_x86_sse2.c
 *  Hot Stand-by RBD management - Optimized using x86 SSE2 instruction set
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

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_x86.h"
#include "../hot_standby_x86.h"
#include "../integral_x86.h"


static void rbdHotStandbyStepV2dSse2(struct rbdHotStandbyData *data, unsigned int time);
static void rbdHotStandbyStepV1dSse2(struct rbdHotStandbyData *data, unsigned int time);


/**
 * rbdHotStandbyWorkerSse2
 *
 * Hot Stand-by RBD Worker function with x86 SSE2 instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting x86 SSE2 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerSse2(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV2dSse2(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV1dSse2(data, time);
    }

    return NULL;
}

/**
 * rbdHotStandbyStepV2dSse2
 *
 * Hot Stand-by RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
static FUNCTION_TARGET("sse2") void rbdHotStandbyStepV2dSse2(struct rbdHotStandbyData *data, unsigned int time)
{
    __m128d v2dTmp;
    __m128d v2dPri;
    __m128d v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = _mm_loadu_pd(&data->primaryReliability[time]);
    v2dTmp = _mm_loadu_pd(&data->standbyReliability[time]);
    v2dRes = rbdIntegralHotStandbyV2dSse2(data, time);
    v2dRes = _mm_mul_pd(v2dRes, v2dTmp);
    v2dRes = _mm_add_pd(v2dRes, v2dPri);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdHotStandbyStepV1dSse2
 *
 * Hot Stand-by RBD step function with x86 SSE2 64bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting x86 SSE2 64bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
static FUNCTION_TARGET("sse2") void rbdHotStandbyStepV1dSse2(struct rbdHotStandbyData *data, unsigned int time)
{
    __m128d v2dTmp;
    __m128d v2dPri;
    __m128d v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = _mm_load_sd(&data->primaryReliability[time]);
    v2dTmp = _mm_load_sd(&data->standbyReliability[time]);
    v2dRes = rbdIntegralHotStandbyV1dSse2(data, time);
    v2dRes = _mm_mul_sd(v2dRes, v2dTmp);
    v2dRes = _mm_add_sd(v2dRes, v2dPri);

    /* Cap the computed reliability and set it into output array */
    _mm_store_sd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
