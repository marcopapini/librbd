/*
 *  Component: reliability_aarch64_neon.c
 *  Reliability RBD management - Optimized using AArch64 NEON instruction set
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

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../reliability_aarch64.h"


/**
 * rbdUnreliabilityWorkerNeon
 *
 * Unreliability RBD Worker function with AArch64 NEON instruction set
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting AArch64 NEON instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+simd") void *rbdUnreliabilityWorkerNeon(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    float64x2_t v2dRes;
    float64x1_t v1dRes;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + V2D);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + V2D);
        /* Compute unreliability at current time instant */
        v2dRes = vld1q_f64(&data->reliability[time]);
        v2dRes = vsubq_f64(v2dOnes, v2dRes);
        vst1q_f64(&data->unreliability[time], capReliabilityV2dNeon(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute unreliability at current time instant */
        v1dRes = vld1_f64(&data->reliability[time]);
        v1dRes = vsub_f64(v1dOnes, v1dRes);
        vst1_f64(&data->unreliability[time], capReliabilityV1dNeon(v1dRes));
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerNeon
 *
 * Reliability Sum RBD Worker function with AArch64 NEON instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting AArch64 NEON instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+simd") void *rbdReliabilitySumWorkerNeon(struct rbdReliabilityData *data)
{
    unsigned int time;
    float64x2_t v2dRes;
    float64x2_t v2dTmp;
    float64x1_t v1dRes;
    float64x1_t v1dTmp;

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + V2D);
        prefetchRead(data->r2, 1, data->numTimes, time + V2D);
        prefetchWrite(data->output, 1, data->numTimes, time + V2D);
        /* Compute reliability sum at current time instant */
        v2dRes = vld1q_f64(&data->r1[time]);
        v2dTmp = vld1q_f64(&data->r2[time]);
        v2dRes = vaddq_f64(v2dRes, v2dTmp);
        vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability sum at current time instant */
        v1dRes = vld1_f64(&data->r1[time]);
        v1dTmp = vld1_f64(&data->r2[time]);
        v1dRes = vadd_f64(v1dRes, v1dTmp);
        vst1_f64(&data->output[time], capReliabilityV1dNeon(v1dRes));
    }

    return NULL;
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
