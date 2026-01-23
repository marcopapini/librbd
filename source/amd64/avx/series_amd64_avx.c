/*
 *  Component: series_amd64_avx.c
 *  Series RBD management - Optimized using amd64 AVX instruction set
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
 * rbdSeriesGenericWorkerAvx
 *
 * Generic Series RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorkerAvx(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerAvx
 *
 * Identical Series RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorkerAvx(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
        if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesIdenticalStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
    }
    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepV4dAvx
 *
 * Generic Series RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdSeriesGenericStepV4dAvx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Compute reliability of Series RBD at current time instant */
    v4dRes = _mm256_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v4dTmp = _mm256_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdSeriesIdenticalStepV4dAvx
 *
 * Identical Series RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdSeriesIdenticalStepV4dAvx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Load reliability */
    v4dTmp = _mm256_loadu_pd(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v4dRes = v4dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
