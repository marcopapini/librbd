/*
 *  Component: bridge_aarch64_neon.c
 *  Bridge RBD management - Optimized using AArch64 NEON instruction set
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

#if CPU_AARCH64_NEON != 0
#include "../rbd_internal_aarch64.h"
#include "../bridge_aarch64.h"


/**
 * rbdBridgeGenericStepV2dNeon
 *
 * Generic Bridge RBD step function with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting AArch64 NEON 128bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdBridgeGenericStepV2dNeon(struct rbdBridgeData *data, unsigned int time)
{
    float64x2_t v2dR1, v2dR2, v2dR3, v2dR4, v2dR5;
    float64x2_t v2dTmp1, v2dTmp2;
    float64x2_t v2dRes;

    /* Load reliabilities */
    v2dR1 = vld1q_f64(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dR2 = vld1q_f64(&data->reliabilities[(1 * data->numTimes) + time]);
    v2dR3 = vld1q_f64(&data->reliabilities[(2 * data->numTimes) + time]);
    v2dR4 = vld1q_f64(&data->reliabilities[(3 * data->numTimes) + time]);
    v2dR5 = vld1q_f64(&data->reliabilities[(4 * data->numTimes) + time]);

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
    v2dTmp1 = vaddq_f64(v2dR1, v2dR3);
    v2dTmp2 = vaddq_f64(v2dR2, v2dR4);
    v2dTmp1 = vfmsq_f64(v2dTmp1, v2dR1, v2dR3);
    v2dTmp2 = vfmsq_f64(v2dTmp2, v2dR2, v2dR4);
    v2dRes = vmulq_f64(v2dTmp1, v2dTmp2);
    /* At this point v2dRes vector contains VAL1 value */
    v2dTmp1 = vmulq_f64(v2dR3, v2dR4);
    v2dTmp2 = vmulq_f64(v2dR1, v2dR2);
    v2dTmp1 = vfmsq_f64(v2dTmp1, v2dTmp1, v2dTmp2);
    v2dTmp1 = vaddq_f64(v2dTmp1, v2dTmp2);
    /* At this point v2dTmp1 vector contains VAL2 value */
    v2dRes = vsubq_f64(v2dRes, v2dTmp1);
    v2dRes = vfmaq_f64(v2dTmp1, v2dR5, v2dRes);

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdBridgeIdenticalStepV2dNeon
 *
 * Identical Bridge RBD step function with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting AArch64 NEON 128bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdBridgeIdenticalStepV2dNeon(struct rbdBridgeData *data, unsigned int time)
{
    float64x2_t v2dR, v2dU;
    float64x2_t v2dTmp;
    float64x2_t v2dRes;

    /* Load reliability */
    v2dR = vld1q_f64(&data->reliabilities[time]);

    /* Compute unreliability */
    v2dU = vsubq_f64(v2dOnes, v2dR);

    /* Compute reliability of Bridge block */
    v2dRes = vfmsq_f64(v2dTwos, v2dR, v2dR);
    v2dTmp = vfmaq_f64(v2dMinusTwos, v2dU, v2dU);
    v2dRes = vmulq_f64(v2dRes, v2dR);
    v2dTmp = vfmaq_f64(v2dRes, v2dTmp, v2dU);
    v2dTmp = vfmaq_f64(v2dOnes, v2dTmp, v2dU);
    v2dRes = vmulq_f64(v2dTmp, v2dR);

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}


#endif /* CPU_AARCH64_NEON */
