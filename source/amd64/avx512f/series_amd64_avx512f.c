/*
 *  Component: series_amd64_avx512f.c
 *  Series RBD management - Optimized using amd64 AVX512F instruction set
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../series_amd64.h"
#include "../../x86/series_x86.h"


/**
 * rbdSeriesGenericWorkerAvx512f
 *
 * Generic Series RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdSeriesGenericWorkerAvx512f(struct rbdSeriesData *data)
{
    unsigned int time;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerAvx512f
 *
 * Identical Series RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdSeriesIdenticalWorkerAvx512f(struct rbdSeriesData *data)
{
    unsigned int time;
    unsigned int alignSteps;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Are there at least 8 - 1 time instants to process? */
    if ((time + V8D) < data->numTimes) {
        /* Align, if possible, to vector size */
        if (((uintptr_t)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            /* Compute the number of doubles to align to vector size */
            alignSteps = ((uintptr_t)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) / sizeof(double);
            alignSteps = (V8D - alignSteps) & (V8D - 1);
            if (alignSteps > 0) {
                /* Compute mask for the management of the head */
                mask = (__mmask8)_cvtu32_mask16((1U << alignSteps) - 1);
                /* Compute reliability of Series RBD at current time instant */
                rbdSeriesIdenticalStepVNdAvx512f(mask, data, time);
                /* Increment current time instant */
                time += alignSteps;
            }
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepVNdAvx512f
 *
 * Generic Series RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdSeriesGenericStepVNdAvx512f(__mmask8 mask, struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m512d vNdTmp;
    __m512d vNdRes;

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(component * data->numTimes) + time]);
        vNdRes = _mm512_maskz_mul_pd(mask, vNdRes, vNdTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdSeriesIdenticalStepVNdAvx512f
 *
 * Identical Series RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdSeriesIdenticalStepVNdAvx512f(__mmask8 mask, struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m512d vNdTmp;
    __m512d vNdRes;

    /* Load reliability */
    vNdTmp = _mm512_maskz_loadu_pd(mask, &data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = vNdTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = _mm512_maskz_mul_pd(mask, vNdRes, vNdTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
