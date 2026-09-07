/*
 *  Component: integral_aarch64_sve.c
 *  Compute integral for RBD management - Optimized using AArch64 SVE instruction set
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

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../integral_aarch64.h"


#if !defined(COMPILER_VS)
/**
 * rbdIntegralColdStandbyVNdSve
 *
 * Compute integrals for Cold Stand-by function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes N integrals for a Cold Stand-by RBD step
 *  exploiting AArch64 SVE.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+1-\tau) d\tau}$
 *  ...
 *  - $\int_0^{t+N-1}{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t+N-1-\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (svfloat64_t):
 *  The result of the N integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("+sve") svfloat64_t rbdIntegralColdStandbyVNdSve(svbool_t pg, struct rbdColdStandbyData *data, unsigned int time)
{
    svfloat64_t vNdSum;
    svfloat64_t vNdC;
    svfloat64_t vNdMinusY;
    svfloat64_t vNdTSum;
    svfloat64_t vNdFailP;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRelS;
    svbool_t stepPg;
    svint64_t vNiOffsets;
    svint64_t vNiStepOffsets;
    unsigned int idx;
    unsigned int startRevIdxT;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        stepPg = svcmpne_n_u64(pg, svindex_u64(0, 1), 0);
        pg = svand_b_z(pg, pg, stepPg);
    }

    vNdSum = svdup_n_f64(0.0);
    vNdC = svdup_n_f64(0.0);

    /* Early return if the SVE predicate is empty */
    if (!svptest_any(svptrue_b64(), pg)) {
        return vNdSum;
    }

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;

    /* Dynamically generate the offset for the Gather Load operations */
    vNiOffsets = svindex_s64(0, -1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdFailP = svdup_n_f64(data->primaryFailureDensity[idx]);
        vNdTmp = svdup_n_f64(data->switchReliability[idx]);
        vNdRelS = svld1_gather_s64index_f64(pg, &data->standbyReliabilityRev[startRevIdxT + idx], vNiOffsets);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * One product is implicit with FMA operation
         */
        vNdTmp = svmul_f64_x(pg, vNdFailP, vNdTmp);

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_z(pg, vNdC, vNdTmp, vNdRelS);
        vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
        vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = svdup_n_f64(0.5 * data->primaryFailureDensity[0]);
    vNdTmp = svdup_n_f64(data->switchReliability[0]);
    vNdRelS = svld1_gather_s64index_f64(pg, &data->standbyReliabilityRev[startRevIdxT], vNiOffsets);
    vNdTmp = svmul_f64_x(pg, vNdFailP, vNdTmp);

    /* Add the current product to the result using the Kahan's method */
    vNdMinusY = svmls_f64_z(pg, vNdC, vNdTmp, vNdRelS);
    vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
    vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);
    vNdSum = vNdTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * One product is implicit with FMA operation
     */
    idx = (time == 0) ? 1 : 0;
    while (1) {
        /* Lane k needs a node if k > idx. We dynamically mask out lanes <= idx */
        stepPg = svcmpgt_n_u64(pg, svindex_u64(0, 1), idx);
        if (!svptest_any(svptrue_b64(), stepPg)) {
            /**
             * No Lane is active
             * We have completed the computation of internal nodes
             */
            break;
        }

        vNiStepOffsets = svadd_n_s64_x(stepPg, vNiOffsets, idx);
        vNdFailP = svdup_n_f64(data->primaryFailureDensity[time + idx]);
        vNdTmp = svdup_n_f64(data->switchReliability[time + idx]);
        vNdRelS  = svld1_gather_s64index_f64(stepPg, &data->standbyReliabilityRev[data->numTimes - 1], vNiStepOffsets);
        vNdTmp = svmul_f64_x(stepPg, vNdFailP, vNdTmp);

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_x(stepPg, vNdC, vNdTmp, vNdRelS);
        vNdTSum = svsub_f64_m(stepPg, vNdSum, vNdMinusY);
        vNdC = svsel_f64(stepPg, svadd_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY), vNdC);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = svld1_f64(pg, &data->primaryFailureDensity[time]);
    vNdTmp = svld1_f64(pg, &data->switchReliability[time]);
    vNdFailP = svmul_n_f64_x(pg, vNdFailP, 0.5);
    vNdRelS = svdup_n_f64(data->standbyReliabilityRev[data->numTimes - 1]);
    vNdTmp = svmul_f64_x(pg, vNdFailP, vNdTmp);

    /* Add the current product to the result using the Kahan's method */
    vNdMinusY = svmls_f64_x(pg, vNdC, vNdTmp, vNdRelS);
    vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
    vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);

    /* Apply Kahan compensation to clean the accumulated sums */
    vNdSum = svsub_f64_z(pg, vNdTSum, vNdC);

    /* Multiply the partial result with the delta time */
    vNdSum = svmul_n_f64_z(pg, vNdSum, data->deltaT);

    return vNdSum;
}

/**
 * rbdIntegralHotStandbyVNdSve
 *
 * Compute integrals for Hot Stand-by function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes N integrals for a Hot Stand-by RBD step
 *  exploiting AArch64 SVE.
 *  It is responsible to compute:
 *  - $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  - $\int_0^{t+1}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  ...
 *  - $\int_0^{t+N-1}{f_{pri}(\tau) R_{swi}(\tau) d\tau}$
 *  where f_{pri} is the failure density of the primary component and R_{swi} is the
 *  reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (svfloat64_t):
 *  The result of the N integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("+sve") svfloat64_t rbdIntegralHotStandbyVNdSve(svbool_t pg, struct rbdHotStandbyData *data, unsigned int time)
{
    svfloat64_t vNdSum;
    svfloat64_t vNdC;
    svfloat64_t vNdMinusY;
    svfloat64_t vNdTSum;
    svfloat64_t vNdTmp;
    svfloat64_t vNdFailP;
    svfloat64_t vNdZeros;
    svuint64_t vNuBaseIdx;
    svuint64_t vNuMask;
    svbool_t stepPg;
    unsigned int idx;
    unsigned int vectorSize;
    unsigned int numLanes;
    unsigned int dist;

    /* Manage the case when the first integral is computed over the empty time domain */
    if (time == 0) {
        stepPg = svcmpne_n_u64(pg, svindex_u64(0, 1), 0);
        pg = svand_b_z(pg, pg, stepPg);
    }

    vNdZeros = svdup_n_f64(0.0);
    vNdSum = vNdZeros;
    vNdC = vNdZeros;

    /* Early return if the SVE predicate is empty */
    if (!svptest_any(svptrue_b64(), pg)) {
        return vNdSum;
    }

    /* Retrieve total number of Lanes */
    numLanes = svcntd();

    /* For each VLEN-tuple of internal time instants... */
    idx = 1;
    while (idx < time) {
        stepPg = svwhilelt_b64(idx, time);
        vectorSize = svcntp_b64(svptrue_b64(), stepPg);

        vNdFailP = svld1_f64(stepPg, &data->primaryFailureDensity[idx]);
        vNdTmp = svld1_f64(stepPg, &data->switchReliability[idx]);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_x(stepPg, vNdC, vNdTmp, vNdFailP);
        vNdTSum = svsub_f64_m(stepPg, vNdSum, vNdMinusY);
        vNdC = svsel_f64(stepPg, svadd_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY), vNdC);
        vNdSum = vNdTSum;

        /* Increment index (tau) */
        idx += vectorSize;
    }

    /**
     * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
     * All other Lanes are disabled
     */
    stepPg = svptrue_pat_b64(SV_VL1);
    vNdFailP = svdup_n_f64(0.5 * data->primaryFailureDensity[0]);
    vNdTmp = svdup_n_f64(data->switchReliability[0]);

    /**
     * Compute $R_{swi}(0) \cdot f_{pri}(0)$
     * The weight is 0.5 (trapezoidal rule for external instants)
     * This product is implicit with FMA operation
     */

    /* Add the current product to the result using the Kahan's method */
    vNdMinusY = svmls_f64_x(stepPg, vNdC, vNdTmp, vNdFailP);
    vNdTSum = svsub_f64_m(stepPg, vNdSum, vNdMinusY);
    vNdC = svsel_f64(stepPg, svadd_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY), vNdC);
    vNdSum = vNdTSum;

    /**
     * Perform Vector Length Agnostic Horizontal Reduction
     * For each iteration perform the following operations:
     * - Compute the mask to swap Lanes
     * - Swap selected Lanes
     * - Add swapped Lanes using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     */
    stepPg = svptrue_b64();
    vNuBaseIdx = svindex_u64(0, 1);
    for (dist = 1; dist < numLanes; dist *= 2) {
        /* Compute the mask to swap Lanes */
        vNuMask = sveor_n_u64_z(stepPg, vNuBaseIdx, dist);

        /* Swap selected Lanes */
        vNdTmp = svtbl_f64(vNdSum, vNuMask);
        vNdFailP = svtbl_f64(vNdC, vNuMask);

        /* Add swapped Lanes using Kahan's method */
        vNdMinusY = svsub_f64_z(stepPg, vNdTmp, vNdC);
        vNdTSum = svadd_f64_z(stepPg, vNdSum, vNdMinusY);
        vNdC = svsub_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY);
        vNdSum = vNdTSum;

        /* Compensate the result using Kahan's method with the swapped compensation value */
        vNdMinusY = svsub_f64_z(stepPg, svsub_f64_z(stepPg, vNdZeros, vNdFailP), vNdC);
        vNdTSum = svadd_f64_z(stepPg, vNdSum, vNdMinusY);
        vNdC = svsub_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY);
        vNdSum = vNdTSum;
    }

    /* Ensure that the result and the compensation values among the N Lanes are identical */
    vNdSum = svsel_f64(pg, svdup_lane_f64(vNdSum, 0), vNdZeros);
    vNdC = svsel_f64(pg, svdup_lane_f64(vNdC, 0), vNdZeros);

    /**
     * First step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
     */
    idx = (time == 0) ? 1 : 0;
    while (1) {
        /* Lane k needs a node if k > step. We dynamically mask out lanes <= step */
        stepPg = svcmpgt_n_u64(pg, vNuBaseIdx, idx);
        if (!svptest_any(pg, stepPg)) {
            /**
             * No Lane is active
             * We have completed the computation of internal nodes, early exit
             */
            break;
        }

        vNdFailP = svdup_n_f64_z(stepPg, data->primaryFailureDensity[time + idx]);
        vNdTmp = svdup_n_f64_z(stepPg, data->switchReliability[time + idx]);

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_x(stepPg, vNdC, vNdTmp, vNdFailP);
        vNdTSum = svsub_f64_m(stepPg, vNdSum, vNdMinusY);
        vNdC = svsel_f64(stepPg, svadd_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY), vNdC);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Second step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = svld1_f64(pg, &data->primaryFailureDensity[time]);
    vNdTmp = svld1_f64(pg, &data->switchReliability[time]);
    vNdFailP = svmul_f64_z(pg, vNdFailP, svdup_n_f64(0.5));

    /* Add the current product to the result using the Kahan's method */
    vNdMinusY = svmls_f64_z(pg, vNdC, vNdTmp, vNdFailP);
    vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
    vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);

    /* Apply Kahan compensation to clean the accumulated sums */
    vNdSum = svsub_f64_z(pg, vNdTSum, vNdC);

    /* Multiply the partial result with the delta time */
    vNdTmp = svdup_n_f64(data->deltaT);
    vNdSum = svmul_f64_z(pg, vNdSum, vNdTmp);

    return vNdSum;
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
