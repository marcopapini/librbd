/*
 *  Component: bridge_amd64_avx512f.c
 *  Bridge RBD management - Optimized using amd64 AVX512F instruction set
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


#include "../../generic/rbd_internal_generic.h"

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../bridge_amd64.h"


/**
 * rbdBridgeGenericWorkerAvx512f
 *
 * Generic Bridge RBD Worker function with amd64 AVX512F 512bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorkerAvx512f(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

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
        rbdBridgeGenericStepV4dFma3(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepV2dFma3(data, time);
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

/**
 * rbdBridgeIdenticalWorkerAvx512f
 *
 * Identical Bridge RBD Worker function with amd64 AVX512F 512bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorkerAvx512f(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
        if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV2dFma3(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepV4dFma3(data, time);
            /* Increment current time instant */
            time += V4D;
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV4dFma3(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdBridgeGenericStepV8dAvx512f
 *
 * Generic Bridge RBD step function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdBridgeGenericStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time)
{
    __m512d v8dR1, v8dR2, v8dR3, v8dR4, v8dR5;
    __m512d v8dTmp1, v8dTmp2;
    __m512d v8dRes;

    /* Load reliabilities */
    v8dR1 = _mm512_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v8dR2 = _mm512_loadu_pd(&data->reliabilities[(1 * data->numTimes) + time]);
    v8dR3 = _mm512_loadu_pd(&data->reliabilities[(2 * data->numTimes) + time]);
    v8dR4 = _mm512_loadu_pd(&data->reliabilities[(3 * data->numTimes) + time]);
    v8dR5 = _mm512_loadu_pd(&data->reliabilities[(4 * data->numTimes) + time]);

    /**
     * Formula:
     *   R = R5 * (1 - (F1 * F3)) * (1 - (F2 * F4)) + F5 * (1 - (1 - (R1 * R2)) * (1 - (R3 * R4)))
     *
     * Optimized formula:
     *   VAL1 = (R1 + R3 - (R1 * R3)) * (R2 + R4 - (R2 * R4))
     *   VAL2 = (R1 * R2) + (R3 * R4) - (R1 * R2 * R3 * R4)
     *   R = R5 * (VAL1 - VAL2) + VAL2
     */

    /* Compute reliability of Bridge block */
    v8dTmp1 = _mm512_add_pd(v8dR1, v8dR3);
    v8dTmp2 = _mm512_add_pd(v8dR2, v8dR4);
    v8dTmp1 = _mm512_fnmadd_pd(v8dR1, v8dR3, v8dTmp1);
    v8dTmp2 = _mm512_fnmadd_pd(v8dR2, v8dR4, v8dTmp2);
    v8dRes = _mm512_mul_pd(v8dTmp1, v8dTmp2);
    /* At this point v8dRes vector contains VAL1 value */
    v8dTmp1 = _mm512_mul_pd(v8dR3, v8dR4);
    v8dTmp2 = _mm512_mul_pd(v8dR1, v8dR2);
    v8dTmp1 = _mm512_fnmadd_pd(v8dTmp1, v8dTmp2, v8dTmp1);
    v8dTmp1 = _mm512_add_pd(v8dTmp1, v8dTmp2);
    /* At this point v8dTmp1 vector contains VAL2 value */
    v8dRes = _mm512_sub_pd(v8dRes, v8dTmp1);
    v8dRes = _mm512_fmadd_pd(v8dR5, v8dRes, v8dTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdBridgeIdenticalStepV8dAvx512f
 *
 * Identical Bridge RBD step function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdBridgeIdenticalStepV8dAvx512f(struct rbdBridgeData *data, unsigned int time)
{
    __m512d v8dR, v8dU;
    __m512d v8dTmp;
    __m512d v8dRes;

    /* Load reliability */
    v8dR = _mm512_loadu_pd(&data->reliabilities[time]);

    /* Compute unreliability */
    v8dU = _mm512_sub_pd(v8dOnes, v8dR);

    /* Compute reliability of Bridge block */
    v8dRes = v8dR;
    v8dRes = _mm512_fnmadd_pd(v8dRes, v8dRes, v8dTwos);
    v8dRes = _mm512_mul_pd(v8dRes, v8dR);
    v8dTmp = v8dU;
    v8dTmp = _mm512_fmsub_pd(v8dTmp, v8dTmp, v8dTwos);
    v8dTmp = _mm512_fmadd_pd(v8dTmp, v8dU, v8dRes);
    v8dTmp = _mm512_fmadd_pd(v8dTmp, v8dU, v8dOnes);
    v8dRes = _mm512_mul_pd(v8dTmp, v8dR);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
