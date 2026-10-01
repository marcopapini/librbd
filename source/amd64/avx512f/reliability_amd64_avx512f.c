/*
 *  Component: reliability_amd64_avx512f.c
 *  Reliability RBD management - Optimized using amd64 AVX512F instruction set
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
#include "../reliability_amd64.h"


/**
 * rbdUnreliabilityWorkerAvx
 *
 * Unreliability RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdUnreliabilityWorkerAvx512f(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    __m512d v8dRes;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + V8D);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + V8D);
        /* Compute unreliability at current time instant */
        v8dRes = _mm512_loadu_pd(&data->reliability[time]);
        v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);
        _mm512_storeu_pd(&data->unreliability[time], capReliabilityVNdAvx512f((__mmask8)0xFFU, v8dRes));
        /* Increment current time instant */
        time += V8D;
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute unreliability at current time instant */
        v8dRes = _mm512_maskz_loadu_pd(mask, &data->reliability[time]);
        v8dRes = _mm512_maskz_sub_pd(mask, v8dOnes, v8dRes);
        _mm512_mask_storeu_pd(&data->unreliability[time], mask, capReliabilityVNdAvx512f(mask, v8dRes));
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerAvx512f
 *
 * Reliability Sum RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdReliabilitySumWorkerAvx512f(struct rbdReliabilityData *data)
{
    unsigned int time;
    __m512d v8dRes;
    __m512d v8dTmp;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + V8D);
        prefetchRead(data->r2, 1, data->numTimes, time + V8D);
        prefetchWrite(data->output, 1, data->numTimes, time + V8D);
        /* Compute reliability sum at current time instant */
        v8dRes = _mm512_loadu_pd(&data->r1[time]);
        v8dTmp = _mm512_loadu_pd(&data->r2[time]);
        v8dRes = _mm512_add_pd(v8dRes, v8dTmp);
        _mm512_storeu_pd(&data->output[time], capReliabilityVNdAvx512f((__mmask8)0xFFU, v8dRes));
        /* Increment current time instant */
        time += V8D;
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability sum at current time instant */
        v8dRes = _mm512_maskz_loadu_pd(mask, &data->r1[time]);
        v8dTmp = _mm512_maskz_loadu_pd(mask, &data->r2[time]);
        v8dRes = _mm512_maskz_add_pd(mask, v8dRes, v8dTmp);
        _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, v8dRes));
    }

    return NULL;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
