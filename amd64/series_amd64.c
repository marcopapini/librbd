/*
 *  Component: series_amd64.c
 *  Series RBD management - amd64 platform-specific implementation
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


#include "../generic/rbd_internal_generic.h"

#if defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_amd64.h"
#include "series_amd64.h"
#include "../x86/series_x86.h"
#include "../series.h"


static void *rbdSeriesGenericWorkerAvx512f(struct rbdSeriesData *data);
static void *rbdSeriesGenericWorkerAvx(struct rbdSeriesData *data);
static void *rbdSeriesIdenticalWorkerAvx512f(struct rbdSeriesData *data);
static void *rbdSeriesIdenticalWorkerAvx(struct rbdSeriesData *data);


/**
 * rbdSeriesGenericWorker
 *
 * Generic Series RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorker(void *arg)
{
    struct rbdSeriesData *data;
    unsigned int time;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdSeriesGenericWorkerAvx512f(data);
    }

    if (amd64AvxSupported()) {
        return rbdSeriesGenericWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdSeriesGenericWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepS1d(data, time);
        /* Increment current time instant */
        time += data->numTimes;
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorker
 *
 * Identical Series RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorker(void *arg)
{
    struct rbdSeriesData *data;
    unsigned int time;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (amd64Avx512fSupported()) {
        return rbdSeriesIdenticalWorkerAvx512f(data);
    }

    if (amd64AvxSupported()) {
        return rbdSeriesIdenticalWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdSeriesIdenticalWorkerSse2(data);
    }

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

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
 *      data: the pointer to a Series RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdSeriesGenericWorkerAvx512f(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
        if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepV4dAvx(data, time);
            /* Increment current time instant */
            time += V4D;
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV4dAvx(data, time);
        /* Increment current time instant */
        time += V4D;
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
 *      data: the pointer to a Series RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdSeriesGenericWorkerAvx(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
        if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
    }
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
 *      data: the pointer to a Series RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdSeriesIdenticalWorkerAvx512f(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

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
        if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesIdenticalStepV4dAvx(data, time);
            /* Increment current time instant */
            time += V4D;
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV4dAvx(data, time);
        /* Increment current time instant */
        time += V4D;
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
 *      data: the pointer to a Series RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdSeriesIdenticalWorkerAvx(struct rbdSeriesData *data)
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

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
