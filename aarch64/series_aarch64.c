/*
 *  Component: series_aarch64.c
 *  Series RBD management - AArch64 platform-specific implementation
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

#if defined(ARCH_AARCH64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_aarch64.h"
#include "series_aarch64.h"
#include "../series.h"


/**
 * rbdSeriesGenericWorker
 *
 * Generic Series RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting AArch64 NEON instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a
 *                      void pointer to allow SMP computation of Series RBD
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
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesGenericStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorker
 *
 * Identical Series RBD Worker function with AArch64 NEON
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting AArch64 NEON instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a
 *                      void pointer to allow SMP computation of Series RBD
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
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepS1d(data, time);
    }

    return NULL;
}

#endif /* defined(ARCH_AARCH64) && CPU_ENABLE_SIMD != 0 */
