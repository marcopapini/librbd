/*
 *  Component: integral_amd64_avx512f.c
 *  Compute integral for RBD management - Optimized using amd64 AVX512F instruction set
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
#include "../integral_amd64.h"


/**
 * cntto
 *
 * Count Trailing Ones (CTO)
 *
 * Description:
 *  This function counts the trailing ones of the given value.
 *  If the given value is 0xFFFFFFFF, then the CTO operation is undefined
 *  and this function returns -1.
 *
 * Parameters:
 *      val: value used for CTO operation
 *
 * Return (int):
 *  The number of trailing ones, -1 if the given value is 0xFFFFFFFF
 */
static inline int cntto(unsigned int val) {
    int bit;

    /* If the value is 0xFFFFFFFF, then the count trailing ones is undefined, return -1 */
    if (val == 0xFFFFFFFFU) {
        return -1;
    }

    /* Test each bit of value, from lsb, until the first one set to 1 is found */
    bit = 31;
    /* Isolate the lowest bit (closest to lsb) set to 0 */
    val = ~val & (val + 1);
    /* Count trailing zeroes through binary search */
    bit -= (!!(val & 0x0000FFFF)) << 4;
    bit -= (!!(val & 0x00FF00FF)) << 3;
    bit -= (!!(val & 0x0F0F0F0F)) << 2;
    bit -= (!!(val & 0x33333333)) << 1;
    bit -= !!(val & 0x55555555);

    return bit;
}

/**
 * rbdIntegralColdStandbyVNdAvx512f
 *
 * Compute integrals for Cold Stand-by function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes (at most) eight integrals for a Cold Stand-by RBD step
 *  exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute (at most):
 *  - $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_P(\tau) R_S(t+1-\tau) d\tau}$
 *  - $\int_0^{t+2}{f_P(\tau) R_S(t+2-\tau) d\tau}$
 *  - $\int_0^{t+3}{f_P(\tau) R_S(t+3-\tau) d\tau}$
 *  - $\int_0^{t+4}{f_P(\tau) R_S(t+4-\tau) d\tau}$
 *  - $\int_0^{t+5}{f_P(\tau) R_S(t+5-\tau) d\tau}$
 *  - $\int_0^{t+6}{f_P(\tau) R_S(t+6-\tau) d\tau}$
 *  - $\int_0^{t+7}{f_P(\tau) R_S(t+7-\tau) d\tau}$
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m512d):
 *  The result of the (at most) eight integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx512f") __m512d rbdIntegralColdStandbyVNdAvx512f(__mmask8 mask, struct rbdColdStandbyData *data, unsigned int time)
{
    __m512d vNdSum;
    __m512d vNdC;
    __m512d vNdY;
    __m512d vNdTSum;
    __m512d vNdTmp;
    __m512d vNdFailP;
    __m512d vNdRelS;
    __m512i vLMinusOne;
    __m512i vIdentity;
    __m512i vRevIdx;
    __mmask8 stepMask;
    __mmask8 loadMask;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxTN;
    unsigned int L;
    unsigned int loadBase;

    /* Manage the case when the first integral is computed over the empty time domain */
    L = cntto((unsigned int)mask);
    if (time == 0) {
        mask &= ~0x1U;
    }

    vNdSum = v8dZeros;
    vNdC = v8dZeros;

    /* Early return if the mask is 0 */
    if (mask == 0x0U) {
        return vNdSum;
    }

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxTN = startRevIdxT - L + 1;

    vLMinusOne = _mm512_set1_epi64((long long)L - 1);
    vIdentity = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
    vRevIdx = _mm512_maskz_sub_epi64(mask, vLMinusOne, vIdentity);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdFailP = _mm512_set1_pd(data->primaryFailureDensity[idx]);
        vNdRelS = _mm512_maskz_loadu_pd(mask, &data->standbyReliabilityRev[startRevIdxTN + idx]);
        vNdRelS = _mm512_maskz_permutexvar_pd(mask, vRevIdx, vNdRelS);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_maskz_fmsub_pd(mask, vNdFailP, vNdRelS, vNdC);
        vNdTSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);
        vNdC = _mm512_maskz_sub_pd(mask, _mm512_maskz_sub_pd(mask, vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    vNdFailP = _mm512_set1_pd(0.5 * data->primaryFailureDensity[0]);
    vNdRelS = _mm512_maskz_loadu_pd(mask, &data->standbyReliabilityRev[startRevIdxTN]);
    vNdRelS = _mm512_maskz_permutexvar_pd(mask, vRevIdx, vNdRelS);

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_maskz_fmsub_pd(mask, vNdFailP, vNdRelS, vNdC);
    vNdTSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);
    vNdC = _mm512_maskz_sub_pd(mask, _mm512_maskz_sub_pd(mask, vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
     */
    for (idx = (time == 0) ? 1 : 0; idx < V8D - 1; ++idx) {
        /* Lane k needs a node if k > step. We dynamically mask out lanes <= step */
        stepMask = mask & (0xFFU << (idx + 1));
        if (stepMask == 0) {
            /**
             * No Lane is active
             * We have completed the computation of internal nodes, early exit
             */
            break;
        }

        loadMask = (__mmask8)(1U << (L - idx - 1)) - 1;
        loadBase = data->numTimes - L + idx;
        vNdFailP = _mm512_set1_pd(data->primaryFailureDensity[time + idx]);
        vNdRelS = _mm512_maskz_loadu_pd(loadMask, &data->standbyReliabilityRev[loadBase]);
        vNdRelS = _mm512_maskz_permutexvar_pd(stepMask, vRevIdx, vNdRelS);

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_maskz_fmsub_pd(stepMask, vNdFailP, vNdRelS, vNdC);
        vNdTSum = _mm512_mask_add_pd(vNdSum, stepMask, vNdSum, vNdY);
        vNdC = _mm512_mask_sub_pd(vNdC, stepMask, _mm512_maskz_sub_pd(stepMask, vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;
    }

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    vNdFailP = _mm512_maskz_loadu_pd(mask, &data->primaryFailureDensity[time]);
    vNdFailP = _mm512_maskz_mul_pd(mask, vNdFailP, v8dHalfs);
    vNdRelS = _mm512_set1_pd(data->standbyReliabilityRev[data->numTimes - 1]);

    /* Add the current product to the result using the Kahan's method (no need to save vNdC here) */
    vNdY = _mm512_maskz_fmsub_pd(mask, vNdFailP, vNdRelS, vNdC);
    vNdSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);

    /* Multiply the partial result with the delta time */
    vNdTmp = _mm512_set1_pd(data->deltaT);
    vNdSum = _mm512_maskz_mul_pd(mask, vNdSum, vNdTmp);

    return vNdSum;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
