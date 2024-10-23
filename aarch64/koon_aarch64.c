/*
 *  Component: koon_aarch64.c
 *  KooN (K-out-of-N) RBD management - AArch64 platform-specific implementation
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
#include "koon_aarch64.h"
#include "../koon.h"



/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function with x86 FMA+AVX
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker exploiting x86 FMA+AVX instruction sets.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a fill KooN data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation
 *
 * Return (void *):
 *  NULL
 */
__attribute__((visibility ("hidden"))) void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;
    float64x2_t m128d;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchIdx * V2D_SIZE);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;
    /* Define vector (2d) with provided value */
    m128d = vdupq_n_f64(data->value);

    /* For each time instant (blocks of 2 time instants)... */
    while ((time + V2D_SIZE) <= timeLimit) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
        /* Fill output Reliability array with fixed value */
        vst1q_f64(&data->output[time], m128d);
        /* Increment current time instant */
        time += (numCores * V2D_SIZE);
    }
    /* Is 1 time instant remaining? */
    if (time < timeLimit) {
        /* Fill output Reliability array with fixed value */
        data->output[time++] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorker
 *
 * Generic KooN RBD Worker function with x86 FMA+AVX
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting x86 FMA+AVX instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
__attribute__((visibility ("hidden"))) void *rbdKooNGenericWorker(void *arg)
{
    struct rbdKooNGenericData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchIdx * V2D_SIZE);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    if (data->bRecursive == 0) {
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D_SIZE) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D_SIZE));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNGenericSuccessStepV2dNeon(data, time);
                /* Increment current time instant */
                time += (numCores * V2D_SIZE);
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNGenericSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D_SIZE) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D_SIZE));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNGenericFailStepV2dNeon(data, time);
                /* Increment current time instant */
                time += (numCores * V2D_SIZE);
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNGenericFailStepS1d(data, time);
            }
        }
    }
    else {
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D_SIZE) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D_SIZE));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
            /* Recursively compute reliability of KooN RBD at current time instant */
            rbdKooNRecursionV2dNeon(data, time);
            /* Increment current time instant */
            time += (numCores * V2D_SIZE);
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Recursively compute reliability of KooN RBD at current time instant */
            rbdKooNRecursionS1d(data, time);
        }
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function with x86 FMA+AVX
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worke exploiting x86 FMA+AVX instruction setsr.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a identical KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
__attribute__((visibility ("hidden"))) void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchIdx * V2D_SIZE);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D_SIZE) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D_SIZE));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dNeon(data, time);
            /* Increment current time instant */
            time += (numCores * V2D_SIZE);
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
        }
    }
    else {
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D_SIZE) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D_SIZE));
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D_SIZE));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dNeon(data, time);
            /* Increment current time instant */
            time += (numCores * V2D_SIZE);
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
        }
    }

    return NULL;
}

#endif /* CPU_AARCH64_NEON */