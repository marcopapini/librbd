/*
 *  Component: series_aarch64_neon.c
 *  Series RBD management - Optimized using AArch64 NEON instruction set
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
#include "../series_aarch64.h"


/**
 * rbdSeriesGenericStepV2dNeon
 *
 * Generic Series RBD step function with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting AArch64 NEON 128bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdSeriesGenericStepV2dNeon(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    float64x2_t v2dTmp;
    float64x2_t v2dRes;

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = vld1q_f64(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = vld1q_f64(&data->reliabilities[(component * data->numTimes) + time]);
        v2dRes = vmulq_f64(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdSeriesIdenticalStepV2dNeon
 *
 * Identical Series RBD step function with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting AArch64 NEON 128bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdSeriesIdenticalStepV2dNeon(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    float64x2_t v2dTmp;
    float64x2_t v2dRes;

    /* Load reliability */
    v2dTmp = vld1q_f64(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = v2dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v2dRes = vmulq_f64(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}


#endif /* CPU_AARCH64_NEON */
