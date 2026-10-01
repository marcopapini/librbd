/*
 *  Component: reliability_x86_sse2.c
 *  Reliability RBD management - Optimized using x86 SSE2 instruction set
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
#include "../reliability_x86.h"


/**
 * rbdUnreliabilityWorkerSse2
 *
 * Unreliability RBD Worker function with x86 SSE2 instruction set
 *
 * Input:
 *      struct rbdUnreliabilityWorker *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting x86 SSE2 instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("sse2") void *rbdUnreliabilityWorkerSse2(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    __m128d v2dRes;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + V2D);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + V2D);
        /* Compute unreliability at current time instant */
        v2dRes = _mm_loadu_pd(&data->reliability[time]);
        v2dRes = _mm_sub_pd(v2dOnes, v2dRes);
        _mm_storeu_pd(&data->unreliability[time], capReliabilityV2dSse2(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute unreliability at current time instant */
        v2dRes = _mm_load_sd(&data->reliability[time]);
        v2dRes = _mm_sub_sd(v2dOnes, v2dRes);
        _mm_store_sd(&data->unreliability[time], capReliabilityV2dSse2(v2dRes));
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerSse2
 *
 * Reliability Sum RBD Worker function with x86 SSE2 instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting x86 SSE2 instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("sse2") void *rbdReliabilitySumWorkerSse2(struct rbdReliabilityData *data)
{
    unsigned int time;
    __m128d v2dRes;
    __m128d v2dTmp;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + V2D);
        prefetchRead(data->r2, 1, data->numTimes, time + V2D);
        prefetchWrite(data->output, 1, data->numTimes, time + V2D);
        /* Compute reliability sum at current time instant */
        v2dRes = _mm_loadu_pd(&data->r1[time]);
        v2dTmp = _mm_loadu_pd(&data->r2[time]);
        v2dRes = _mm_add_pd(v2dRes, v2dTmp);
        _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability sum at current time instant */
        v2dRes = _mm_load_sd(&data->r1[time]);
        v2dTmp = _mm_load_sd(&data->r2[time]);
        v2dRes = _mm_add_sd(v2dRes, v2dTmp);
        _mm_store_sd(&data->output[time], capReliabilityV2dSse2(v2dRes));
    }

    return NULL;
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
