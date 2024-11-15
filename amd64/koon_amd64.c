/*
 *  Component: koon_amd64.c
 *  KooN (K-out-of-N) RBD management - amd64 platform-specific implementation
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

#if defined(ARCH_AMD64)
#include "rbd_internal_amd64.h"
#include "koon_amd64.h"
#include "../koon.h"


/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker amd64 platform-specific instruction sets.
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
HIDDEN
#if CPU_ENABLE_SIMD != 0
FUNCTION_TARGET("avx512f")
#endif /* CPU_ENABLE_SIMD */
void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;
#if CPU_ENABLE_SIMD != 0
    __m512d m512d;
    __m256d m256d;
    __m128d m128d;
#endif /* CPU_ENABLE_SIMD */

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

#if CPU_ENABLE_SIMD != 0
    if (x86Avx512fSupported()) {
        time *= V8D;
        /* Define vector (8d, 4d and 2d) with provided value */
        m512d = _mm512_set1_pd(data->value);
        m256d = _mm256_set1_pd(data->value);
        m128d = _mm_set1_pd(data->value);

        /* For each time instant (blocks of 8 time instants)... */
        while ((time + V8D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
            /* Fill output Reliability array with fixed value */
            _mm512_storeu_pd(&data->output[time], m512d);
            /* Increment current time instant */
            time += (numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= timeLimit) {
            /* Fill output Reliability array with fixed value */
            _mm256_storeu_pd(&data->output[time], m256d);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= timeLimit) {
            /* Fill output Reliability array with fixed value */
            _mm_storeu_pd(&data->output[time], m128d);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Fill output Reliability array with fixed value */
            data->output[time++] = data->value;
        }

        return NULL;
    }

    if (x86AvxSupported()) {
        time *= V4D;
        /* Define vectors (4d and 2d) with provided value */
        m256d = _mm256_set1_pd(data->value);
        m128d = _mm_set1_pd(data->value);

        /* For each time instant (blocks of 4 time instants)... */
        while ((time + V4D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
            /* Fill output Reliability array with fixed value */
            _mm256_storeu_pd(&data->output[time], m256d);
            /* Increment current time instant */
            time += (numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= timeLimit) {
            /* Fill output Reliability array with fixed value */
            _mm_storeu_pd(&data->output[time], m128d);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Fill output Reliability array with fixed value */
            data->output[time++] = data->value;
        }

        return NULL;
    }

    if (x86Sse2Supported()) {
        time *= V2D;
        /* Define vector (2d) with provided value */
        m128d = _mm_set1_pd(data->value);

        /* For each time instant (blocks of 2 time instants)... */
        while ((time + V2D) <= timeLimit) {
            /* Prefetch for next iteration */
            prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
            /* Fill output Reliability array with fixed value */
            _mm_storeu_pd(&data->output[time], m128d);
            /* Increment current time instant */
            time += (numCores * V2D);
        }
        /* Is 1 time instant remaining? */
        if (time < timeLimit) {
            /* Fill output Reliability array with fixed value */
            data->output[time++] = data->value;
        }

        return NULL;
    }
#endif /* CPU_ENABLE_SIMD */

    /* For each time instant... */
    while (time < timeLimit) {
        /* Fill output Reliability array with fixed value */
        data->output[time] = data->value;
        time += numCores;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorker
 *
 * Generic KooN RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting amd64 platform-specific instruction sets.
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
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

#if CPU_ENABLE_SIMD != 0
    if (x86Avx512fSupported()) {
        time *= V8D;
        if (data->bRecursive == 0) {
            /* If compute unreliability flag is not set... */
            if (data->bComputeUnreliability == 0) {
                /* For each time instant to be processed (blocks of 8 time instants)... */
                while ((time + V8D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V8D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV8dAvx512f(data, time);
                    /* Increment current time instant */
                    time += (numCores * V8D);
                }
                /* Are (at least) 4 time instants remaining? */
                if ((time + V4D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV4dFma(data, time);
                    /* Increment current time instant */
                    time += V4D;
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV2dFma(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepS1d(data, time);
                }
            }
            else {
                /* For each time instant to be processed (blocks of 8 time instants)... */
                while ((time + V8D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V8D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV8dAvx512f(data, time);
                    /* Increment current time instant */
                    time += (numCores * V8D);
                }
                /* Are (at least) 4 time instants remaining? */
                if ((time + V4D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV4dFma(data, time);
                    /* Increment current time instant */
                    time += V4D;
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV2dFma(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepS1d(data, time);
                }
            }
        }
        else {
            /* For each time instant to be processed (blocks of 8 time instants)... */
            while ((time + V8D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V8D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV8dAvx512f(data, time);
                /* Increment current time instant */
                time += (numCores * V8D);
            }
            /* Are (at least) 4 time instants remaining? */
            if ((time + V4D) <= timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV4dFma(data, time);
                /* Increment current time instant */
                time += V4D;
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV2dFma(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86FmaSupported()) {
        time *= V4D;
        if (data->bRecursive == 0) {
            /* If compute unreliability flag is not set... */
            if (data->bComputeUnreliability == 0) {
                /* For each time instant to be processed (blocks of 4 time instants)... */
                while ((time + V4D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV4dFma(data, time);
                    /* Increment current time instant */
                    time += (numCores * V4D);
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV2dFma(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepS1d(data, time);
                }
            }
            else {
                /* For each time instant to be processed (blocks of 4 time instants)... */
                while ((time + V4D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV4dFma(data, time);
                    /* Increment current time instant */
                    time += (numCores * V4D);
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV2dFma(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepS1d(data, time);
                }
            }
        }
        else {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV4dFma(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV2dFma(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86AvxSupported()) {
        time *= V4D;
        if (data->bRecursive == 0) {
            /* If compute unreliability flag is not set... */
            if (data->bComputeUnreliability == 0) {
                /* For each time instant to be processed (blocks of 4 time instants)... */
                while ((time + V4D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV4dAvx(data, time);
                    /* Increment current time instant */
                    time += (numCores * V4D);
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepS1d(data, time);
                }
            }
            else {
                /* For each time instant to be processed (blocks of 4 time instants)... */
                while ((time + V4D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV4dAvx(data, time);
                    /* Increment current time instant */
                    time += (numCores * V4D);
                }
                /* Are (at least) 2 time instants remaining? */
                if ((time + V2D) <= timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += V2D;
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepS1d(data, time);
                }
            }
        }
        else {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV4dAvx(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86Sse2Supported()) {
        time *= V2D;
        if (data->bRecursive == 0) {
            /* If compute unreliability flag is not set... */
            if (data->bComputeUnreliability == 0) {
                /* For each time instant to be processed (blocks of 2 time instants)... */
                while ((time + V2D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += (numCores * V2D);
                }
                /* Is 1 time instant remaining? */
                if (time < timeLimit) {
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNGenericSuccessStepS1d(data, time);
                }
            }
            else {
                /* For each time instant to be processed (blocks of 2 time instants)... */
                while ((time + V2D) <= timeLimit) {
                    /* Prefetch for next iteration */
                    prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D));
                    prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNGenericFailStepV2dSse2(data, time);
                    /* Increment current time instant */
                    time += (numCores * V2D);
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
            while ((time + V2D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionV2dSse2(data, time);
                /* Increment current time instant */
                time += (numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Recursively compute reliability of KooN RBD at current time instant */
                rbdKooNRecursionS1d(data, time);
            }
        }

        return NULL;
    }
#endif /* CPU_ENABLE_SIMD */

    if (data->bRecursive == 0) {
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed... */
            while (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNGenericSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += numCores;
            }
        }
        else {
            /* For each time instant to be processed... */
            while (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNGenericFailStepS1d(data, time);
                /* Increment current time instant */
                time += numCores;
            }
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < timeLimit) {
            /* Recursively compute reliability of KooN RBD at current time instant */
            rbdKooNRecursionS1d(data, time);
            /* Increment current time instant */
            time += numCores;
        }
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting amd64 platform-specific instruction sets.
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
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

#if CPU_ENABLE_SIMD != 0
    if (x86Avx512fSupported()) {
        time *= V8D;
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 8 time instants)... */
            while ((time + V8D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V8D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV8dAvx512f(data, time);
                /* Increment current time instant */
                time += (numCores * V8D);
            }
            /* Are (at least) 4 time instants remaining? */
            if ((time + V4D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV4dFma(data, time);
                /* Increment current time instant */
                time += V4D;
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dFma(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 8 time instants)... */
            while ((time + V8D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V8D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V8D));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV8dAvx512f(data, time);
                /* Increment current time instant */
                time += (numCores * V8D);
            }
            /* Are (at least) 4 time instants remaining? */
            if ((time + V4D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV4dAvx(data, time);
                /* Increment current time instant */
                time += V4D;
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86FmaSupported()) {
        time *= V4D;
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV4dFma(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dFma(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV4dAvx(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86AvxSupported()) {
        time *= V4D;
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV4dAvx(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 4 time instants)... */
            while ((time + V4D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V4D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V4D));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV4dAvx(data, time);
                /* Increment current time instant */
                time += (numCores * V4D);
            }
            /* Are (at least) 2 time instants remaining? */
            if ((time + V2D) <= timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
            }
        }

        return NULL;
    }

    if (x86Sse2Supported()) {
        time *= V2D;
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dSse2(data, time);
                /* Increment current time instant */
                time += (numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
            }
        }
        else {
            /* For each time instant to be processed (blocks of 2 time instants)... */
            while ((time + V2D) <= timeLimit) {
                /* Prefetch for next iteration */
                prefetchRead(data->reliabilities, 1, data->numTimes, time + (numCores * V2D));
                prefetchWrite(data->output, 1, data->numTimes, time + (numCores * V2D));
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += (numCores * V2D);
            }
            /* Is 1 time instant remaining? */
            if (time < timeLimit) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
            }
        }

        return NULL;
    }
#endif /* CPU_ENABLE_SIMD */

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed... */
        while (time < timeLimit) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
            /* Increment current time instant */
            time += numCores;
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < timeLimit) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
            /* Increment current time instant */
            time += numCores;
        }
    }

    return NULL;
}


#endif /* defined(ARCH_AMD64) */
