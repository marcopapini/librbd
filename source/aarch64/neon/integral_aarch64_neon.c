/*
 *  Component: integral_aarch64_neon.c
 *  Compute integral for RBD management - Optimized using AArch64 NEON instruction set
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

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../integral_aarch64.h"


/**
 * rbdIntegralColdStandbyV2dNeon
 *
 * Compute integrals for Cold Stand-by function with AArch64 NEON 128bit
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
 *  exploiting AArch64 NEON 128bit.
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
 * Return (float64x2_t):
 *  The result of the two integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("+simd") float64x2_t rbdIntegralColdStandbyV2dNeon(struct rbdColdStandbyData *data, unsigned int time)
{
    float64x2_t v2dSum;
    float64x2_t v2dC;
    float64x2_t v2dMinusY;
    float64x2_t v2dTSum;
    float64x2_t v2dFailP;
    float64x2_t v2dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT1;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return vcombine_f64(vdup_n_f64(0.0), rbdIntegralColdStandbyV1dNeon(data, 1));
    }

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT1 = startRevIdxT - (V2D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v2dFailP = vdupq_n_f64(data->primaryFailureDensity[idx]);
        v2dRelS = vld1q_f64(&data->standbyReliabilityRev[startRevIdxT1 + idx]);
        v2dRelS = vextq_f64(v2dRelS, v2dRelS, 1);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
        v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
        v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);
        v2dSum = v2dTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    v2dFailP = vdupq_n_f64(data->primaryFailureDensity[0]);
    v2dRelS = vld1q_f64(&data->standbyReliabilityRev[startRevIdxT1]);
    v2dFailP = vmulq_f64(v2dFailP, v2dHalfs);
    v2dRelS = vextq_f64(v2dRelS, v2dRelS, 1);

    /* Add the current product to the result using the Kahan's method */
    v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
    v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
    v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);
    v2dSum = v2dTSum;
    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * This product is implicit with FMA operation
     */
    v2dFailP = vsetq_lane_f64(data->primaryFailureDensity[time], v2dZeros, 1);
    v2dRelS = vsetq_lane_f64(data->standbyReliabilityRev[data->numTimes - 2], v2dZeros, 1);

    /* Add the current product to the result using the Kahan's method */
    v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
    v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
    v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);
    v2dSum = v2dTSum;

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    v2dFailP = vld1q_f64(&data->primaryFailureDensity[time]);
    v2dFailP = vmulq_f64(v2dFailP, v2dHalfs);
    v2dRelS = vdupq_n_f64(data->standbyReliabilityRev[data->numTimes - 1]);

    /* Add the current product to the result using the Kahan's method */
    v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
    v2dSum = vsubq_f64(v2dSum, v2dMinusY);

    /* Multiply the partial result with the delta time */
    v2dTSum = vdupq_n_f64(data->deltaT);
    v2dSum = vmulq_f64(v2dSum, v2dTSum);

    return v2dSum;
}

/**
 * rbdIntegralColdStandbyV1dNeon
 *
 * Compute integral for Cold Stand-by function with AArch64 NEON 128bit
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
 *  exploiting AArch64 NEON 128bit.
 *  It is responsible to compute $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$,
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (float64x1_t):
 *  The result of the integral for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("+simd") float64x1_t rbdIntegralColdStandbyV1dNeon(struct rbdColdStandbyData *data, unsigned int time)
{
    float64x2_t v2dSum;
    float64x2_t v2dC;
    float64x2_t v2dMinusY;
    float64x2_t v2dTSum;
    float64x2_t v2dFailP;
    float64x2_t v2dRelS;
    unsigned int idx;
    unsigned int startRevIdx;

    /* The integral is null (0.0) if the time domain is empty */
    if (time == 0) {
        return v1dZeros;
    }

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdx = data->numTimes - 1 - time;

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* For each pair of internal time instants... */
    idx = 1;
    while ((idx + V2D) <= time) {
        /* Load $f_P(\tau)$ and $f_P(\tau+1)$ */
        v2dFailP = vld1q_f64(&data->primaryFailureDensity[idx]);
        /* Load $R_S(t-\tau)$ and $R_S(t-\tau-1)$ */
        v2dRelS = vld1q_f64(&data->standbyReliabilityRev[startRevIdx + idx]);

        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$ for two steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         * This product is implicit with FMA operation
         */

        /* Add the current products to the result using the SIMD Kahan's method */
        v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
        v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
        v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);
        v2dSum = v2dTSum;

        /* Increment index (tau) */
        idx += V2D;
    }

    /* Is 1 time instant remaining? (Tail handling, purely in vector) */
    if (idx < time) {
        /**
         * Lane 0 gets the tail node $f_P(\tau) * R_S(t-\tau)
         * Lane 1 gets 0.0 so it doesn't alter its accumulator
         */
        v2dFailP = vsetq_lane_f64(data->primaryFailureDensity[idx], v2dZeros, 0);
        v2dRelS = vsetq_lane_f64(data->standbyReliabilityRev[startRevIdx + idx], v2dZeros, 0);

        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$ for two steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         * This product is implicit with FMA operation
         */

        /* Add the current products to the result using the SIMD Kahan's method */
        v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
        v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
        v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);
        v2dSum = v2dTSum;
    }

    /**
     * Compute external time instants (Left and Right) at once
     * Lane 0: 0.5 * f_P(0) * R_S(t)
     * Lane 1: 0.5 * f_P(t) * R_S(0)
     * This product is implicit with FMA operation
     */
    v2dFailP = vsetq_lane_f64(0.5 * data->primaryFailureDensity[0], v2dZeros, 0);
    v2dFailP = vsetq_lane_f64(0.5 * data->primaryFailureDensity[time], v2dFailP, 1);
    v2dRelS  = vsetq_lane_f64(data->standbyReliabilityRev[startRevIdx], v2dZeros, 0);
    v2dRelS  = vsetq_lane_f64(data->standbyReliabilityRev[startRevIdx + time], v2dRelS, 1);

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dMinusY = vfmsq_f64(v2dC, v2dFailP, v2dRelS);
    v2dTSum = vsubq_f64(v2dSum, v2dMinusY);
    v2dC = vaddq_f64(vsubq_f64(v2dTSum, v2dSum), v2dMinusY);

    /* Apply Kahan compensation to clean the accumulated sums before merging */
    v2dSum = vsubq_f64(v2dTSum, v2dC);

    /* Horizontal reduction: Add Lane 1 to Lane 0 (and viceversa to broadcast the result) */
    v2dTSum = vextq_f64(v2dSum, v2dSum, 1);
    v2dSum = vaddq_f64(v2dSum, v2dTSum);

    /* Multiply the final vector result with the delta time. */
    v2dTSum = vdupq_n_f64(data->deltaT);
    v2dSum = vmulq_f64(v2dSum, v2dTSum);

    return vget_low_f64(v2dSum);
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
