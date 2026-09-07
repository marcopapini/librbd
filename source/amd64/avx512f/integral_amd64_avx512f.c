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
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+1-\tau) d\tau}$
 *  - $\int_0^{t+2}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+2-\tau) d\tau}$
 *  - $\int_0^{t+3}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+3-\tau) d\tau}$
 *  - $\int_0^{t+4}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+4-\tau) d\tau}$
 *  - $\int_0^{t+5}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+5-\tau) d\tau}$
 *  - $\int_0^{t+6}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+6-\tau) d\tau}$
 *  - $\int_0^{t+7}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+7-\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
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
    __m512i vNiLMinusOne;
    __m512i vNiIdentity;
    __m512i vNiRevIdx;
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

    vNiLMinusOne = _mm512_set1_epi64((long long)L - 1);
    vNiIdentity = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
    vNiRevIdx = _mm512_maskz_sub_epi64(mask, vNiLMinusOne, vNiIdentity);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdFailP = _mm512_set1_pd(data->primaryFailureDensity[idx]);
        vNdTmp = _mm512_set1_pd(data->switchReliability[idx]);
        vNdRelS = _mm512_maskz_loadu_pd(mask, &data->standbyReliabilityRev[startRevIdxTN + idx]);
        vNdRelS = _mm512_maskz_permutexvar_pd(mask, vNiRevIdx, vNdRelS);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * One product is implicit with FMA operation
         */
        vNdTmp = _mm512_maskz_mul_pd(mask, vNdFailP, vNdTmp);

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_maskz_fmsub_pd(mask, vNdTmp, vNdRelS, vNdC);
        vNdTSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);
        vNdC = _mm512_maskz_sub_pd(mask, _mm512_maskz_sub_pd(mask, vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = _mm512_set1_pd(0.5 * data->primaryFailureDensity[0]);
    vNdTmp = _mm512_set1_pd(data->switchReliability[0]);
    vNdRelS = _mm512_maskz_loadu_pd(mask, &data->standbyReliabilityRev[startRevIdxTN]);
    vNdRelS = _mm512_maskz_permutexvar_pd(mask, vNiRevIdx, vNdRelS);
    vNdTmp = _mm512_maskz_mul_pd(mask, vNdFailP, vNdTmp);

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_maskz_fmsub_pd(mask, vNdTmp, vNdRelS, vNdC);
    vNdTSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);
    vNdC = _mm512_maskz_sub_pd(mask, _mm512_maskz_sub_pd(mask, vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * One product is implicit with FMA operation
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
        vNdTmp = _mm512_set1_pd(data->switchReliability[time + idx]);
        vNdRelS = _mm512_maskz_loadu_pd(loadMask, &data->standbyReliabilityRev[loadBase]);
        vNdRelS = _mm512_maskz_permutexvar_pd(stepMask, vNiRevIdx, vNdRelS);
        vNdTmp = _mm512_maskz_mul_pd(stepMask, vNdFailP, vNdTmp);

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_maskz_fmsub_pd(stepMask, vNdTmp, vNdRelS, vNdC);
        vNdTSum = _mm512_mask_add_pd(vNdSum, stepMask, vNdSum, vNdY);
        vNdC = _mm512_mask_sub_pd(vNdC, stepMask, _mm512_maskz_sub_pd(stepMask, vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;
    }

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = _mm512_maskz_loadu_pd(mask, &data->primaryFailureDensity[time]);
    vNdTmp = _mm512_maskz_loadu_pd(mask, &data->switchReliability[time]);
    vNdFailP = _mm512_maskz_mul_pd(mask, vNdFailP, v8dHalfs);
    vNdRelS = _mm512_set1_pd(data->standbyReliabilityRev[data->numTimes - 1]);
    vNdTmp = _mm512_maskz_mul_pd(mask, vNdFailP, vNdTmp);

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_maskz_fmsub_pd(mask, vNdTmp, vNdRelS, vNdC);
    vNdTSum = _mm512_maskz_add_pd(mask, vNdSum, vNdY);
    vNdC = _mm512_maskz_sub_pd(mask, _mm512_sub_pd(vNdTSum, vNdSum), vNdY);

    /* Apply Kahan compensation to clean the accumulated sums */
    vNdSum = _mm512_maskz_sub_pd(mask, vNdTSum, vNdC);

    /* Multiply the partial result with the delta time */
    vNdTmp = _mm512_set1_pd(data->deltaT);
    vNdSum = _mm512_maskz_mul_pd(mask, vNdSum, vNdTmp);

    return vNdSum;
}

/**
 * rbdIntegralHotStandbyVNdAvx512f
 *
 * Compute integrals for Hot Stand-by function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes (at most) eight integrals for a Hot Stand-by RBD step
 *  exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute (at most):
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+2}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+3}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+4}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+5}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+6}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+7}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (__m512d):
 *  The result of the (at most) eight integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx512f") __m512d rbdIntegralHotStandbyVNdAvx512f(__mmask8 mask, struct rbdHotStandbyData *data, unsigned int time)
{
    __m512d vNdSum;
    __m512d vNdC;
    __m512d vNdY;
    __m512d vNdTSum;
    __m512d vNdTmp;
    __m512d vNdFailP;
    __mmask8 stepMask;
    unsigned int idx;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        mask &= ~0x1U;
    }

    vNdSum = v8dZeros;
    vNdC = v8dZeros;

    /* Early return if the mask is 0 */
    if (mask == 0x0U) {
        return vNdSum;
    }

    /* For each eight-tuple of internal time instants... */
    idx = 1;
    while ((idx + V8D) <= time) {
        vNdFailP = _mm512_loadu_pd(&data->primaryFailureDensity[idx]);
        vNdTmp = _mm512_loadu_pd(&data->switchReliability[idx]);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_fmsub_pd(vNdTmp, vNdFailP, vNdC);
        vNdTSum = _mm512_add_pd(vNdSum, vNdY);
        vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;

        /* Increment index (tau) */
        idx += V8D;
    }

    /* Compute mask for the management of the tail */
    stepMask = (__mmask8)_cvtu32_mask16((1U << (time - idx)) - 1);

    /**
     * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$ for the tail time instants
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
     */
    vNdFailP = _mm512_maskz_loadu_pd(stepMask, &data->primaryFailureDensity[idx]);
    vNdTmp = _mm512_maskz_loadu_pd(stepMask, &data->switchReliability[idx]);

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_fmsub_pd(vNdTmp, vNdFailP, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
     * Lanes 1 to 7 get 0.0 so they don't alter their accumulator
     */
    vNdFailP = _mm512_set_pd(0.0,
                             0.0,
                             0.0,
                             0.0,
                             0.0,
                             0.0,
                             0.0,
                             0.5 * data->primaryFailureDensity[0]);
    vNdTmp = _mm512_set_pd(0.0,
                           0.0,
                           0.0,
                           0.0,
                           0.0,
                           0.0,
                           0.0,
                           data->switchReliability[0]);

    /**
     * Compute $R_{swi}(0) \cdot f_{pri}(0)$
     * The weight is 0.5 (trapezoidal rule for external instants)
     * This product is implicit with FMA operation
     */

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_fmsub_pd(vNdTmp, vNdFailP, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * First horizontal reduction to merge the eight lanes
     * - Swap Lanes 0 and 1, swap Lanes 2 and 3, swap Lanes 4 and 5 and swap Lanes 6 and 7
     * - Add Lane 1 to Lane 0, add Lane 3 to Lane 2, add Lane 5 to Lane 4 and add Lane 7 to Lane 6
     *   (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     */

    /* Swap Lanes 0 and 1, swap Lanes 2 and 3, swap Lanes 4 and 5 and swap Lanes 6 and 7 */
    vNdTmp = _mm512_shuffle_pd(vNdSum, vNdSum, 0x55);
    vNdFailP = _mm512_shuffle_pd(vNdC, vNdC, 0x55);

    /**
     * Add Lane 1 to Lane 0, add Lane 3 to Lane 2, add Lane 5 to Lane 4 and add Lane 7 to Lane 6
     * (and viceversa) using Kahan's method
     */
    vNdY = _mm512_sub_pd(vNdTmp, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    vNdY = _mm512_sub_pd(_mm512_sub_pd(v8dZeros, vNdFailP), vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * Second horizontal reduction to merge the eight lanes
     * - Swap Lanes 0 and 2, swap Lanes 1 and 3, swap Lanes 4 and 6 and swap Lanes 5 and 7
     * - Add Lane 2 to Lane 0, add Lane 3 to Lane 1, add Lane 6 to Lane 4 and add Lane 7 to Lane 5
     *   (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     */

    /* Swap Lanes 0 and 2, swap Lanes 1 and 3, swap Lanes 4 and 6 and swap Lanes 5 and 7 */
    vNdTmp = _mm512_shuffle_f64x2(vNdSum, vNdSum, 0xB1);
    vNdFailP = _mm512_shuffle_f64x2(vNdC, vNdC, 0xB1);

    /**
     * Add Lane 2 to Lane 0, add Lane 3 to Lane 1, add Lane 6 to Lane 4 and add Lane 7 to Lane 5
     * (and viceversa) using Kahan's method
     */
    vNdY = _mm512_sub_pd(vNdTmp, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    vNdY = _mm512_sub_pd(_mm512_sub_pd(v8dZeros, vNdFailP), vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /**
     * Third horizontal reduction to merge the eight lanes
     * - Swap Lanes 0 and 4, swap Lanes 1 and 5, swap Lanes 2 and 6 and swap Lanes 3 and 7
     * - Add Lane 4 to Lane 0, add Lane 5 to Lane 1, add Lane 6 to Lane 2 and add Lane 7 to Lane 3
     *   (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     * - Ensure that the result and the compensation values among the (at most) eight lanes are identical
     */

    /* Swap Lanes 0 and 4, swap Lanes 1 and 5, swap Lanes 2 and 6 and swap Lanes 3 and 7 */
    vNdTmp = _mm512_shuffle_f64x2(vNdSum, vNdSum, 0x4E);
    vNdFailP = _mm512_shuffle_f64x2(vNdC, vNdC, 0x4E);

    /**
     * Add Lane 4 to Lane 0, add Lane 5 to Lane 1, add Lane 6 to Lane 2 and add Lane 7 to Lane 3
     * (and viceversa) using Kahan's method
     */
    vNdY = _mm512_sub_pd(vNdTmp, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    vNdY = _mm512_sub_pd(_mm512_sub_pd(v8dZeros, vNdFailP), vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
    vNdSum = vNdTSum;

    /* Ensure that the result and the compensation values among the four lanes are identical */
    vNdSum = _mm512_mask_blend_pd(mask, v8dZeros, vNdSum);
    vNdC = _mm512_mask_blend_pd(mask, v8dZeros, vNdC);

    /**
     * First step - Compute missing internal nodes in interval [time, currIdx)
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

        vNdFailP = _mm512_maskz_mov_pd(stepMask, _mm512_set1_pd(data->primaryFailureDensity[time + idx]));
        vNdTmp = _mm512_maskz_mov_pd(stepMask, _mm512_set1_pd(data->switchReliability[time + idx]));

        /* Add the current product to the result using the Kahan's method */
        vNdY = _mm512_fmsub_pd(vNdTmp, vNdFailP, vNdC);
        vNdTSum = _mm512_add_pd(vNdSum, vNdY);
        vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);
        vNdSum = vNdTSum;
    }

    /**
     * Second step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = _mm512_maskz_loadu_pd(mask, &data->primaryFailureDensity[time]);
    vNdTmp = _mm512_maskz_loadu_pd(mask, &data->switchReliability[time]);
    vNdFailP = _mm512_maskz_mul_pd(mask, vNdFailP, v8dHalfs);

    /* Add the current product to the result using the Kahan's method */
    vNdY = _mm512_fmsub_pd(vNdTmp, vNdFailP, vNdC);
    vNdTSum = _mm512_add_pd(vNdSum, vNdY);
    vNdC = _mm512_sub_pd(_mm512_sub_pd(vNdTSum, vNdSum), vNdY);

    /* Apply Kahan compensation to clean the accumulated sums */
    vNdSum = _mm512_maskz_sub_pd(mask, vNdTSum, vNdC);

    /* Multiply the partial result with the delta time */
    vNdTmp = _mm512_set1_pd(data->deltaT);
    vNdSum = _mm512_maskz_mul_pd(mask, vNdSum, vNdTmp);

    return vNdSum;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
