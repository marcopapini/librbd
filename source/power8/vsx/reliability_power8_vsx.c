/*
 *  Component: reliability_power8_vsx.c
 *  Reliability RBD management - Optimized using POWER8 VSX instruction set
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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
#include "../reliability_power8.h"


/**
 * rbdUnreliabilityWorkerVsx
 *
 * Unreliability RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("vsx") void *rbdUnreliabilityWorkerVsx(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    double64x2 v2dRes;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + V2D);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + V2D);
        /* Compute unreliability at current time instant */
        v2dRes = vectorLoad(&data->reliability[time]);
        v2dRes = vec_sub(v2dOnes, v2dRes);
        vectorStore(&data->unreliability[time], capReliabilityV2dVsx(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute unreliability at current time instant */
        v2dRes = vec_promote(data->reliability[time], 0);
        v2dRes = vec_sub(v2dOnes, v2dRes);
        data->unreliability[time] = vec_extract(capReliabilityV2dVsx(v2dRes), 0);
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerVsx
 *
 * Reliability Sum RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("vsx") void *rbdReliabilitySumWorkerVsx(struct rbdReliabilityData *data)
{
    unsigned int time;
    double64x2 v2dRes;
    double64x2 v2dTmp;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + V2D);
        prefetchRead(data->r2, 1, data->numTimes, time + V2D);
        prefetchWrite(data->output, 1, data->numTimes, time + V2D);
        /* Compute reliability sum at current time instant */
        v2dRes = vectorLoad(&data->r1[time]);
        v2dTmp = vectorLoad(&data->r2[time]);
        v2dRes = vec_add(v2dRes, v2dTmp);
        vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability sum at current time instant */
        v2dRes = vec_promote(data->r1[time], 0);
        v2dTmp = vec_promote(data->r2[time], 0);
        v2dRes = vec_add(v2dRes, v2dTmp);
        data->output[time] = vec_extract(capReliabilityV2dVsx(v2dRes), 0);
    }

    return NULL;
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
