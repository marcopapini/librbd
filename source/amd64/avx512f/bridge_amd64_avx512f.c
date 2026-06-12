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
HIDDEN FUNCTION_TARGET("avx512f") void *rbdBridgeGenericWorkerAvx512f(struct rbdBridgeData *data)
{
    unsigned int time;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepVNdAvx512f(mask, data, time);
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
HIDDEN FUNCTION_TARGET("avx512f") void *rbdBridgeIdenticalWorkerAvx512f(struct rbdBridgeData *data)
{
    unsigned int time;
    unsigned int alignSteps;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Are there at least 8 - 1 time instants to process? */
    if ((time + V8D) < data->numTimes) {
        /* Align, if possible, to vector size */
        if (((uintptr_t)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            /* Compute the number of doubles to align to vector size */
            alignSteps = ((uintptr_t)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) / sizeof(double);
            alignSteps = (V8D - alignSteps) & (V8D - 1);
            if (alignSteps > 0) {
                /* Compute mask for the management of the head */
                mask = (__mmask8)_cvtu32_mask16((1U << alignSteps) - 1);
                /* Compute reliability of Bridge RBD at current time instant */
                rbdBridgeIdenticalStepVNdAvx512f(mask, data, time);
                /* Increment current time instant */
                time += alignSteps;
            }
        }
    }
    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdBridgeGenericStepVNdAvx512f
 *
 * Generic Bridge RBD step function with amd64 AVX512F instruction set
 *
 * Input:
 *      __mmask8 mask
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
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdBridgeGenericStepVNdAvx512f(__mmask8 mask, struct rbdBridgeData *data, unsigned int time)
{
    __m512d vNdR1, vNdR2, vNdR3, vNdR4, vNdR5;
    __m512d vNdTmp1, vNdTmp2;
    __m512d vNdRes;

    /* Load reliabilities */
    vNdR1 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(0 * data->numTimes) + time]);
    vNdR2 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(1 * data->numTimes) + time]);
    vNdR3 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(2 * data->numTimes) + time]);
    vNdR4 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(3 * data->numTimes) + time]);
    vNdR5 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(4 * data->numTimes) + time]);

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
    vNdTmp1 = _mm512_maskz_add_pd(mask, vNdR1, vNdR3);
    vNdTmp2 = _mm512_maskz_add_pd(mask, vNdR2, vNdR4);
    vNdTmp1 = _mm512_maskz_fnmadd_pd(mask, vNdR1, vNdR3, vNdTmp1);
    vNdTmp2 = _mm512_maskz_fnmadd_pd(mask, vNdR2, vNdR4, vNdTmp2);
    vNdRes = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdTmp2);
    /* At this point v8dRes vector contains VAL1 value */
    vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdR3, vNdR4);
    vNdTmp2 = _mm512_maskz_mul_pd(mask, vNdR1, vNdR2);
    vNdTmp1 = _mm512_maskz_fnmadd_pd(mask, vNdTmp1, vNdTmp2, vNdTmp1);
    vNdTmp1 = _mm512_maskz_add_pd(mask, vNdTmp1, vNdTmp2);
    /* At this point v8dTmp1 vector contains VAL2 value */
    vNdRes = _mm512_maskz_sub_pd(mask, vNdRes, vNdTmp1);
    vNdRes = _mm512_maskz_fmadd_pd(mask, vNdR5, vNdRes, vNdTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdBridgeIdenticalStepVNdAvx512f
 *
 * Identical Bridge RBD step function with amd64 AVX512F instruction set
 *
 * Input:
 *      __mmask8 mask
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
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdBridgeIdenticalStepVNdAvx512f(__mmask8 mask, struct rbdBridgeData *data, unsigned int time)
{
    __m512d vNdR, vNdU;
    __m512d vNdTmp;
    __m512d vNdRes;

    /* Load reliability */
    vNdR = _mm512_maskz_loadu_pd(mask, &data->reliabilities[time]);

    /* Compute unreliability */
    vNdU = _mm512_maskz_sub_pd(mask, v8dOnes, vNdR);

    /* Compute reliability of Bridge block */
    vNdRes = vNdR;
    vNdRes = _mm512_maskz_fnmadd_pd(mask, vNdRes, vNdRes, v8dTwos);
    vNdRes = _mm512_maskz_mul_pd(mask, vNdRes, vNdR);
    vNdTmp = vNdU;
    vNdTmp = _mm512_maskz_fmsub_pd(mask, vNdTmp, vNdTmp, v8dTwos);
    vNdTmp = _mm512_maskz_fmadd_pd(mask, vNdTmp, vNdU, vNdRes);
    vNdTmp = _mm512_maskz_fmadd_pd(mask, vNdTmp, vNdU, v8dOnes);
    vNdRes = _mm512_maskz_mul_pd(mask, vNdTmp, vNdR);

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
