/*
 *  Component: bridge_amd64_avx.c
 *  Bridge RBD management - Optimized using amd64 AVX instruction set
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

#if CPU_X86_AVX != 0
#include "../rbd_internal_amd64.h"
#include "../bridge_amd64.h"


/**
 * rbdBridgeGenericStepV4dAvx
 *
 * Generic Bridge RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdBridgeGenericStepV4dAvx(struct rbdBridgeData *data, unsigned int time)
{
    __m256d v4dR1, v4dR2, v4dR3, v4dR4, v4dR5;
    __m256d v4dTmp1, v4dTmp2, v4dTmp3;
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
    v4dTmp1 = _mm256_mul_pd(v4dR1, v4dR3);
    v4dTmp2 = _mm256_mul_pd(v4dR2, v4dR4);
    v4dTmp1 = _mm256_sub_pd(v4dR3, v4dTmp1);
    v4dTmp2 = _mm256_sub_pd(v4dR4, v4dTmp2);
    v4dTmp1 = _mm256_add_pd(v4dR1, v4dTmp1);
    v4dTmp2 = _mm256_add_pd(v4dR2, v4dTmp2);
    v4dRes = _mm256_mul_pd(v4dTmp1, v4dTmp2);
    /* At this point v4dRes vector contains VAL1 value */
    v4dTmp1 = _mm256_mul_pd(v4dR3, v4dR4);
    v4dTmp2 = _mm256_mul_pd(v4dR1, v4dR2);
    v4dTmp3 = _mm256_mul_pd(v4dTmp1, v4dTmp2);
    v4dTmp1 = _mm256_add_pd(v4dTmp1, v4dTmp2);
    v4dTmp1 = _mm256_sub_pd(v4dTmp1, v4dTmp3);
    /* At this point v4dTmp1 vector contains VAL2 value */
    v4dRes = _mm256_sub_pd(v4dRes, v4dTmp1);
    v4dRes = _mm256_mul_pd(v4dR5, v4dRes);
    v4dRes = _mm256_add_pd(v4dRes, v4dTmp1);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdBridgeIdenticalStepV4dAvx
 *
 * Identical Bridge RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdBridgeIdenticalStepV4dAvx(struct rbdBridgeData *data, unsigned int time)
{
    __m256d v4dR, v4dU;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Load reliability */
    v4dR = _mm256_loadu_pd(&data->reliabilities[time]);

    /* Compute unreliability */
    v4dU = _mm256_sub_pd(v4dOnes, v4dR);

    /* Compute reliability of Bridge block */
    v4dRes = _mm256_mul_pd(v4dR, v4dR);
    v4dRes = _mm256_sub_pd(v4dTwos, v4dRes);
    v4dRes = _mm256_mul_pd(v4dRes, v4dR);
    v4dTmp = _mm256_mul_pd(v4dU, v4dU);
    v4dTmp = _mm256_sub_pd(v4dTmp, v4dTwos);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dU);
    v4dTmp = _mm256_add_pd(v4dTmp, v4dRes);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dU);
    v4dTmp = _mm256_add_pd(v4dTmp, v4dOnes);
    v4dRes = _mm256_mul_pd(v4dTmp, v4dR);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}


#endif /* CPU_X86_AVX */
