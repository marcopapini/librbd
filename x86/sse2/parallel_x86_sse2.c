/*
 *  Component: parallel_x86_sse2.c
 *  Parallel RBD management - Optimized using x86 SSE2 instruction set
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

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_x86.h"
#include "../parallel_x86.h"


/**
 * rbdParallelGenericStepV2dSse2
 *
 * Generic Parallel RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdParallelGenericStepV2dSse2(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v2dRes = _mm_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dRes = _mm_sub_pd(v2dOnes, v2dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = _mm_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v2dTmp = _mm_sub_pd(v2dOnes, v2dTmp);
        v2dRes = _mm_mul_pd(v2dRes, v2dTmp);
    }
    v2dRes = _mm_sub_pd(v2dOnes, v2dRes);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdParallelIdenticalStepV2dSse2
 *
 * Identical Parallel RBD step function with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting x86 SSE2 128bit.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdParallelIdenticalStepV2dSse2(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m128d v2dU;
    __m128d v2dRes;

    /* Load unreliability */
    v2dU = _mm_loadu_pd(&data->reliabilities[time]);
    v2dU = _mm_sub_pd(v2dOnes, v2dU);

    /* Compute reliability of Parallel RBD at current time instant */
    v2dRes = v2dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v2dRes = _mm_mul_pd(v2dRes, v2dU);
    }
    v2dRes = _mm_sub_pd(v2dOnes, v2dRes);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
