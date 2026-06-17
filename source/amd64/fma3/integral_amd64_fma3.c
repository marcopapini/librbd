/*
 *  Component: integral_amd64_fma3.c
 *  Compute integral for RBD management - Optimized using amd64 FMA3 instruction set
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
#include "../integral_amd64.h"


/**
 * rbdIntegralColdStandbyV4dFma3
 *
 * Compute integrals for Cold Stand-by function with amd64 FMA3 256bit
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
 *  exploiting amd64 FMA3 256bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_P(\tau) R_S(t+1-\tau) d\tau}$
 *  - $\int_0^{t+2}{f_P(\tau) R_S(t+2-\tau) d\tau}$
 *  - $\int_0^{t+3}{f_P(\tau) R_S(t+3-\tau) d\tau}$
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the four integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("fma") __m256d rbdIntegralColdStandbyV4dFma3(struct rbdColdStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dY;
    __m256d v4dTSum;
    __m256d v4dFailP;
    __m256d v4dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT3;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        v4dSum = rbdIntegralColdStandbyV1dFma3(data, 1);
        v4dC = rbdIntegralColdStandbyV1dFma3(data, 2);
        v4dY = rbdIntegralColdStandbyV1dFma3(data, 3);
        v4dTSum = _mm256_unpacklo_pd(v4dZeros, v4dSum);
        v4dFailP = _mm256_unpacklo_pd(v4dC, v4dY);
        return _mm256_permute2f128_pd(v4dTSum, v4dFailP, 0x20);
    }

    v4dSum = v4dZeros;
    v4dC = v4dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT3 = startRevIdxT - (V4D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v4dFailP = _mm256_broadcast_sd(&data->primaryFailureDensity[idx]);
        v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdxT3 + idx]);
        v4dRelS = _mm256_permute2f128_pd(v4dRelS, v4dRelS, 1);
        v4dRelS = _mm256_permute_pd(v4dRelS, 0x5);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
        v4dTSum = _mm256_add_pd(v4dSum, v4dY);
        v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
        v4dSum = v4dTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    v4dFailP = _mm256_broadcast_sd(&data->primaryFailureDensity[0]);
    v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdxT3]);
    v4dFailP = _mm256_mul_pd(v4dFailP, v4dHalfs);
    v4dRelS = _mm256_permute2f128_pd(v4dRelS, v4dRelS, 1);
    v4dRelS = _mm256_permute_pd(v4dRelS, 0x5);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
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

    /**
     * First sub-step
     * This product is implicit with FMA operation
     */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             data->primaryFailureDensity[time],
                             0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 4],
                            data->standbyReliabilityRev[data->numTimes - 3],
                            data->standbyReliabilityRev[data->numTimes - 2],
                            0.0);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Second sub-step
     * This product is implicit with FMA operation
     */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 1],
                             data->primaryFailureDensity[time + 1],
                             0.0,
                             0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 3],
                            data->standbyReliabilityRev[data->numTimes - 2],
                            0.0,
                            0.0);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Third sub-step
     * This product is implicit with FMA operation
     */
    v4dFailP = _mm256_set_pd(data->primaryFailureDensity[time + 2],
                             0.0,
                             0.0,
                             0.0);
    v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[data->numTimes - 2],
                            0.0,
                            0.0,
                            0.0);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
    v4dSum = v4dTSum;

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[time]);
    v4dFailP = _mm256_mul_pd(v4dFailP, v4dHalfs);
    v4dRelS = _mm256_broadcast_sd(&data->standbyReliabilityRev[data->numTimes - 1]);

    /* Add the current product to the result using the Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
    v4dSum = _mm256_add_pd(v4dSum, v4dY);

    /* Multiply the partial result with the delta time */
    v4dTSum = _mm256_broadcast_sd(&data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTSum);

    return v4dSum;
}

/**
 * rbdIntegralColdStandbyV2dFma3
 *
 * Compute integrals for Cold Stand-by function with amd64 FMA3 128bit
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
 *  exploiting amd64 FMA3 128bit.
 *  It is responsible to compute:
 *  - $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_P(\tau) R_S(t+1-\tau) d\tau}$
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m128d):
 *  The result of the two integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("fma") __m128d rbdIntegralColdStandbyV2dFma3(struct rbdColdStandbyData *data, unsigned int time)
{
    __m128d v2dSum;
    __m128d v2dC;
    __m128d v2dY;
    __m128d v2dTSum;
    __m128d v2dFailP;
    __m128d v2dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT1;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return _mm_unpacklo_pd(v2dZeros, _mm256_castpd256_pd128(rbdIntegralColdStandbyV1dFma3(data, 1)));
    }

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT1 = startRevIdxT - (V2D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v2dFailP = _mm_load1_pd(&data->primaryFailureDensity[idx]);
        v2dRelS = _mm_loadu_pd(&data->standbyReliabilityRev[startRevIdxT1 + idx]);
        v2dRelS = _mm_shuffle_pd(v2dRelS, v2dRelS, 1);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        v2dY = _mm_fmsub_pd(v2dFailP, v2dRelS, v2dC);
        v2dTSum = _mm_add_pd(v2dSum, v2dY);
        v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    v2dFailP = _mm_load1_pd(&data->primaryFailureDensity[0]);
    v2dRelS = _mm_loadu_pd(&data->standbyReliabilityRev[startRevIdxT1]);
    v2dFailP = _mm_mul_pd(v2dFailP, v2dHalfs);
    v2dRelS = _mm_shuffle_pd(v2dRelS, v2dRelS, 1);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_fmsub_pd(v2dFailP, v2dRelS, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;
    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * This product is implicit with FMA operation
     */
    v2dFailP = _mm_set_pd(data->primaryFailureDensity[time], 0.0);
    v2dRelS = _mm_set_pd(data->standbyReliabilityRev[data->numTimes - 2], 0.0);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_fmsub_pd(v2dFailP, v2dRelS, v2dC);
    v2dTSum = _mm_add_pd(v2dSum, v2dY);
    v2dC = _mm_sub_pd(_mm_sub_pd(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    v2dFailP = _mm_loadu_pd(&data->primaryFailureDensity[time]);
    v2dFailP = _mm_mul_pd(v2dFailP, v2dHalfs);
    v2dRelS = _mm_load1_pd(&data->standbyReliabilityRev[data->numTimes - 1]);

    /* Add the current product to the result using the Kahan's method */
    v2dY = _mm_fmsub_pd(v2dFailP, v2dRelS, v2dC);
    v2dSum = _mm_add_pd(v2dSum, v2dY);

    /* Multiply the partial result with the delta time */
    v2dTSum = _mm_load1_pd(&data->deltaT);
    v2dSum = _mm_mul_pd(v2dSum, v2dTSum);

    return v2dSum;
}

/**
 * rbdIntegralColdStandbyV1dFma3
 *
 * Compute integral for Cold Stand-by function with amd64 FMA3 256bit
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
 *  exploiting amd64 FMA3 256bit.
 *  It is responsible to compute $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$,
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (__m256d):
 *  The result of the integral for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("fma") __m256d rbdIntegralColdStandbyV1dFma3(struct rbdColdStandbyData *data, unsigned int time)
{
    __m256d v4dSum;
    __m256d v4dC;
    __m256d v4dY;
    __m256d v4dTSum;
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

    /* For each pair of internal time instants... */
    idx = 1;
    while ((idx + V4D) <= time) {
        /* Load $f_P(\tau)$ and $f_P(\tau+1)$ */
        v4dFailP = _mm256_loadu_pd(&data->primaryFailureDensity[idx]);
        /* Load $R_S(t-\tau)$ and $R_S(t-\tau-1)$ */
        v4dRelS = _mm256_loadu_pd(&data->standbyReliabilityRev[startRevIdx + idx]);

        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$ for four steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         * This product is implicit with FMA operation
         */

        /* Add the current products to the result using the SIMD Kahan's method */
        v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
        v4dTSum = _mm256_add_pd(v4dSum, v4dY);
        v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
        v4dSum = v4dTSum;

        /* Increment index (tau) */
        idx += V4D;
    }

    switch (time - idx) {
        case 1:
            /**
             * Lane 0 gets the tail node $f_P(\tau) * R_S(t-\tau)
             * Lane 1 gets the external left time instant 0.5 * f_P(0) * R_S(t)
             * Lane 2 gets the external right time instant 0.5 * f_P(t) * R_S(0)
             * Lane 3 gets 0.0 so it doesn't alter its accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx],
                    data->standbyReliabilityRev[startRevIdx + idx]);
            break;
        case 2:
            /**
             * Lanes 0 and 1 get the tail nodes $f_P(\tau) * R_S(t-\tau)
             * Lane 2 gets the external left time instant 0.5 * f_P(0) * R_S(t)
             * Lane 3 gets the external right time instant 0.5 * f_P(t) * R_S(0)
             */
            v4dFailP = _mm256_set_pd(0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx],
                    data->standbyReliabilityRev[startRevIdx + idx + 1],
                    data->standbyReliabilityRev[startRevIdx + idx]);
            break;
        case 3:
            /**
             * Lanes 0, 1 and 2 get the tail nodes ($f_P(\tau)$ and $R_S(t-\tau)$)
             * Lane 3 gets 0.0 so it doesn't alter its accumulator
             */
            v4dFailP = _mm256_set_pd(0.0,
                    data->primaryFailureDensity[idx + 2],
                    data->primaryFailureDensity[idx + 1],
                    data->primaryFailureDensity[idx]);
            v4dRelS = _mm256_set_pd(0.0,
                    data->standbyReliabilityRev[startRevIdx + idx + 2],
                    data->standbyReliabilityRev[startRevIdx + idx + 1],
                    data->standbyReliabilityRev[startRevIdx + idx]);

            /**
             * Compute $f_P(\tau) \cdot R_S(t-\tau)$ for four steps at once
             * Weight is 1.0 (internal trapezoidal nodes)
             * This product is implicit with FMA operation
             */

            /* Add the current products to the result using the SIMD Kahan's method */
            v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
            v4dTSum = _mm256_add_pd(v4dSum, v4dY);
            v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);
            v4dSum = v4dTSum;

            /**
             * Compute external time instants (Left and Right) at once
             * Lane 0: 0.5 * f_P(0) * R_S(t)
             * Lane 1: 0.5 * f_P(t) * R_S(0)
             * Lanes 2 and 3: 0
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx]);
            break;
        case 0:
        default:
            /**
             * Compute external time instants (Left and Right) at once
             * Lane 0: 0.5 * f_P(0) * R_S(t)
             * Lane 1: 0.5 * f_P(t) * R_S(0)
             * Lanes 2 and 3: 0
             */
            v4dFailP = _mm256_set_pd(0.0,
                    0.0,
                    0.5 * data->primaryFailureDensity[time],
                    0.5 * data->primaryFailureDensity[0]);
            v4dRelS = _mm256_set_pd(0.0,
                    0.0,
                    data->standbyReliabilityRev[startRevIdx + time],
                    data->standbyReliabilityRev[startRevIdx]);
            break;
    }

    /**
     * Compute $f_P(\tau) \cdot R_S(t-\tau)$ for four steps at once
     * Weight is 1.0 (internal trapezoidal nodes)
     * This product is implicit with FMA operation
     */

    /* Add the current products to the result using the SIMD Kahan's method */
    v4dY = _mm256_fmsub_pd(v4dFailP, v4dRelS, v4dC);
    v4dTSum = _mm256_add_pd(v4dSum, v4dY);
    v4dC = _mm256_sub_pd(_mm256_sub_pd(v4dTSum, v4dSum), v4dY);

    /* Apply Kahan compensation to clean the accumulated sums before merging */
    v4dSum = _mm256_sub_pd(v4dTSum, v4dC);

    /* Horizontal reduction: Add Lane 1 to Lane 0 (and viceversa to broadcast the result) */
    v4dTSum = _mm256_permute2f128_pd(v4dSum, v4dSum, 1);
    v4dSum = _mm256_add_pd(v4dSum, v4dTSum);
    v4dTSum = _mm256_permute_pd(v4dSum, 0x5);
    v4dSum = _mm256_add_pd(v4dSum, v4dTSum);

    /**
     * Force Lanes 1, 2 and 3 to be 0.0 using blend
     * - Lane 0: taken from 2nd operand (v4dSum)
     * - Lanes 1, 2, 3: taken from 1st operand (v4dZeros)
     * Result: [0.0, 0.0, 0.0, v4dSum]
     */
    v4dSum = _mm256_blend_pd(v4dZeros, v4dSum, 0x01);

    /**
     * Multiply the final vector result with the delta time.
     * Since Lanes 1, 2 and 3 are 0.0, 0.0 * deltaT remains strictly 0.0
     */
    v4dTSum = _mm256_set1_pd(data->deltaT);
    v4dSum = _mm256_mul_pd(v4dSum, v4dTSum);

    return v4dSum;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
