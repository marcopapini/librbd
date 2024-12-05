/*
 *  Component: parallel_amd64_avx512f.c
 *  Parallel RBD management - Optimized using amd64 AVX512F instruction set
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
 * rbdParallelGenericStepV8dAvx512f
 *
 * Generic Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelGenericStepV8dAvx512f(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dTmp;
    __m512d v8dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v8dRes = _mm512_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v8dTmp = _mm512_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v8dRes = _mm512_fnmadd_pd(v8dRes, v8dTmp, v8dRes);
    }
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdParallelIdenticalStepV8dAvx512f
 *
 * Identical Parallel RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdParallelIdenticalStepV8dAvx512f(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dU;
    __m512d v8dRes;

    /* Load unreliability */
    v8dU = _mm512_loadu_pd(&data->reliabilities[time]);
    v8dU = _mm512_sub_pd(v8dOnes, v8dU);

    /* Compute reliability of Parallel RBD at current time instant */
    v8dRes = v8dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v8dRes = _mm512_mul_pd(v8dRes, v8dU);
    }
    v8dRes = _mm512_sub_pd(v8dOnes, v8dRes);

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
