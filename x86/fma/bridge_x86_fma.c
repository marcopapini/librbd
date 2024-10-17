/*
 *  Component: bridge_x86_fma.c
 *  Bridge RBD management - Optimized using x86 FMA instruction set
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

#if CPU_X86_FMA != 0
#include "../rbd_internal_x86.h"
#include "../bridge_x86.h"


/* Save GCC target and optimization options and add x86 AVX/FMA instruction sets */
#pragma GCC push_options
#pragma GCC target ("avx", "fma")


/**
 * rbdBridgeGenericStepV4dFma
 *
 * Generic Bridge RBD step function with x86 FMA3 256bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting x86 FMA3 256bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeGenericStepV4dFma(struct rbdBridgeData *data, unsigned int time)
{
    __m256d v4dR1, v4dR2, v4dR3, v4dR4, v4dR5;
    __m256d v4dTmp1, v4dTmp2;
    __m256d v4dRes;

    /* Load reliabilities */
    v4dR1 = _mm256_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v4dR2 = _mm256_loadu_pd(&data->reliabilities[(1 * data->numTimes) + time]);
    v4dR3 = _mm256_loadu_pd(&data->reliabilities[(2 * data->numTimes) + time]);
    v4dR4 = _mm256_loadu_pd(&data->reliabilities[(3 * data->numTimes) + time]);
    v4dR5 = _mm256_loadu_pd(&data->reliabilities[(4 * data->numTimes) + time]);

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
    v4dTmp1 = _mm256_add_pd(v4dR1, v4dR3);
    v4dTmp2 = _mm256_add_pd(v4dR2, v4dR4);
    v4dTmp1 = _mm256_fnmadd_pd(v4dR1, v4dR3, v4dTmp1);
    v4dTmp2 = _mm256_fnmadd_pd(v4dR2, v4dR4, v4dTmp2);
    v4dRes = _mm256_mul_pd(v4dTmp1, v4dTmp2);
    /* At this point v4dRes vector contains VAL1 value */
    v4dTmp1 = _mm256_mul_pd(v4dR3, v4dR4);
    v4dTmp2 = _mm256_mul_pd(v4dR1, v4dR2);
    v4dTmp1 = _mm256_fnmadd_pd(v4dTmp1, v4dTmp2, v4dTmp1);
    v4dTmp1 = _mm256_add_pd(v4dTmp1, v4dTmp2);
    /* At this point v4dTmp1 vector contains VAL2 value */
    v4dRes = _mm256_sub_pd(v4dRes, v4dTmp1);
    v4dRes = _mm256_fmadd_pd(v4dR5, v4dRes, v4dTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}
/**
 * rbdBridgeIdenticalStepV4dFma
 *
 * Identical Bridge RBD step function with x86 FMA3 256bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting x86 FMA3 256bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeIdenticalStepV4dFma(struct rbdBridgeData *data, unsigned int time)
{
    __m256d v4dR, v4dU;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Load reliability */
    v4dR = _mm256_loadu_pd(&data->reliabilities[time]);

    /* Compute unreliability */
    v4dU = _mm256_sub_pd(v4dOnes, v4dR);

    /* Compute reliability of Bridge block */
    v4dRes = v4dR;
    v4dRes = _mm256_fnmadd_pd(v4dRes, v4dRes, v4dTwos);
    v4dRes = _mm256_mul_pd(v4dRes, v4dR);
    v4dTmp = v4dU;
    v4dTmp = _mm256_fmsub_pd(v4dTmp, v4dTmp, v4dTwos);
    v4dTmp = _mm256_fmadd_pd(v4dTmp, v4dU, v4dRes);
    v4dTmp = _mm256_fmadd_pd(v4dTmp, v4dU, v4dOnes);
    v4dRes = _mm256_mul_pd(v4dTmp, v4dR);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdBridgeGenericStepV2dFma
 *
 * Generic Bridge RBD step function with x86 FMA3 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting x86 FMA3 128bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeGenericStepV2dFma(struct rbdBridgeData *data, unsigned int time)
{
    __m128d v2dR1, v2dR2, v2dR3, v2dR4, v2dR5;
    __m128d v2dTmp1, v2dTmp2;
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
    v2dTmp1 = _mm_add_pd(v2dR1, v2dR3);
    v2dTmp2 = _mm_add_pd(v2dR2, v2dR4);
    v2dTmp1 = _mm_fnmadd_pd(v2dR1, v2dR3, v2dTmp1);
    v2dTmp2 = _mm_fnmadd_pd(v2dR2, v2dR4, v2dTmp2);
    v2dRes = _mm_mul_pd(v2dTmp1, v2dTmp2);
    /* At this point v2dRes vector contains VAL1 value */
    v2dTmp1 = _mm_mul_pd(v2dR3, v2dR4);
    v2dTmp2 = _mm_mul_pd(v2dR1, v2dR2);
    v2dTmp1 = _mm_fnmadd_pd(v2dTmp1, v2dTmp2, v2dTmp1);
    v2dTmp1 = _mm_add_pd(v2dTmp1, v2dTmp2);
    /* At this point v2dTmp1 vector contains VAL2 value */
    v2dRes = _mm_sub_pd(v2dRes, v2dTmp1);
    v2dRes = _mm_fmadd_pd(v2dR5, v2dRes, v2dTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdBridgeIdenticalStepV2dFma
 *
 * Identical Bridge RBD step function with x86 FMA3 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting x86 FMA3 128bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdBridgeIdenticalStepV2dFma(struct rbdBridgeData *data, unsigned int time)
{
    __m128d v2dR, v2dU;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Load reliability */
    v2dR = _mm_loadu_pd(&data->reliabilities[time]);

    /* Compute unreliability */
    v2dU = _mm_sub_pd(v2dOnes, v2dR);

    /* Compute reliability of Bridge block */
    v2dRes = v2dR;
    v2dRes = _mm_fnmadd_pd(v2dRes, v2dRes, v2dTwos);
    v2dRes = _mm_mul_pd(v2dRes, v2dR);
    v2dTmp = v2dU;
    v2dTmp = _mm_fmsub_pd(v2dTmp, v2dTmp, v2dTwos);
    v2dTmp = _mm_fmadd_pd(v2dTmp, v2dU, v2dRes);
    v2dTmp = _mm_fmadd_pd(v2dTmp, v2dU, v2dOnes);
    v2dRes = _mm_mul_pd(v2dTmp, v2dR);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_FMA */
