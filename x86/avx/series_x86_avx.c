/*
 *  Component: series_x86_avx.c
 *  Series RBD management - Optimized using x86 AVX instruction set
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
#include "../series_x86.h"


/* Save GCC target and optimization options and add x86 AVX instruction set */
#pragma GCC push_options
#pragma GCC target ("avx")


/**
 * rbdSeriesGenericStepV4dAvx
 *
 * Generic Series RBD step function with x86 AVX 256bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting x86 AVX 256bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesGenericStepV4dAvx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Compute reliability of Series RBD at current time instant */
    v4dRes = _mm256_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v4dTmp = _mm256_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdSeriesIdenticalStepV4dAvx
 *
 * Identical Series RBD step function with x86 AVX 256bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting x86 AVX 256bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesIdenticalStepV4dAvx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m256d v4dTmp;
    __m256d v4dRes;

    /* Load reliability */
    v4dTmp = _mm256_loadu_pd(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v4dRes = v4dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdSeriesGenericStepV2dAvx
 *
 * Generic Series RBD step function with x86 AVX 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting x86 AVX 128bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesGenericStepV2dAvx(struct rbdSeriesData *data, unsigned int time)
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
    _mm_storeu_pd(&data->output[time], capReliabilityV2dAvx(v2dRes));
}

/**
 * rbdSeriesIdenticalStepV2dAvx
 *
 * Identical Series RBD step function with x86 AVX 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting x86 AVX 128bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesIdenticalStepV2dAvx(struct rbdSeriesData *data, unsigned int time)
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
    _mm_storeu_pd(&data->output[time], capReliabilityV2dAvx(v2dRes));
}


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_AVX */
