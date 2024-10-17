/*
 *  Component: parallel_x86_avx.c
 *  Parallel RBD management - Optimized using x86 AVX instruction set
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

#if CPU_X86_AVX != 0
#include "../rbd_internal_x86.h"
#include "../parallel_x86.h"


/* Save GCC target and optimization options and add x86 AVX instruction set */
#pragma GCC push_options
#pragma GCC target ("avx")


/**
 * rbdParallelGenericStepV4dAvx
 *
 * Generic Parallel RBD step function with x86 AVX 256bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting x86 AVX 256bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdParallelGenericStepV4dAvx(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v4dRes = _mm256_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    v4dRes = _mm256_sub_pd(v4dOnes, v4dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v4dTmp = _mm256_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v4dTmp = _mm256_sub_pd(v4dOnes, v4dTmp);
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    }
    v4dRes = _mm256_sub_pd(v4dOnes, v4dRes);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdParallelIdenticalStepV4dAvx
 *
 * Identical Parallel RBD step function with x86 AVX 256bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting x86 AVX 256bit.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdParallelIdenticalStepV4dAvx(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dU;
    __m256d v4dRes;

    /* Load unreliability */
    v4dU = _mm256_loadu_pd(&data->reliabilities[time]);
    v4dU = _mm256_sub_pd(v4dOnes, v4dU);

    /* Compute reliability of Parallel RBD at current time instant */
    v4dRes = v4dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v4dRes = _mm256_mul_pd(v4dRes, v4dU);
    }
    v4dRes = _mm256_sub_pd(v4dOnes, v4dRes);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_AVX */
