/*
 *  Component: parallel_amd64_avx512f.c
 *  Parallel RBD management - Optimized using amd64 AVX512F instruction set
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
#include "../parallel_amd64.h"
#include "../../x86/parallel_x86.h"


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
HIDDEN void *rbdParallelGenericWorkerAvx512f(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV4dFma3(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
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
HIDDEN void *rbdParallelIdenticalWorkerAvx512f(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Parallel RBD at current time instant */
            rbdParallelIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
        if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Parallel RBD at current time instant */
            rbdParallelIdenticalStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Parallel RBD at current time instant from working components */
            rbdParallelIdenticalStepV4dAvx(data, time);
            /* Increment current time instant */
            time += V4D;
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV4dAvx(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdParallelGenericStepV8dAvx512f
 *
 * Generic Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
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
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelGenericStepV8dAvx512f(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dTmp;
    __m512d v8dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v8dRes = _mm512_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v8dTmp = _mm512_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v8dRes = _mm512_fnmadd_pd(v8dRes, v8dTmp, v8dRes);
    }
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdParallelIdenticalStepV8dAvx512f
 *
 * Identical Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
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
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelIdenticalStepV8dAvx512f(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dU;
    __m512d v8dRes;

    /* Load unreliability */
    v8dU = _mm512_loadu_pd(&data->reliabilities[time]);
    v8dU = _mm512_sub_pd(v8dOnes, v8dU);

    /* Compute reliability of Parallel RBD at current time instant */
    v8dRes = v8dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v8dRes = _mm512_mul_pd(v8dRes, v8dU);
    }
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
