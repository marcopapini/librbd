/*
 *  Component: integral_power8_vsx.c
 *  Compute integral for RBD management - Optimized using POWER8 VSX instruction set
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

#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_power8.h"
#include "../integral_power8.h"


/**
 * rbdIntegralHotStandbyCommonVsx
 *
 * Compute the common part of the integral for Hot Stand-by functions with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *      double64x2 *v2dOutC
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the common part of the integral for a Hot Stand-by RBD step
 *  exploiting POWER8 VSX 128bit.
 *  It computes the common part of $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *      v2dOutC: filled with the Kahan's method compensation value
 *
 * Return (double64x2):
 *  The common part of the integral for Hot Stand-by computation
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("vsx") double64x2 rbdIntegralHotStandbyCommonVsx(
        struct rbdHotStandbyData *data,
        unsigned int time,
        double64x2 *v2dOutC)
{
    double64x2 v2dSum;
    double64x2 v2dC;
    double64x2 v2dRelS;
    double64x2 v2dFailP;
    double64x2 v2dY;
    double64x2 v2dTSum;
    unsigned int idx;

    v2dSum  = v2dZeros;
    v2dC = v2dZeros;

    /* For each pair of internal time instants... */
    idx = 1;
    while ((idx + V2D) <= time) {
        /* Load $f_{pri}(\tau)$ to $f_{pri}(\tau+3)$ */
        v2dFailP = vectorLoad(&data->primaryFailureDensity[idx]);
        /* Load $R_{swi}(\tau)$ to $R_{swi}(\tau+3)$ */
        v2dRelS = vectorLoad(&data->switchReliability[idx]);

        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$ for four steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         * This product is implicit with FMA operation
         */

        /* Add the current products to the result using the SIMD Kahan's method */
        v2dY = vec_msub(v2dFailP, v2dRelS, v2dC);
        v2dTSum = vec_add(v2dSum, v2dY);
        v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;

        /* Increment index (tau) */
        idx += V2D;
    }

    /* Is 1 time instant remaining? (Tail handling, purely in vector) */
    if (idx < time) {
        /**
         * Lane 0 gets the initial node 0.5 * f_{pri}(0) * R_{swi}(0)
         * Lane 1 gets the tail node f_{pri}(\tau) * R_{swi}(\tau)
         */
        v2dFailP = vec_insert(data->primaryFailureDensity[idx],
                              vec_splats(0.5 * data->primaryFailureDensity[0]), 1);
        v2dRelS = vec_insert(data->switchReliability[idx],
                             vec_splats(data->switchReliability[0]), 1);
    }
    else {
        /**
         * Lane 0 gets the initial node 0.5 * f_{pri}(0) * R_{swi}(0)
         * Lane 1 gets 0.0 so it doesn't alter its accumulator
         */
        v2dFailP = vec_insert(0.5 * data->primaryFailureDensity[0], v2dZeros, 0);
        v2dRelS = vec_insert(data->switchReliability[0], v2dZeros, 0);
    }

    /**
     * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$ for up to two steps at once
     * This product is implicit with FMA operation
     */

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dY = vec_msub(v2dFailP, v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Horizontal reduction to merge the two lanes
     * - Swap the results
     * - Add Lane 1 to Lane 0 (and viceversa) using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     * - Ensure that the result and the compensation values among the two lanes are identical
     */

    /* Swap Lanes 0 and 1 */
    v2dRelS = vec_xxpermdi(v2dSum, v2dSum, 0x02);
    v2dFailP = vec_xxpermdi(v2dC, v2dC, 0x02);

    /* Add Lane 1 to Lane 0 (and viceversa) using Kahan's method */
    v2dY = vec_sub(v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /* Compensate the result using Kahan's method with the swapped compensation value */
    v2dY = vec_sub(vec_sub(v2dZeros, v2dFailP), v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /* Ensure that the result and the compensation values among the two lanes are identical */
    v2dSum  = vec_splat(v2dSum, 0x00);
    *v2dOutC = vec_splat(v2dC, 0x00);

    return v2dSum;
}


/**
 * rbdIntegralColdStandbyV2dVsx
 *
 * Compute integrals for Cold Stand-by function with POWER8 VSX 128bit
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
 *  exploiting POWER8 VSX 128bit.
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
 * Return (double64x2):
 *  The result of the two integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("vsx") double64x2 rbdIntegralColdStandbyV2dVsx(struct rbdColdStandbyData *data, unsigned int time)
{
    double64x2 v2dSum;
    double64x2 v2dC;
    double64x2 v2dY;
    double64x2 v2dTSum;
    double64x2 v2dFailP;
    double64x2 v2dTmp;
    double64x2 v2dRelS;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned int startRevIdxT1;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return vec_insert(vec_extract(rbdIntegralColdStandbyV1dVsx(data, 1), 0), v2dZeros, 1);
    }

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;
    startRevIdxT1 = startRevIdxT - (V2D - 1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        v2dFailP = vec_splats(data->primaryFailureDensity[idx]);
        v2dTmp = vec_splats(data->switchReliability[idx]);
        v2dRelS = vectorLoad(&data->standbyReliabilityRev[startRevIdxT1 + idx]);
        v2dRelS = vec_xxpermdi(v2dRelS, v2dRelS, 2);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * One product is implicit with FMA operation
         */
        v2dTmp = vec_mul(v2dFailP, v2dTmp);

        /* Add the current product to the result using the Kahan's method */
        v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
        v2dTSum = vec_add(v2dSum, v2dY);
        v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * One product is implicit with FMA operation
     */
    v2dFailP = vec_splats(data->primaryFailureDensity[0]);
    v2dTmp = vec_splats(data->switchReliability[0]);
    v2dRelS = vectorLoad(&data->standbyReliabilityRev[startRevIdxT1]);
    v2dFailP = vec_mul(v2dFailP, v2dHalfs);
    v2dRelS = vec_xxpermdi(v2dRelS, v2dRelS, 2);
    v2dTmp = vec_mul(v2dFailP, v2dTmp);

    /* Add the current product to the result using the Kahan's method */
    v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * One product is implicit with FMA operation
     */
    v2dFailP = vec_insert(data->primaryFailureDensity[idx], v2dZeros, 1);
    v2dTmp = vec_insert(data->switchReliability[idx], v2dZeros, 1);
    v2dRelS = vec_insert(data->standbyReliabilityRev[data->numTimes - 2], v2dZeros, 1);
    v2dTmp = vec_mul(v2dFailP, v2dTmp);

    /* Add the current product to the result using the Kahan's method */
    v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    v2dFailP = vectorLoad(&data->primaryFailureDensity[time]);
    v2dTmp = vectorLoad(&data->switchReliability[time]);
    v2dFailP = vec_mul(v2dFailP, v2dHalfs);
    v2dRelS = vec_splats(data->standbyReliabilityRev[data->numTimes - 1]);
    v2dTmp = vec_mul(v2dFailP, v2dTmp);

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = vec_sub(v2dTSum, v2dC);

    /* Multiply the partial result with the delta time */
    v2dTSum = vec_splats(data->deltaT);
    v2dSum = vec_mul(v2dSum, v2dTSum);

    return v2dSum;
}

/**
 * rbdIntegralColdStandbyV1dVsx
 *
 * Compute integral for Cold Stand-by function with POWER8 VSX 128bit
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
 *  exploiting POWER8 VSX 128bit.
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
 * Return (double64x2):
 *  The result of the integral for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("vsx") double64x2 rbdIntegralColdStandbyV1dVsx(struct rbdColdStandbyData *data, unsigned int time)
{
    double64x2 v2dSum;
    double64x2 v2dC;
    double64x2 v2dY;
    double64x2 v2dTSum;
    double64x2 v2dFailP;
    double64x2 v2dTmp;
    double64x2 v2dRelS;
    unsigned int idx;
    unsigned int startRevIdx;

    /* The integral is null (0.0) if the time domain is empty */
    if (time == 0) {
        return v2dZeros;
    }

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdx = data->numTimes - 1 - time;

    v2dSum = v2dZeros;
    v2dC = v2dZeros;

    /* For each pair of internal time instants... */
    idx = 1;
    while ((idx + V2D) <= time) {
        /* Load $f_{pri}(\tau)$ and $f_{pri}(\tau+1)$ */
        v2dFailP = vectorLoad(&data->primaryFailureDensity[idx]);
        /* Load $R_{swi}(\tau)$ and $R_{swi}(\tau+1)$ */
        v2dTmp = vectorLoad(&data->switchReliability[idx]);
        /* Load $R_{sec}(t-\tau)$ and $R_{sec}(t-\tau-1)$ */
        v2dRelS = vectorLoad(&data->standbyReliabilityRev[startRevIdx + idx]);

        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$ for two steps at once
         * Weight is 1.0 (internal trapezoidal nodes)
         * One product is implicit with FMA operation
         */
        v2dTmp = vec_mul(v2dFailP, v2dTmp);

        /* Add the current products to the result using the SIMD Kahan's method */
        v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
        v2dTSum = vec_add(v2dSum, v2dY);
        v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;

        /* Increment index (tau) */
        idx += V2D;
    }

    /* Is 1 time instant remaining? (Tail handling, purely in vector) */
    if (idx < time) {
        /**
         * Lane 0 gets the tail node f_{pri}(\tau) * R_{swi}(\tau) * R_{sec}(t-\tau)
         * Lane 1 gets 0.0 so it doesn't alter its accumulator
         */
        v2dFailP = vec_insert(data->primaryFailureDensity[idx], v2dZeros, 0);
        v2dTmp = vec_insert(data->switchReliability[idx], v2dZeros, 0);
        v2dRelS = vec_insert(data->standbyReliabilityRev[startRevIdx + idx], v2dZeros, 0);

        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$ for tail node
         * Weight is 1.0 (internal trapezoidal nodes)
         * One product is implicit with FMA operation
         */
        v2dTmp = vec_mul(v2dFailP, v2dTmp);

        /* Add the current products to the result using the SIMD Kahan's method */
        v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
        v2dTSum = vec_add(v2dSum, v2dY);
        v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
        v2dSum = v2dTSum;
    }

    /**
     * Compute external time instants (Left and Right) at once
     * Lane 0: 0.5 * R_{swi}(0) * f_{pri}(0) * R_{sec}(t)
     * Lane 1: 0.5 * R_{swi}(t) * f_{pri}(t) * R_{sec}(0)
     * One product is implicit with FMA operation
     */
    v2dFailP = vec_insert(0.5 * data->primaryFailureDensity[0],
                          vec_splats(0.5 * data->primaryFailureDensity[time]), 0);
    v2dTmp = vec_insert(data->switchReliability[0],
                        vec_splats(data->switchReliability[time]), 0);
    v2dRelS = vec_insert(data->standbyReliabilityRev[startRevIdx],
                         vec_splats(data->standbyReliabilityRev[startRevIdx + time]), 0);
    v2dTmp = vec_mul(v2dFailP, v2dTmp);

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dY = vec_msub(v2dTmp, v2dRelS, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = vec_sub(v2dTSum, v2dC);

    /* Horizontal reduction: Add Lane 1 to Lane 0 (and viceversa to broadcast the result) */
    v2dTSum = vec_xxpermdi(v2dSum, v2dSum, 2);
    v2dSum = vec_add(v2dSum, v2dTSum);

    /* Force Lane 1 to be 0.0 using vec_insert */
    v2dSum = vec_insert(0.0, v2dSum, 1);

    /**
     * Multiply the final vector result with the delta time.
     * Since Lane 1, is 0.0, 0.0 * deltaT remains strictly 0.0
     */
    v2dTSum = vec_splats(data->deltaT);
    v2dSum = vec_mul(v2dSum, v2dTSum);

    return v2dSum;
}

/**
 * rbdIntegralHotStandbyV2dVsx
 *
 * Compute integrals for Hot Stand-by function with POWER8 VSX 128bit
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
 *  exploiting POWER8 VSX 128bit.
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
 * Return (double64x2):
 *  The result of the two integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("vsx") double64x2 rbdIntegralHotStandbyV2dVsx(struct rbdHotStandbyData *data, unsigned int time)
{
    double64x2 v2dSum;
    double64x2 v2dC;
    double64x2 v2dY;
    double64x2 v2dTSum;
    double64x2 v2dTmp;
    double64x2 v2dFailP;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        return vec_insert(vec_extract(rbdIntegralHotStandbyV1dVsx(data, 1), 0), v2dZeros, 1);
    }

    /* Compute the common part of the integral for Hot Stand-by */
    v2dSum = rbdIntegralHotStandbyCommonVsx(data, time, &v2dC);

    /**
     * First step - Compute missing internal nodes in interval [time, time + 1)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * - Lane 0 does not have missing nodes
     * - Lane 1 has a single missing node for \tau=time
     * This product is implicit with FMA operation
     */

    v2dFailP = vec_insert(data->primaryFailureDensity[time], v2dZeros, 1);
    v2dTmp = vec_insert(data->switchReliability[time], v2dZeros, 1);

    /* Add the current product to the result using the Kahan's method */
    v2dY = vec_msub(v2dFailP, v2dTmp, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);
    v2dSum = v2dTSum;

    /**
     * Second step - Compute $f_{pri}(\tau) \cdot R_{swi}(\tau)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    v2dFailP = vectorLoad(&data->primaryFailureDensity[time]);
    v2dTmp = vectorLoad(&data->switchReliability[time]);
    v2dFailP = vec_mul(v2dFailP, v2dHalfs);

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dY = vec_msub(v2dFailP, v2dTmp, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = vec_sub(v2dTSum, v2dC);

    /* Multiply the partial result with the delta time */
    v2dTmp = vec_splats(data->deltaT);
    v2dSum = vec_mul(v2dSum, v2dTmp);

    return v2dSum;
}

/**
 * rbdIntegralHotStandbyV1dVsx
 *
 * Compute integral for Hot Stand-by function with POWER8 VSX 128bit
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
 *  exploiting POWER8 VSX 128bit.
 *  It is responsible to compute $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$,
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (double64x2):
 *  The result of the integral for Hot Stand-by computation in Lane 0
 */
HIDDEN FUNCTION_TARGET("vsx") double64x2 rbdIntegralHotStandbyV1dVsx(struct rbdHotStandbyData *data, unsigned int time)
{
    double64x2 v2dSum;
    double64x2 v2dC;
    double64x2 v2dTSum;
    double64x2 v2dY;
    double64x2 v2dFailP;
    double64x2 v2dTmp;

    /* The integral is null (0.0) if the time domain is empty */
    if (time == 0) {
        return v2dZeros;
    }

    /* Compute the common part of the integral for Hot Stand-by */
    v2dSum = rbdIntegralHotStandbyCommonVsx(data, time, &v2dC);

    /**
     * Compute external time instant (Right)
     * Lane 0: 0.5 * R_{swi}(t) * f_{pri}(t)
     * Lane 1 gets 0.0 so it doesn't alter its accumulator
     * This product is implicit with FMA operation
     */
    v2dFailP = vec_insert(0.5 * data->primaryFailureDensity[time], v2dZeros, 0);
    v2dTmp = vec_insert(data->switchReliability[time], v2dZeros, 0);

    /* Add the current products to the result using the SIMD Kahan's method */
    v2dY = vec_msub(v2dFailP, v2dTmp, v2dC);
    v2dTSum = vec_add(v2dSum, v2dY);
    v2dC = vec_sub(vec_sub(v2dTSum, v2dSum), v2dY);

    /* Apply Kahan compensation to clean the accumulated sums */
    v2dSum = vec_sub(v2dTSum, v2dC);

    /* Force Lane 1 to be 0.0 using vec_insert */
    v2dSum = vec_insert(0.0, v2dSum, 1);

    /**
     * Multiply the final vector result with the delta time.
     * Since Lane 1, is 0.0, 0.0 * deltaT remains strictly 0.0
     */
    v2dTSum = vec_splats(data->deltaT);
    v2dSum = vec_mul(v2dSum, v2dTSum);

    return v2dSum;
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
