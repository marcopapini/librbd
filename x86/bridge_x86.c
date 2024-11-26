/*
 *  Component: bridge_x86.c
 *  Bridge RBD management - x86 platform-specific implementation
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

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_x86.h"
#include "bridge_x86.h"
#include "../bridge.h"


#if defined(ARCH_X86) && CPU_ENABLE_SIMD != 0

/**
 * rbdBridgeGenericWorker
 *
 * Bridge RBD Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting x86 platform-specific instruction sets.
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

    if (x86Sse2Supported()) {
        return rbdBridgeGenericWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
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
 * Identical Bridge RBD Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting x86 platform-specific instruction sets.
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

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;

    if (x86Sse2Supported()) {
        return rbdBridgeIdenticalWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

#endif /* defined(ARCH_X86) && CPU_ENABLE_SIMD != 0 */

/**
 * rbdBridgeGenericWorkerSse2
 *
 * Bridge RBD Worker function with x86 SSE2 instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting x86 SSE2 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      data: the pointer to a Bridge RBD data
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorkerSse2(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeGenericStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
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

/**
 * rbdBridgeIdenticalWorkerSse2
 *
 * Identical Bridge RBD Worker function with x86 SSE2 instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting x86 SSE2 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      data: the pointer to a Bridge RBD data
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorkerSse2(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV2dSse2(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
    }

    return NULL;
}

#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
