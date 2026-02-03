/*
 *  Component: bridge_power8_vsx.c
 *  Bridge RBD management - Optimized using POWER8 VSX instruction set
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

#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_power8.h"
#include "../bridge_power8.h"


/**
 * rbdBridgeGenericWorkerVsx
 *
 * Bridge RBD Worker function with POWER8 VSX
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorkerVsx(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorkerVsx
 *
 * Identical Bridge RBD Worker function with POWER8 VSX
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorkerVsx(struct rbdBridgeData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Bridge RBD at current time instant */
            rbdBridgeIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdBridgeGenericStepV2dVsx
 *
 * Generic Bridge RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdBridgeGenericStepV2dVsx(struct rbdBridgeData *data, unsigned int time)
{
    double64x2 v2dR1, v2dR2, v2dR3, v2dR4, v2dR5;
    double64x2 v2dTmp1, v2dTmp2;
    double64x2 v2dRes;

    /* Load reliabilities */
    v2dR1 = vectorLoad(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dR2 = vectorLoad(&data->reliabilities[(1 * data->numTimes) + time]);
    v2dR3 = vectorLoad(&data->reliabilities[(2 * data->numTimes) + time]);
    v2dR4 = vectorLoad(&data->reliabilities[(3 * data->numTimes) + time]);
    v2dR5 = vectorLoad(&data->reliabilities[(4 * data->numTimes) + time]);

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
    v2dTmp1 = vec_add(v2dR1, v2dR3);
    v2dTmp2 = vec_add(v2dR2, v2dR4);
    v2dTmp1 = vec_nmadd(v2dR1, v2dR3, v2dTmp1);
    v2dTmp2 = vec_nmadd(v2dR2, v2dR4, v2dTmp2);
    v2dRes = vec_mul(v2dTmp1, v2dTmp2);
    /* At this point v2dRes vector contains VAL1 value */
    v2dTmp1 = vec_mul(v2dR3, v2dR4);
    v2dTmp2 = vec_mul(v2dR1, v2dR2);
    v2dTmp1 = vec_nmadd(v2dTmp1, v2dTmp2, v2dTmp1);
    v2dTmp1 = vec_add(v2dTmp1, v2dTmp2);
    /* At this point v2dTmp1 vector contains VAL2 value */
    v2dRes = vec_sub(v2dRes, v2dTmp1);
    v2dRes = vec_madd(v2dRes, v2dR5, v2dTmp1);

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdBridgeIdenticalStepV2dVsx
 *
 * Identical Bridge RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdBridgeIdenticalStepV2dVsx(struct rbdBridgeData *data, unsigned int time)
{
    double64x2 v2dR, v2dU;
    double64x2 v2dTmp;
    double64x2 v2dRes;

    /* Load reliability */
    v2dR = vectorLoad(&data->reliabilities[time]);

    /* Compute unreliability */
    v2dU = vec_sub(v2dOnes, v2dR);

    /* Compute reliability of Bridge block */
    v2dRes = vec_nmadd(v2dR, v2dR, v2dTwos);
    v2dTmp = vec_msub(v2dU, v2dU, v2dTwos);
    v2dRes = vec_mul(v2dRes, v2dR);
    v2dTmp = vec_madd(v2dTmp, v2dU, v2dRes);
    v2dTmp = vec_madd(v2dTmp, v2dU, v2dOnes);
    v2dRes = vec_mul(v2dTmp, v2dR);

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
