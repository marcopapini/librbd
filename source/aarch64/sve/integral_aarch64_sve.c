/*
 *  Component: integral_aarch64_sve.c
 *  Compute integral for RBD management - Optimized using AArch64 SVE instruction set
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
 *  - $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_P(\tau) R_S(t+1-\tau) d\tau}$
 *  ...
 *  - $\int_0^{t+N-1}{f_P(\tau) R_S(t+N-1-\tau) d\tau}$
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
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
    svfloat64_t vNdRelS;
    svbool_t stepPg;
    svint64_t vOffsets;
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
    vOffsets = svindex_s64(0, -1);

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdFailP = svdup_n_f64(data->primaryFailureDensity[idx]);
        vNdRelS = svld1_gather_s64index_f64(pg, &data->standbyReliabilityRev[startRevIdxT + idx], vOffsets);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_z(pg, vNdC, vNdFailP, vNdRelS);
        vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
        vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    vNdFailP = svdup_n_f64(0.5 * data->primaryFailureDensity[0]);
    vNdRelS = svld1_gather_s64index_f64(pg, &data->standbyReliabilityRev[startRevIdxT], vOffsets);

    /* Add the current product to the result using the Kahan's method */
    vNdMinusY = svmls_f64_z(pg, vNdC, vNdFailP, vNdRelS);
    vNdTSum = svsub_f64_z(pg, vNdSum, vNdMinusY);
    vNdC = svadd_f64_z(pg, svsub_f64_z(pg, vNdTSum, vNdSum), vNdMinusY);
    vNdSum = vNdTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
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

        vNdFailP = svdup_n_f64(data->primaryFailureDensity[time + idx]);
        vNdRelS  = svld1_gather_s64index_f64(stepPg, &data->standbyReliabilityRev[data->numTimes - 1 + idx], vOffsets);

        /* Add the current product to the result using the Kahan's method */
        vNdMinusY = svmls_f64_x(stepPg, vNdC, vNdFailP, vNdRelS);
        vNdTSum = svsub_f64_m(stepPg, vNdSum, vNdMinusY);
        vNdC = svsel_f64(stepPg, svadd_f64_z(stepPg, svsub_f64_z(stepPg, vNdTSum, vNdSum), vNdMinusY), vNdC);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    vNdFailP = svld1_f64(pg, &data->primaryFailureDensity[time]);
    vNdFailP = svmul_n_f64_x(pg, vNdFailP, 0.5);
    vNdRelS = svdup_n_f64(data->standbyReliabilityRev[data->numTimes - 1]);

    /* Add the current product to the result using the Kahan's method (no need to save vNdC here) */
    vNdMinusY = svmls_f64_x(pg, vNdC, vNdFailP, vNdRelS);
    vNdSum = svsub_f64_m(pg, vNdSum, vNdMinusY);

    /* Multiply the partial result with the delta time */
    vNdSum = svmul_n_f64_z(pg, vNdSum, data->deltaT);

    return vNdSum;
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
