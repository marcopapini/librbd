/*
 *  Component: bridge_x86_avx512f.c
 *  Bridge RBD management - Optimized using x86 AVX512F instruction set
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

#if CPU_X86_AVX512F != 0
#include "../rbd_internal_x86.h"
#include "../bridge_x86.h"


/**
 * rbdBridgeGenericStepV8dAvx512f
 *
 * Generic Bridge RBD step function with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting x86 AVX512F 512bit.
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
 * Identical Bridge RBD step function with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting x86 AVX512F 512bit.
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


#endif /* CPU_X86_AVX512F */
