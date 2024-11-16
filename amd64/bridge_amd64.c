/*
 *  Component: bridge_amd64.c
 *  Bridge RBD management - amd64 platform-specific implementation
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
#include "bridge_amd64.h"
#include "../bridge.h"


/**
 * rbdBridgeGenericWorker
 *
 * Bridge RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Bridge RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (x86Avx512fSupported()) {
        time *= V8D;
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV4dFma(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV2dFma(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepS1d(data, time);
        }

        return NULL;
    }

    if (x86FmaSupported()) {
        time *= V4D;
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV4dFma(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV2dFma(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepS1d(data, time);
        }

        return NULL;
    }

    if (x86AvxSupported()) {
        time *= V4D;
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV4dAvx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepS1d(data, time);
        }

        return NULL;
    }

    if (x86Sse2Supported()) {
        time *= V2D;
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepV2dSse2(data, time);
            /* Increment current time instant */
            time += (data->numCores * V2D);
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepS1d(data, time);
        }

        return NULL;
    }

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepS1d(data, time);
        /* Increment current time instant */
        time += data->numTimes;
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorker
 *
 * Identical Bridge RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    if (x86Avx512fSupported()) {
        time *= V8D;
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV4dFma(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV2dFma(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
        }

        return NULL;
    }

    if (x86FmaSupported()) {
        time *= V4D;
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV4dFma(data, time);
            /* Increment current time instant */
            time += (numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV2dFma(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
        }

        return NULL;
    }

    if (x86AvxSupported()) {
        time *= V4D;
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV4dAvx(data, time);
            /* Increment current time instant */
            time += (numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
        }

        return NULL;
    }

    if (x86Sse2Supported()) {
        time *= V2D;
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV2dSse2(data, time);
            /* Increment current time instant */
            time += (numCores * V2D);
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
        }

        return NULL;
    }

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
