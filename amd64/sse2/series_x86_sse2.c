/*
 *  Component: series_x86_sse2.c
 *  Series RBD management - Optimized using x86 SSE2 instruction set
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

#if CPU_X86_SSE2 != 0
#include "../rbd_internal_amd64.h"
#include "../series_amd64.h"


/**
 * rbdSeriesGenericStepV2dSse2
 *
 * Generic Series RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdSeriesGenericStepV2dSse2(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = _mm_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = _mm_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v2dRes = _mm_mul_pd(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdSeriesIdenticalStepV2dSse2
 *
 * Identical Series RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdSeriesIdenticalStepV2dSse2(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Load reliability */
    v2dTmp = _mm_loadu_pd(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = v2dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v2dRes = _mm_mul_pd(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


#endif /* CPU_X86_SSE2 */
