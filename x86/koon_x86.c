/*
 *  Component: koon_x86.c
 *  KooN (K-out-of-N) RBD management - x86 platform-specific implementation
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

#if defined(ARCH_X86) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_x86.h"
#include "koon_x86.h"
#include "../koon.h"


/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker x86 platform-specific instruction sets.
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
HIDDEN FUNCTION_TARGET("sse2") void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;
    __m128d m128d;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (x86Sse2Supported()) {
        time *= V2D;
        /* Define vector (2d) with provided value */
        m128d = _mm_set1_pd(data->value);

        /* For each time instant (blocks of 2 time instants)... */
        while ((time + V2D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
            /* Fill output Reliability array with fixed value */
            _mm_storeu_pd(&data->output[time], m128d);
            /* Increment current time instant */
            time += (data->numCores * V2D);
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Fill output Reliability array with fixed value */
            data->output[time++] = data->value;
        }

        return NULL;
    }

    /* For each time instant... */
    while (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time] = data->value;
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorker
 *
 * Generic KooN RBD Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting x86 platform-specific instruction sets.
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
HIDDEN void *rbdKooNGenericWorker(void *arg)
{
    struct rbdKooNGenericData *data;
    unsigned int time;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (x86Sse2Supported()) {
        time *= V2D;
        if (data->bRecursive == 0) {
            /* If compute unreliability flag is not set... */
            if (data->bComputeUnreliability == 0) {
                /* For each time instant to be processed (blocks of 2 time instants)... */
                while ((time + V2D) <= data->numTimes) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += (data->numCores * V2D);
                }
                /* Is 1 time instant remaining? */
                if (time < data->numTimes) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepS1d(data, time);
                }
            }
            else {
                /* For each time instant to be processed (blocks of 2 time instants)... */
                while ((time + V2D) <= data->numTimes) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += (data->numCores * V2D);
                }
                /* Is 1 time instant remaining? */
                if (time < data->numTimes) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepS1d(data, time);
                }
            }
        }
        else {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D) <= data->numTimes) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV2dSse2(data, time);
                /* Increment current time instant */
                time += (data->numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < data->numTimes) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionS1d(data, time);
            }
        }

        return NULL;
    }

    if (data->bRecursive == 0) {
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed... */
            while (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNGenericSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += data->numCores;
            }
        }
        else {
            /* For each time instant to be processed... */
            while (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNGenericFailStepS1d(data, time);
                /* Increment current time instant */
                time += data->numCores;
            }
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Recursively compute reliability of KooN RBD at current time instant */
            rbdKooNRecursionS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting x86 platform-specific instruction sets.
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
HIDDEN void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;
    unsigned int time;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (x86Sse2Supported()) {
        time *= V2D;
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D) <= data->numTimes) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dSse2(data, time);
                /* Increment current time instant */
                time += (data->numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D) <= data->numTimes) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += (data->numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
            }
        }

        return NULL;
    }

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
    }

    return NULL;
}


#endif /* defined(ARCH_X86) && CPU_ENABLE_SIMD != 0 */
