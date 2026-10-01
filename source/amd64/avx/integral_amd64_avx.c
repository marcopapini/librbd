/*
 *  Component: integral_amd64_avx.c
 *  Compute integral for RBD management - Optimized using amd64 AVX instruction set
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
 * rbdIntegralHotStandbyCommonAvx
 *
 * Compute the common part of the integral for Hot Stand-by functions with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *      __m256d *v4dOutC
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the common part of the integral for a Hot Stand-by RBD step
 *  exploiting amd64 AVX 256bit.
 *  It computes the common part of $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *      v4dOutC: filled with the Kahan's method compensation value
 *
 * Return (__m256d):
 *  The common part of the integral for Hot Stand-by computation
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("avx") __m256d rbdIntegralHotStandbyCommonAvx(
        struct rbdHotStandbyData *data,
        unsigned int time,
        __m256d *v4dOutC)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dRelS;
    __m256d v4dFailP;
    __m256d v4dProd;
    __m256d v4dY;
    __m256d v4dTSum;
    unsigned int idx;

    v4dSum  = v4dZeros;
    v4dC = v4dZeros;

    /* For each quadruple of internal time instants... */
    idx = 1;
    while ((idx + V4D) <= time) {
        /* Load $f_{pri}(\tau)$ to $f_{pri}(\tau+3)$ */
        v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[idx]);
        /* Load $R_{swi}(\tau)$ to $R_{swi}(\tau+3)$ */
        v4dRelS = _mm256_loadu_pd(&data->switchReliability[idx]);

        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$ for four steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         */
        v4dProd = _mm256_mul_pd(v4dRelS, v4dFailP);

        /* Add the current products to the result using the SIMD Kahan's method */
        v4dY = _mm256_sub_pd(v4dProd, v4dC);
        v4dTSum = _mm256_add_pd(v4dSum, v4dY);
        v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
        v4dSum = v4dTSum;

        /* Increment index (tau) */
        idx += V4D;
    }

    switch (time - idx) {
        case 1:
            /**
             * Lane 0 gets the tail node f_{pri}(\tau) * R_{swi}(\tau)
             * Lane 1 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0) * R_{sec}(t)
             * Lanes 2 and 3 get 0.0 so they don't alter their accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    data->switchReliability[0],
                    data->switchReliability[idx]);
            break;
        case 2:
            /**
             * Lanes 0 and 1 get the tail nodes f_{pri}(\tau) * R_{swi}(\tau)
             * Lane 2 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
             * Lane 3 gets 0.0 so it doesn't alter its accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    data->switchReliability[0],
                    data->switchReliability[idx + 1],
                    data->switchReliability[idx]);
            break;
        case 3:
            /**
             * Lanes 0, 1 and 2 get the tail nodes f_{pri}(\tau) * R_{swi}(\tau)
             * Lane 3 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
             */
            v4dFailP = _mm256_set_pd(0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx + 2],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(data->switchReliability[0],
                    data->switchReliability[idx + 2],
                    data->switchReliability[idx + 1],
                    data->switchReliability[idx]);
            break;
        case 0:
        default:
            /**
             * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
             * Lanes 1, 2 and 3 get 0.0 so they don't alter their accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[0]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    0.0,
                    data->switchReliability[0]);
            break;
    }

    /**
     * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$ for up to four steps at once
     */
    v4dProd = _mm256_mul_pd(v4dRelS, v4dFailP);

    /* Add the current products to the result using the SIMD Kahan's method */
    v4dY = _mm256_sub_pd(v4dProd, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * First horizontal reduction to merge the four lanes
     * - Swap Lanes 0 and 1 and swap Lanes 2 and 3
     * - Add Lane 1 to Lane 0, add Lane 3 to Lane 2 (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     */

    /* Swap Lanes 0 and 1 and swap Lanes 2 and 3 */
    v4dRelS = _mm256_shuffle_pd(v4dSum, v4dSum, 0x05);
    v4dFailP = _mm256_shuffle_pd(v4dC, v4dC, 0x05);

    /* Add Lane 1 to Lane 0, add Lane 3 to Lane 2 (and viceversa) using Kahan's method */
    v4dY = _mm256_sub_pd(v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    v4dY = _mm256_sub_pd(_mm256_sub_pd(v4dZeros, v4dFailP), v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Second horizontal reduction to merge the four lanes
     * - Swap Lanes 0 and 2 and swap Lanes 1 and 3
     * - Add Lane 2 to Lane 0, add Lane 3 to Lane 1 (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     * - Ensure that the result and the compensation values among the four lanes are identical
     */

    /* Swap Lanes 0 and 2 and swap Lanes 1 and 3 */
    v4dRelS = _mm256_permute2f128_pd(v4dSum, v4dSum, 0x01);
    v4dFailP = _mm256_permute2f128_pd(v4dC, v4dC, 0x01);

    /* Add Lane 2 to Lane 0, add Lane 3 to Lane 1 (and viceversa) using Kahan's method */
    v4dY = _mm256_sub_pd(v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    v4dY = _mm256_sub_pd(_mm256_sub_pd(v4dZeros, v4dFailP), v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Ensure that the result and the compensation values among the four lanes are identical */
    v4dSum = _mm256_permute2f128_pd(v4dSum, v4dSum, 0x00);
    v4dC = _mm256_permute2f128_pd(v4dC, v4dC, 0x00);
    v4dSum  = _mm256_shuffle_pd(v4dSum, v4dSum, 0x00);
    *v4dOutC = _mm256_shuffle_pd(v4dC, v4dC, 0x00);

    return v4dSum;
}


/**
 * rbdIntegralColdStandbyV4dAvx
 *
 * Compute integrals for Cold Stand-by function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes four integrals for a Cold Stand-by RBD step
 *  exploiting amd64 AVX 256bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+1-\tau) d\tau}$
 *  - $\int_0^{t+2}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+2-\tau) d\tau}$
 *  - $\int_0^{t+3}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+3-\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the four integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx") __m256d rbdIntegralColdStandbyV4dAvx(struct rbdColdStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dY;
    __m256d v4dTSum;
    __m256d v4dTmp;
    __m256d v4dFailP;
    __m256d v4dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT3;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        v4dSum = rbdIntegralColdStandbyV1dAvx(data, 1);
        v4dC = _mm256_castpd128_pd256(rbdIntegralColdStandbyV2dAvx(data, 2));
        return _mm256_insertf128_pd(
                    _mm256_castpd128_pd256(
                            _mm_shuffle_pd(v2dZeros, _mm256_castpd256_pd128(v4dSum), 0x00)),
                    _mm256_castpd256_pd128(v4dC), 1);
    }

    v4dSum = v4dZeros;
    v4dC = v4dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT3 = startRevIdxT - (V4D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v4dFailP = _mm256_broadcast_sd(&data->primaryFailureDensity[idx]);
        v4dTmp = _mm256_broadcast_sd(&data->switchReliability[idx]);
        v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdxT3 + idx]);
        v4dRelS = _mm256_permute2f128_pd(v4dRelS, v4dRelS, 1);
        v4dRelS = _mm256_permute_pd(v4dRelS, 0x5);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         */
        v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
        v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

        /* Add the current product to the result using the Kahan's method */
        v4dY = _mm256_sub_pd(v4dTmp, v4dC);
        v4dTSum = _mm256_add_pd(v4dSum, v4dY);
        v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
        v4dSum = v4dTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     */
    v4dFailP = _mm256_broadcast_sd(&data->primaryFailureDensity[0]);
    v4dTmp = _mm256_broadcast_sd(&data->switchReliability[0]);
    v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdxT3]);
    v4dRelS = _mm256_permute2f128_pd(v4dRelS, v4dRelS, 1);
    v4dRelS = _mm256_permute_pd(v4dRelS, 0x5);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * - Lane 2 has two missing nodes for \tau=time+1 and \tau=time
     * - Lane 3 has three missing nodes for \tau=time+2, \tau=time+1 and \tau=time
     */

    /* First sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time],
                           data->switchReliability[time],
                           data->switchReliability[time],
                           0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 4],
                            data->standbyReliabilityRev[data->numTimes - 3],
                            data->standbyReliabilityRev[data->numTimes - 2],
                            0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Second sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 1],
                             data->primaryFailureDensity[time + 1],
                             0.0,
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time + 1],
                           data->switchReliability[time + 1],
                           0.0,
                           0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 3],
                            data->standbyReliabilityRev[data->numTimes - 2],
                            0.0,
                            0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Third sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 2],
                             0.0,
                             0.0,
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time + 2],
                           0.0,
                           0.0,
                           0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 2],
                            0.0,
                            0.0,
                            0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     */
    v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[time]);
    v4dTmp = _mm256_loadu_pd(&data->switchReliability[time]);
    v4dRelS = _mm256_broadcast_sd(&data->standbyReliabilityRev[data->numTimes - 1]);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v4dSum = _mm256_sub_pd(v4dTSum, v4dC);

    /* Multiply the partial result with the delta time */
    v4dTmp = _mm256_broadcast_sd(&data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTmp);

    return v4dSum;
}


/**
 * rbdIntegralColdStandbyV2dAvx
 *
 * Compute integrals for Cold Stand-by function with amd64 AVX 128bit
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes two integrals for a Cold Stand-by RBD step
 *  exploiting amd64 AVX 128bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+1-\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m128d):
 *  The result of the two integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx") __m128d rbdIntegralColdStandbyV2dAvx(struct rbdColdStandbyData *data, unsigned int time)
{
    __m128d v2dSum;
    __m128d v2dC;
    __m128d v2dY;
    __m128d v2dTSum;
    __m128d v2dTmp;
    __m128d v2dFailP;
    __m128d v2dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT1;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return _mm_shuffle_pd(v2dZeros, _mm256_castpd256_pd128(rbdIntegralColdStandbyV1dAvx(data, 1)), 0);
    }

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT1 = startRevIdxT - (V2D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v2dFailP = _mm_load1_pd(&data->primaryFailureDensity[idx]);
        v2dTmp = _mm_load1_pd(&data->switchReliability[idx]);
        v2dRelS = _mm_loadu_pd(&data->standbyReliabilityRev[startRevIdxT1 + idx]);
        v2dRelS = _mm_shuffle_pd(v2dRelS, v2dRelS, 1);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         */
        v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);
        v2dTmp = _mm_mul_pd(v2dTmp, v2dRelS);

        /* Add the current product to the result using the Kahan's method */
        v2dY = _mm_sub_pd(v2dTmp, v2dC);
        v2dTSum = _mm_add_pd(v2dSum, v2dY);
        v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     */
    v2dFailP = _mm_load1_pd(&data->primaryFailureDensity[0]);
    v2dTmp = _mm_load1_pd(&data->switchReliability[0]);
    v2dRelS = _mm_loadu_pd(&data->standbyReliabilityRev[startRevIdxT1]);
    v2dRelS = _mm_shuffle_pd(v2dRelS, v2dRelS, 1);
    v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dRelS);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_sub_pd(v2dTmp, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     */
    v2dFailP = _mm_set_pd(data->primaryFailureDensity[time], 0.0);
    v2dTmp = _mm_set_pd(data->switchReliability[time], 0.0);
    v2dRelS = _mm_set_pd(data->standbyReliabilityRev[data->numTimes - 2], 0.0);
    v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dRelS);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_sub_pd(v2dTmp, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     */
    v2dFailP = _mm_loadu_pd(&data->primaryFailureDensity[time]);
    v2dTmp = _mm_loadu_pd(&data->switchReliability[time]);
    v2dRelS = _mm_load1_pd(&data->standbyReliabilityRev[data->numTimes - 1]);
    v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dRelS);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_sub_pd(v2dTmp, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = _mm_sub_pd(v2dTSum, v2dC);

    /* Multiply the partial result with the delta time */
    v2dTmp = _mm_set1_pd(data->deltaT);
    v2dSum = _mm_mul_pd(v2dSum, v2dTmp);

    return v2dSum;
}

/**
 * rbdIntegralColdStandbyV1dAvx
 *
 * Compute integral for Cold Stand-by function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the integral for a Cold Stand-by RBD step
 *  exploiting amd64 AVX 256bit.
 *  It is responsible to compute $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$,
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the integral for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx") __m256d rbdIntegralColdStandbyV1dAvx(struct rbdColdStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dY;
    __m256d v4dTSum;
    __m256d v4dTmp;
    __m256d v4dFailP;
    __m256d v4dRelS;
    unsigned int idx;
    unsigned int startRevIdx;

    /* The integral is null (0.0) if the time domain is empty */
    if (time == 0) {
        return v4dZeros;
    }

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdx = data->numTimes - 1 - time;

    v4dSum = v4dZeros;
    v4dC = v4dZeros;

    /* For each quadruple of internal time instants... */
    idx = 1;
    while ((idx + V4D) <= time) {
        /* Load $f_{pri}(\tau)$ to $f_{pri}(\tau+3)$ */
        v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[idx]);
        /* Load $R_{swi}(\tau)$ to $R_{swi}(\tau+3)$ */
        v4dTmp = _mm256_loadu_pd(&data->switchReliability[idx]);
        /* Load $R_{sec}(t-\tau)$ to $R_{sec}(t-\tau-3)$ */
        v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdx + idx]);

        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$ for four steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         */
        v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
        v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

        /* Add the current products to the result using the SIMD Kahan's method */
        v4dY = _mm256_sub_pd(v4dTmp, v4dC);
        v4dTSum = _mm256_add_pd(v4dSum, v4dY);
        v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
        v4dSum = v4dTSum;

        /* Increment index (tau) */
        idx += V4D;
    }

    switch (time - idx) {
        case 1:
            /**
             * Lane 0 gets the tail node f_{pri}(\tau) * R_{swi}(\tau) * R_{sec}(t-\tau)
             * Lane 1 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0) * R_{sec}(t)
             * Lane 2 gets the external right time instant 0.5 * f_{pri}(t) * R_{swi}(t) * R_{sec}(0)
             * Lane 3 gets 0.0 so it doesn't alter its accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx]);
            v4dTmp = _mm256_set_pd(0.0,
                    data->switchReliability[time],
                    data->switchReliability[0],
                    data->switchReliability[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx],
                    data->standbyReliabilityRev[startRevIdx + idx]);
            break;
        case 2:
            /**
             * Lanes 0 and 1 get the tail nodes f_{pri}(\tau) * R_{swi}(\tau) * R_{sec}(t-\tau)
             * Lane 2 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0) * R_{sec}(t)
             * Lane 3 gets the external right time instant 0.5 * f_{pri}(t) * R_{swi}(t) * R_{sec}(0)
             */
            v4dFailP = _mm256_set_pd(0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dTmp = _mm256_set_pd(data->switchReliability[time],
                    data->switchReliability[0],
                    data->switchReliability[idx + 1],
                    data->switchReliability[idx]);
            v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx],
                    data->standbyReliabilityRev[startRevIdx + idx + 1],
                    data->standbyReliabilityRev[startRevIdx + idx]);
            break;
        case 3:
            /**
             * Lanes 0, 1 and 2 get the tail nodes f_{pri}(\tau) * R_{swi}(\tau) * R_{sec}(t-\tau)
             * Lane 3 gets 0.0 so it doesn't alter its accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    data->primaryFailureDensity[idx + 2],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dTmp = _mm256_set_pd(0.0,
                    data->switchReliability[idx + 2],
                    data->switchReliability[idx + 1],
                    data->switchReliability[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    data->standbyReliabilityRev[startRevIdx + idx + 2],
                    data->standbyReliabilityRev[startRevIdx + idx + 1],
                    data->standbyReliabilityRev[startRevIdx + idx]);

            /**
             * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$ for three steps at once
             * Weight is 1.0 (internal trapezoidal nodes)
             */
            v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
            v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

            /* Add the current products to the result using the SIMD Kahan's method */
            v4dY = _mm256_sub_pd(v4dTmp, v4dC);
            v4dTSum = _mm256_add_pd(v4dSum, v4dY);
            v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
            v4dSum = v4dTSum;

            /**
             * Compute external time instants (Left and Right) at once
             * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0) * R_{sec}(t)
             * Lane 1 gets the external right time instant 0.5 * f_{pri}(t) * R_{swi}(t) * R_{sec}(0)
             * Lanes 2 and 3: 0
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0]);
            v4dTmp = _mm256_set_pd(0.0,
                    0.0,
                    data->switchReliability[time],
                    data->switchReliability[0]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx]);
            break;
        case 0:
        default:
            /**
             * Compute external time instants (Left and Right) at once
             * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0) * R_{sec}(t)
             * Lane 1 gets the external right time instant 0.5 * f_{pri}(t) * R_{swi}(t) * R_{sec}(0)
             * Lanes 2 and 3: 0
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0]);
            v4dTmp = _mm256_set_pd(0.0,
                    0.0,
                    data->switchReliability[time],
                    data->switchReliability[0]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx]);
            break;
    }

    /**
     * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$ for (up to) four steps at once
     * Weight is already integrated in f_{pri}(\tau)
     */
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dRelS);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v4dSum = _mm256_sub_pd(v4dTSum, v4dC);

    /* Horizontal reduction: Add Lane 2 to Lane 0, add Lane 3 to Lane 1 (and viceversa to broadcast the result) */
    v4dTmp = _mm256_permute2f128_pd(v4dSum, v4dSum, 1);
    v4dSum = _mm256_add_pd(v4dSum, v4dTmp);
    /* Horizontal reduction: Add Lane 1 to Lane 0, add Lane 3 to Lane 2 (and viceversa to broadcast the result) */
    v4dTmp = _mm256_shuffle_pd(v4dSum, v4dSum, 0x5);
    v4dSum = _mm256_add_pd(v4dSum, v4dTmp);

    /* Force Lanes 1, 2 and 3 to be 0.0 */
    v4dSum = _mm256_blend_pd(v4dZeros, v4dSum, 0x01);

    /**
     * Multiply the final vector result with the delta time.
     * Since Lanes 1, 2 and 3 are 0.0, 0.0 * deltaT remains strictly 0.0
     */
    v4dTmp = _mm256_set1_pd(data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTmp);

    return v4dSum;
}

/**
 * rbdIntegralHotStandbyV4dAvx
 *
 * Compute integrals for Hot Stand-by function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes four integrals for a Hot Stand-by RBD step
 *  exploiting amd64 AVX 256bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+2}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+3}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the four integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx") __m256d rbdIntegralHotStandbyV4dAvx(struct rbdHotStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dY;
    __m256d v4dTSum;
    __m256d v4dTmp;
    __m256d v4dFailP;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        v4dSum = rbdIntegralHotStandbyV1dAvx(data, 1);
        v4dC = _mm256_castpd128_pd256(rbdIntegralHotStandbyV2dAvx(data, 2));
        return _mm256_insertf128_pd(
                    _mm256_castpd128_pd256(
                            _mm_shuffle_pd(v2dZeros, _mm256_castpd256_pd128(v4dSum), 0x00)),
                    _mm256_castpd256_pd128(v4dC), 1);
    }

    /* Compute the common part of the integral for Hot Stand-by */
    v4dSum = rbdIntegralHotStandbyCommonAvx(data, time, &v4dC);

    /**
     * First step - Compute missing internal nodes in interval [time, time + 3)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * - Lane 2 has two missing nodes for \tau=time+1 and \tau=time
     * - Lane 3 has three missing nodes for \tau=time+2, \tau=time+1 and \tau=time
     */

    /* First sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time],
                           data->switchReliability[time],
                           data->switchReliability[time],
                           0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Second sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 1],
                             data->primaryFailureDensity[time + 1],
                             0.0,
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time + 1],
                           data->switchReliability[time + 1],
                           0.0,
                           0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /* Third sub-step */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 2],
                             0.0,
                             0.0,
                             0.0);
    v4dTmp = _mm256_set_pd(data->switchReliability[time + 2],
                           0.0,
                           0.0,
                           0.0);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Second step - Compute $f_{pri}(\tau) \cdot R_{swi}(\tau)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     */
    v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[time]);
    v4dTmp = _mm256_loadu_pd(&data->switchReliability[time]);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);
    v4dTmp = _mm256_mul_pd(v4dTmp, v4dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v4dSum = _mm256_sub_pd(v4dTSum, v4dC);

    /* Multiply the partial result with the delta time */
    v4dTmp = _mm256_set1_pd(data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTmp);

    return v4dSum;
}

/**
 * rbdIntegralHotStandbyV2dAvx
 *
 * Compute integrals for Hot Stand-by function with amd64 AVX 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes two integrals for a Hot Stand-by RBD step
 *  exploiting amd64 AVX 128bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (__m128d):
 *  The result of the two integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("avx") __m128d rbdIntegralHotStandbyV2dAvx(struct rbdHotStandbyData *data, unsigned int time)
{
    __m256d v4dTmp;
    __m128d v2dSum;
    __m128d v2dC;
    __m128d v2dY;
    __m128d v2dTSum;
    __m128d v2dTmp;
    __m128d v2dFailP;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return _mm_shuffle_pd(v2dZeros, _mm256_castpd256_pd128(rbdIntegralHotStandbyV1dAvx(data, 1)), 0);
    }

    /* Compute the common part of the integral for Hot Stand-by */
    v2dSum = _mm256_castpd256_pd128(rbdIntegralHotStandbyCommonAvx(data, time, &v4dTmp));
    v2dC = _mm256_castpd256_pd128(v4dTmp);

    /**
     * First step - Compute missing internal nodes in interval [time, time + 1)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     */
    v2dFailP = _mm_set_pd(data->primaryFailureDensity[time], 0.0);
    v2dTmp = _mm_set_pd(data->switchReliability[time], 0.0);
    v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_sub_pd(v2dTmp, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Second step - Compute $f_{pri}(\tau) \cdot R_{swi}(\tau)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     */
    v2dFailP = _mm_loadu_pd(&data->primaryFailureDensity[time]);
    v2dTmp = _mm_loadu_pd(&data->switchReliability[time]);
    v2dTmp = _mm_mul_pd(v2dFailP, v2dTmp);
    v2dTmp = _mm_mul_pd(v2dTmp, v2dHalfs);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_sub_pd(v2dTmp, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = _mm_sub_pd(v2dTSum, v2dC);

    /* Multiply the partial result with the delta time */
    v2dTmp = _mm_set1_pd(data->deltaT);
    v2dSum = _mm_mul_pd(v2dSum, v2dTmp);

    return v2dSum;
}

/**
 * rbdIntegralHotStandbyV1dAvx
 *
 * Compute integral for Hot Stand-by function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the integral for a Hot Stand-by RBD step
 *  exploiting amd64 AVX 256bit.
 *  It is responsible to compute $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$,
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the integral for Hot Stand-by computation in Lane 0
 */
HIDDEN FUNCTION_TARGET("avx") __m256d rbdIntegralHotStandbyV1dAvx(struct rbdHotStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dTSum;
    __m256d v4dY;
    __m256d v4dFailP;
    __m256d v4dTmp;

    /* The integral is null (0.0) if the time domain is empty */
    if (time == 0) {
        return v4dZeros;
    }

    /* Compute the common part of the integral for Hot Stand-by */
    v4dSum = rbdIntegralHotStandbyCommonAvx(data, time, &v4dC);

    /**
     * Compute external time instant (Right)
     * Lane 0: 0.5 * R_{swi}(t) * f_{pri}(t)
     * Lanes 1, 2 and 3 get 0.0 so they don't alter their accumulator
     */
    v4dFailP = _mm256_set_pd(0.0, 0.0, 0.0, 0.5 * data->primaryFailureDensity[time]);
    v4dTmp = _mm256_set_pd(0.0, 0.0, 0.0, data->switchReliability[time]);
    v4dTmp = _mm256_mul_pd(v4dFailP, v4dTmp);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_sub_pd(v4dTmp, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v4dSum = _mm256_sub_pd(v4dTSum, v4dC);

    /* Force Lanes 1, 2 and 3 to be 0.0 */
    v4dSum = _mm256_blend_pd(v4dZeros, v4dSum, 0x01);

    /**
     * Multiply the final vector result with the delta time.
     * Since Lane 1 is 0.0, 0.0 * deltaT remains strictly 0.0
     */
    v4dTmp = _mm256_set1_pd(data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTmp);

    return v4dSum;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
