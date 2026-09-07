/*
 *  Component: failure_density_amd64_avx512f.c
 *  Failure Density computation for RBD management - Optimized using amd64 AVX512F instruction set
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../failure_density_amd64.h"


/**
 * rbdFailureDensityWorkerAvx512f
 *
 * Failure Density Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      double *reliability
 *      unsigned int numTimes
 *      double deltaT
 *
 * Output:
 *      double *failureDensity
 *
 * Description:
 *  This function implements the Failure Density Worker with amd64 AVX512F instruction set.
 *  It is responsible to compute the Failure Density function given the Reliability function
 *
 * Parameters:
 *      reliability: Reliability function
 *      failureDensity: computed Failure Density function
 *      numTimes: number of time instants
 *      deltaT: time difference between two consecutive time instants
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdFailureDensityWorkerAvx512f(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT)
{
    __m512d v8dR;
    __m512d v8dTmp;
    __m512d v8dDT;
    __mmask8 mask;
    unsigned int idx;

    /* Initialize the first element of the Failure Density function to 0 */
    failureDensity[0] = 0.0;

    /* For each one of the remaining elements... */
    v8dDT = _mm512_set1_pd(deltaT);
    idx = 1;
    while ((idx + V8D) <= numTimes) {
        /* Compute the Failure Density function */
        v8dR = _mm512_loadu_pd(&reliability[idx]);
        v8dTmp = _mm512_loadu_pd(&reliability[idx - 1]);
        v8dR = _mm512_sub_pd(v8dTmp, v8dR);
        v8dR = _mm512_div_pd(v8dR, v8dDT);
        v8dR = _mm512_max_pd(v8dR, v8dZeros);
        _mm512_storeu_pd(&failureDensity[idx], v8dR);
        /* Increment current time instant */
        idx += V8D;
    }
    /* Is (at least) 1 time instant remaining? */
    if (idx < numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (numTimes - idx)) - 1);
        /* Compute the Failure Density function */
        v8dR = _mm512_maskz_loadu_pd(mask, &reliability[idx]);
        v8dTmp = _mm512_maskz_loadu_pd(mask, &reliability[idx - 1]);
        v8dR = _mm512_maskz_sub_pd(mask, v8dTmp, v8dR);
        v8dR = _mm512_maskz_div_pd(mask, v8dR, v8dDT);
        v8dR = _mm512_maskz_max_pd(mask, v8dR, v8dZeros);
        _mm512_mask_storeu_pd(&failureDensity[idx], mask, v8dR);
    }
}


#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */
