/*
 *  Component: hot_standby_aarch64_neon.c
 *  Hot Stand-by RBD management - Optimized using AArch64 NEON instruction set
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
#include "../hot_standby_aarch64.h"
#include "../integral_aarch64.h"


/**
 * rbdHotStandbyWorkerNeon
 *
 * Hot Stand-by RBD Worker function with AArch64 NEON instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting AArch64 NEON instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerNeon(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV2dNeon(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV1dNeon(data, time);
    }

    return NULL;
}

/**
 * rbdHotStandbyStepV2dNeon
 *
 * Hot Stand-by RBD step function with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting AArch64 NEON 128bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+simd") void rbdHotStandbyStepV2dNeon(struct rbdHotStandbyData *data, unsigned int time)
{
    float64x2_t v2dTmp;
    float64x2_t v2dPri;
    float64x2_t v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = vld1q_f64(&data->primaryReliability[time]);
    v2dTmp = vld1q_f64(&data->standbyReliability[time]);
    v2dRes = rbdIntegralHotStandbyV2dNeon(data, time);
    v2dRes = vfmaq_f64(v2dPri, v2dRes, v2dTmp);

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdHotStandbyStepV1dNeon
 *
 * Hot Stand-by RBD step function with AArch64 NEON 64bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting AArch64 NEON 64bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+simd") void rbdHotStandbyStepV1dNeon(struct rbdHotStandbyData *data, unsigned int time)
{
    float64x1_t v1dPri;
    float64x1_t v1dTmp;
    float64x1_t v1dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v1dPri = vld1_f64(&data->primaryReliability[time]);
    v1dTmp = vld1_f64(&data->standbyReliability[time]);
    v1dRes = rbdIntegralHotStandbyV1dNeon(data, time);
    v1dRes = vfma_f64(v1dPri, v1dRes, v1dTmp);

    /* Cap the computed reliability and set it into output array */
    vst1_f64(&data->output[time], capReliabilityV1dNeon(v1dRes));
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
