/*
 *  Component: parallel_amd64.c
 *  Parallel RBD management - amd64 platform-specific implementation
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
#include "parallel_amd64.h"
#include "../x86/parallel_x86.h"
#include "../parallel.h"


static void *rbdParallelGenericWorkerAvx512f(struct rbdParallelData *data);
static void *rbdParallelGenericWorkerFma3(struct rbdParallelData *data);
static void *rbdParallelGenericWorkerAvx(struct rbdParallelData *data);
static void *rbdParallelIdenticalWorkerAvx512f(struct rbdParallelData *data);
static void *rbdParallelIdenticalWorkerAvx(void *arg);


/**
 * rbdParallelGenericWorker
 *
 * Parallel RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Parallel RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Parallel RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelGenericWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned int time;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdParallelGenericWorkerAvx512f(data);
    }

    if (amd64Fma3Supported()) {
        return rbdParallelGenericWorkerFma3(data);
    }

    if (amd64AvxSupported()) {
        return rbdParallelGenericWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdParallelGenericWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorker
 *
 * Identical Parallel RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Parallel RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Parallel RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelIdenticalWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned int time;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdParallelIdenticalWorkerAvx512f(data);
    }

    if (amd64AvxSupported()) {
        return rbdParallelIdenticalWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdParallelIdenticalWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdParallelGenericWorkerAvx512f
 *
 * Parallel RBD Worker function with amd64 AVX512F instruction set
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
 *      data: the pointer to a Parallel RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelGenericWorkerAvx512f(struct rbdParallelData *data)
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
 * rbdParallelGenericWorkerFma3
 *
 * Parallel RBD Worker function with amd64 FMA3 instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting amd64 FMA3 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: the pointer to a Parallel RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelGenericWorkerFma3(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV4dFma3(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
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
 * rbdParallelGenericWorkerAvx
 *
 * Parallel RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: the pointer to a Parallel RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelGenericWorkerAvx(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV2dSse2(data, time);
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
 *      data: the pointer to a Parallel RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelIdenticalWorkerAvx512f(struct rbdParallelData *data)
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
 * rbdParallelIdenticalWorkerAvx
 *
 * Identical Parallel RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      data: the pointer to a Parallel RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelIdenticalWorkerAvx(void *arg)
{
    struct rbdParallelData *data;
    unsigned int time;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

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
    }
    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
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

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
