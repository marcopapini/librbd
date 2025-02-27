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

#if defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_amd64.h"
#include "koon_amd64.h"
#include "../x86/koon_x86.h"
#include "../koon.h"


static void *rbdKooNFillWorkerAvx512f(struct rbdKooNFillData *data);
static void *rbdKooNFillWorkerAvx(struct rbdKooNFillData *data);
static void *rbdKooNGenericWorkerAvx512f(struct rbdKooNGenericData *data);
static void *rbdKooNGenericWorkerFma3(struct rbdKooNGenericData *data);
static void *rbdKooNGenericWorkerAvx(struct rbdKooNGenericData *data);
static void *rbdKooNIdenticalWorkerAvx512f(struct rbdKooNIdenticalData *data);
static void *rbdKooNIdenticalWorkerFma3(struct rbdKooNIdenticalData *data);
static void *rbdKooNIdenticalWorkerAvx(struct rbdKooNIdenticalData *data);


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
HIDDEN void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;

    /* Retrieve fill KooN RBD data */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (amd64Avx512fSupported()) {
        return rbdKooNFillWorkerAvx512f(data);
    }

    if (amd64AvxSupported()) {
        return rbdKooNFillWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdKooNFillWorkerSse2(data);
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

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdKooNGenericWorkerAvx512f(data);
    }

    if (amd64Fma3Supported()) {
        return rbdKooNGenericWorkerFma3(data);
    }

    if (amd64AvxSupported()) {
        return rbdKooNGenericWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdKooNGenericWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
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

    /* Retrieve identical KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdKooNIdenticalWorkerAvx512f(data);
    }

    if (amd64Fma3Supported()) {
        return rbdKooNIdenticalWorkerFma3(data);
    }

    if (amd64AvxSupported()) {
        return rbdKooNIdenticalWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdKooNIdenticalWorkerSse2(data);
    }

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
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

/**
 * rbdKooNFillWorkerAvx512f
 *
 * Fill output Reliability with fixed value Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker amd64 AVX512F instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: the pointer to a fill KooN data
 *
 * Return (void *):
 *  NULL
 */
static FUNCTION_TARGET("avx512f") void *rbdKooNFillWorkerAvx512f(struct rbdKooNFillData *data)
{
    unsigned int time;
    __m512d m512d;
    __m256d m256d;
    __m128d m128d;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Define vector (8d, 4d and 2d) with provided value */
    m512d = _mm512_set1_pd(data->value);
    m256d = _mm256_set1_pd(data->value);
    m128d = _mm_set1_pd(data->value);

    /* For each time instant (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Fill output Reliability array with fixed value */
        _mm512_storeu_pd(&data->output[time], m512d);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Fill output Reliability array with fixed value */
        _mm256_storeu_pd(&data->output[time], m256d);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Fill output Reliability array with fixed value */
        _mm_storeu_pd(&data->output[time], m128d);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time++] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNFillWorkerAvx
 *
 * Fill output Reliability with fixed value Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker amd64 AVX instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: the pointer to a fill KooN data
 *
 * Return (void *):
 *  NULL
 */
static FUNCTION_TARGET("avx") void *rbdKooNFillWorkerAvx(struct rbdKooNFillData *data)
{
    unsigned int time;
    __m256d m256d;
    __m128d m128d;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* Define vectors (4d and 2d) with provided value */
    m256d = _mm256_set1_pd(data->value);
    m128d = _mm_set1_pd(data->value);

    /* For each time instant (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Fill output Reliability array with fixed value */
        _mm256_storeu_pd(&data->output[time], m256d);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Fill output Reliability array with fixed value */
        _mm_storeu_pd(&data->output[time], m128d);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time++] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerAvx512f
 *
 * Generic KooN RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: the pointer to a generic KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNGenericWorkerAvx512f(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV4dFma3(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerFma3
 *
 * Generic KooN RBD Worker function with amd64 FMA3 instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting amd64 FMA3 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: the pointer to a generic KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNGenericWorkerFma3(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV4dFma3(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerAvx
 *
 * Generic KooN RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: the pointer to a generic KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNGenericWorkerAvx(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerAvx512f
 *
 * Identical KooN RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      data: the pointer to a identical KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNIdenticalWorkerAvx512f(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dFma3(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV4dFma3(data, time);
                /* Increment current time instant */
                time += V4D;
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV4dFma3(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dFma3(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
        }
    }
    else {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV4dAvx(data, time);
                /* Increment current time instant */
                time += V4D;
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV4dAvx(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
        }
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerFma3
 *
 * Identical KooN RBD Worker function with amd64 FMA3 instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting amd64 FMA3 instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      data: the pointer to a identical KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNIdenticalWorkerFma3(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dFma3(data, time);
                /* Increment current time instant */
                time += V2D;
            }
        }
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV4dFma3(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dFma3(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
        }
    }
    else {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
        }
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV4dAvx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
        }
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerAvx
 *
 * Identical KooN RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      data: the pointer to a identical KooN RBD data
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNIdenticalWorkerAvx(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
        }
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV4dAvx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
        }
    }
    else {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
        }
        /* For each time instant to be processed (blocks of 4 time instants)... */
        while ((time + V4D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V4D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV4dAvx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V4D);
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
        }
    }

    return NULL;
}


#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
