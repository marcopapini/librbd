/*
 *  Component: parallel_amd64_avx512f.c
 *  Parallel RBD management - Optimized using amd64 AVX512F instruction set
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
#include "../parallel_amd64.h"


/**
 * rbdParallelGenericWorkerAvx512f
 *
 * Generic Parallel RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdParallelGenericWorkerAvx512f(struct rbdParallelData *data)
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
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorkerAvx512f
 *
 * Identical Parallel RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdParallelIdenticalWorkerAvx512f(struct rbdParallelData *data)
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
                /* Compute reliability of Parallel RBD at current time instant */
                rbdParallelIdenticalStepVNdAvx512f(mask, data, time);
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
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdParallelGenericStepVNdAvx512f
 *
 * Generic Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelGenericStepVNdAvx512f(__mmask8 mask, struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d vNdTmp;
    __m512d vNdRes;

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(0 * data->numTimes) + time]);
    vNdRes = _mm512_maskz_sub_pd(mask, v8dOnes, vNdRes);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(component * data->numTimes) + time]);
        vNdRes = _mm512_maskz_fnmadd_pd(mask, vNdRes, vNdTmp, vNdRes);
    }
    vNdRes = _mm512_maskz_sub_pd(mask, v8dOnes, vNdRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdParallelIdenticalStepVNdAvx512f
 *
 * Identical Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelIdenticalStepVNdAvx512f(__mmask8 mask, struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d vNdU;
    __m512d vNdRes;

    /* Load unreliability */
    vNdU = _mm512_maskz_loadu_pd(mask, &data->reliabilities[time]);
    vNdU = _mm512_maskz_sub_pd(mask, v8dOnes, vNdU);

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = vNdU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = _mm512_maskz_mul_pd(mask, vNdRes, vNdU);
    }
    vNdRes = _mm512_maskz_sub_pd(mask, v8dOnes, vNdRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
