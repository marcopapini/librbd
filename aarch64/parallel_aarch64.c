/*
 *  Component: parallel_aarch64.c
 *  Parallel RBD management - AArch64 platform-specific implementation
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

#if defined(ARCH_AARCH64)
#include "rbd_internal_aarch64.h"
#include "parallel_aarch64.h"
#include "../parallel.h"



/**
 * rbdParallelGenericWorker
 *
 * Parallel RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Parallel RBD Worker exploiting AArch64 NEON instruction set.
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
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

#if CPU_ENABLE_SIMD != 0
    time *= V2D_SIZE;
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D_SIZE) <= timeLimit) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D_SIZE));
        prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (numCores * V2D_SIZE);
    }
    /* Is 1 time instant remaining? */
    if (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
    }

    return NULL;
#endif /* CPU_ENABLE_SIMD */

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorker
 *
 * Identical Parallel RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting AArch64 NEON instruction set.
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
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

#if CPU_ENABLE_SIMD != 0
    time *= V2D_SIZE;
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D_SIZE) <= timeLimit) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D_SIZE));
        prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (numCores * V2D_SIZE);
    }
    /* Is 1 time instant remaining? */
    if (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
    }

    return NULL;
#endif /* CPU_ENABLE_SIMD */

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}

#endif /* defined(ARCH_AARCH64) */
