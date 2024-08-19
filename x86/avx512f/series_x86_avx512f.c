/*
 *  Component: series_x86_avx512f.c
 *  Series RBD management - Optimized using x86 AVX512F instruction set
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

#if CPU_X86_AVX512F != 0
#include "../rbd_internal_x86.h"
#include "../series_x86.h"


/* Save GCC target and optimization options and add x86 AVX/FMA/AVX512F instruction sets */
#pragma GCC push_options
#pragma GCC target ("avx", "fma", "avx512f")


/**
 * rbdSeriesGenericStepV8dAvx512f
 *
 * Generic Series RBD step function with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting x86 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesGenericStepV8dAvx512f(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dTmp;
    __m512d v8dRes;

    /* Compute reliability of Series RBD at current time instant */
    v8dRes = _mm512_loadu_pd(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v8dTmp = _mm512_loadu_pd(&data->reliabilities[(component * data->numTimes) + time]);
        v8dRes = _mm512_mul_pd(v8dRes, v8dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdSeriesIdenticalStepV8dAvx512f
 *
 * Identical Series RBD step function with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting x86 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
__attribute__((visibility ("hidden"))) void rbdSeriesIdenticalStepV8dAvx512f(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    __m512d v8dTmp;
    __m512d v8dRes;

    /* Load reliability */
    v8dTmp = _mm512_loadu_pd(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v8dRes = v8dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v8dRes = _mm512_mul_pd(v8dRes, v8dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}


/* Restore GCC target and optimization options */
#pragma GCC pop_options


#endif /* CPU_X86_AVX512F */
