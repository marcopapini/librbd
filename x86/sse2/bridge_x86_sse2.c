/*
 *  Component: bridge_x86_sse2.c
 *  Bridge RBD management - Optimized using x86 SSE2 instruction set
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


#include "../../rbd_internal.h"

#if CPU_X86_SSE2 != 0
#include "../rbd_internal_x86.h"
#include "../bridge_x86.h"


/* Save GCC target and optimization options and add x86 SSE2 instruction set */
#pragma GCC push_options
#pragma GCC target ("sse2")


/**
 * rbdBridgeGenericStepV2dSse2
 *
 * Generic Bridge RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeGenericStepV2dSse2(struct rbdBridgeData *data, unsigned int time)
{
    __m128d v2dR1, v2dR2, v2dR3, v2dR4, v2dR5;
    __m128d v2dTmp1, v2dTmp2, v2dTmp3;
    __m128d v2dRes;

    /* Load reliabilities */
    v2dR1 = _mm_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dR2 = _mm_loadu_pd(&data->reliabilities[(1 * data->numTimes) + time]);
    v2dR3 = _mm_loadu_pd(&data->reliabilities[(2 * data->numTimes) + time]);
    v2dR4 = _mm_loadu_pd(&data->reliabilities[(3 * data->numTimes) + time]);
    v2dR5 = _mm_loadu_pd(&data->reliabilities[(4 * data->numTimes) + time]);

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
    v2dTmp1 = _mm_mul_pd(v2dR1, v2dR3);
    v2dTmp2 = _mm_mul_pd(v2dR2, v2dR4);
    v2dTmp1 = _mm_sub_pd(v2dR3, v2dTmp1);
    v2dTmp2 = _mm_sub_pd(v2dR4, v2dTmp2);
    v2dTmp1 = _mm_add_pd(v2dR1, v2dTmp1);
    v2dTmp2 = _mm_add_pd(v2dR2, v2dTmp2);
    v2dRes = _mm_mul_pd(v2dTmp1, v2dTmp2);
    /* At this point v2dRes vector contains VAL1 value */
    v2dTmp1 = _mm_mul_pd(v2dR3, v2dR4);
    v2dTmp2 = _mm_mul_pd(v2dR1, v2dR2);
    v2dTmp3 = _mm_mul_pd(v2dTmp1, v2dTmp2);
    v2dTmp1 = _mm_add_pd(v2dTmp1, v2dTmp2);
    v2dTmp1 = _mm_sub_pd(v2dTmp1, v2dTmp3);
    /* At this point v2dTmp1 vector contains VAL2 value */
    v2dRes = _mm_sub_pd(v2dRes, v2dTmp1);
    v2dRes = _mm_mul_pd(v2dR5, v2dRes);
    v2dRes = _mm_add_pd(v2dRes, v2dTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdBridgeIdenticalStepV2dSse2
 *
 * Identical Bridge RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeIdenticalStepV2dSse2(struct rbdBridgeData *data, unsigned int time)
{
    __m128d v2dR, v2dU;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Load reliability */
    v2dR = _mm_loadu_pd(&data->reliabilities[time]);

    /* Compute unreliability */
    v2dU = _mm_sub_pd(v2dOnes, v2dR);

    /* Compute reliability of Bridge block */
    v2dRes = _mm_mul_pd(v2dR, v2dR);
    v2dRes = _mm_sub_pd(v2dTwos, v2dRes);
    v2dRes = _mm_mul_pd(v2dRes, v2dR);
    v2dTmp = _mm_mul_pd(v2dU, v2dU);
    v2dTmp = _mm_sub_pd(v2dTmp, v2dTwos);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dU);
    v2dTmp = _mm_add_pd(v2dTmp, v2dRes);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dU);
    v2dTmp = _mm_add_pd(v2dTmp, v2dOnes);
    v2dRes = _mm_mul_pd(v2dTmp, v2dR);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_SSE2 */
