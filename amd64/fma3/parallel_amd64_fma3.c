/*
 *  Component: parallel_amd64_fma3.c
 *  Parallel RBD management - Optimized using amd64 FMA3 instruction set
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../parallel_amd64.h"


/**
 * rbdParallelGenericStepV4dFma3
 *
 * Generic Parallel RBD step function with amd64 FMA3 256bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting amd64 FMA3 256bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("fma") void rbdParallelGenericStepV4dFma3(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v4dRes = _mm256_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v4dRes = _mm256_sub_pd(v4dOnes, v4dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v4dTmp = _mm256_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v4dRes = _mm256_fnmadd_pd(v4dRes, v4dTmp, v4dRes);
    }
    v4dRes = _mm256_sub_pd(v4dOnes, v4dRes);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdParallelGenericStepV2dFma3
 *
 * Generic Parallel RBD step function with amd64 FMA3 128bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting amd64 FMA3 128bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("fma") void rbdParallelGenericStepV2dFma3(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m128d v2dTmp;
    __m128d v2dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v2dRes = _mm_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dRes = _mm_sub_pd(v2dOnes, v2dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = _mm_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v2dRes = _mm_fnmadd_pd(v2dRes, v2dTmp, v2dRes);
    }
    v2dRes = _mm_sub_pd(v2dOnes, v2dRes);

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
