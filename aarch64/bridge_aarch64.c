/*
 *  Component: bridge_aarch64.c
 *  Bridge RBD management - AArch64 platform-specific implementation
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


#include "../rbd_internal.h"

#if CPU_AARCH64_NEON != 0
#include "rbd_internal_aarch64.h"
#include "bridge_aarch64.h"
#include "../bridge.h"



/**
 * rbdBridgeGenericWorker
 *
 * Bridge RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Bridge RBD Worker exploiting AArch64 NEON instruction set.
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
__attribute__((visibility ("hidden"))) void *rbdBridgeGenericWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchIdx * V2D_SIZE);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D_SIZE) <= timeLimit) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D_SIZE));
        prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (numCores * V2D_SIZE);
    }
    /* Is 1 time instant remaining? */
    if (time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorker
 *
 * Identical Bridge RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting AArch64 NEON instruction set.
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
__attribute__((visibility ("hidden"))) void *rbdBridgeIdenticalWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchIdx * V2D_SIZE);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D_SIZE) <= timeLimit) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D_SIZE));
        prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (numCores * V2D_SIZE);
    }
    /* Is 1 time instant remaining? */
    if (time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
    }

    return NULL;
}


#endif /* CPU_AARCH64_NEON */
