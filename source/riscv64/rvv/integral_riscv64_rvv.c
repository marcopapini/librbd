/*
 *  Component: integral_riscv64_rvv.c
 *  Compute integral for RBD management - Optimized using RISC-V 64bit RVV instruction set
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

#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#include "rbd_internal_riscv64_rvv.h"
#include "../rbd_internal_riscv64.h"
#include "../integral_riscv64.h"


/**
 * rbdIntegralColdStandbyVNdRvv
 *
 * Compute integrals for Cold Stand-by function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes N integrals for a Cold Stand-by RBD step
 *  exploiting RISC-V 64bit RVV.
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
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *      vl: Vector Length
 *
 * Return (svfloat64_t):
 *  The result of the N integrals for Cold Stand-by computation
 */
HIDDEN FUNCTION_TARGET("arch=+v") vfloat64m1_t rbdIntegralColdStandbyVNdRvv(struct rbdColdStandbyData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdSum;
    vfloat64m1_t vNdC;
    vfloat64m1_t vNdY;
    vfloat64m1_t vNdTSum;
    vfloat64m1_t vNdFailP;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdRelS;
    vbool64_t stepMask;
    vuint64m1_t vLaneIdx;
    uintptr_t basePtr;
    unsigned long idx;
    unsigned int startRevIdxT;
    unsigned char timeWasZero;

    /* Manage the case when the first integral is computed over the empty time domain */
    timeWasZero = 0;
    if (time == 0) {
        timeWasZero = 1;
        time = 1;
        --vl;
    }

    /* Early return if the initial mask is empty */
    if (vl == 0) {
        return __riscv_vfmv_v_f_f64m1(0.0, timeWasZero ? vl + 1 : vl);
    }

    vNdSum = __riscv_vfmv_v_f_f64m1(0.0, vl);
    vNdC = __riscv_vfmv_v_f_f64m1(0.0, vl);

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdRelS = __riscv_vlse64_v_f64m1(&data->standbyReliabilityRev[startRevIdxT + idx], -8, vl);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vf_f64m1(vNdC, data->primaryFailureDensity[idx] * data->switchReliability[idx], vNdRelS, vl);
        vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);
        vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_{pri}(0) \cdot R_{swi}(0) \cdot R_{sec}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    vNdRelS = __riscv_vlse64_v_f64m1(&data->standbyReliabilityRev[startRevIdxT], -8, vl);

    /* Add the current product to the result using the Kahan's method */
    vNdY = __riscv_vfmsac_vf_f64m1(vNdC, 0.5 * data->primaryFailureDensity[0] * data->switchReliability[0], vNdRelS, vl);
    vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);
    vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);
    vNdSum = vNdTSum;

    /**
     * Second step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
     */
    idx = 0;
    vLaneIdx = __riscv_vid_v_u64m1(vl);
    while (1) {
        /* Lane k needs a node if k > idx. We dynamically mask out lanes <= idx */
        stepMask = __riscv_vmsgtu_vx_u64m1_b64(vLaneIdx, idx, vl);
        if (__riscv_vcpop_m_b64(stepMask, vl) == 0) {
            /**
             * No Lane is active
             * We have completed the computation of internal nodes
             */
            break;
        }

        basePtr = (uintptr_t)&data->standbyReliabilityRev[data->numTimes - 1] + (idx * sizeof(double));
        vNdRelS = __riscv_vlse64_v_f64m1_m(stepMask, (const double *)basePtr, -8, vl);

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vf_f64m1_m(stepMask, vNdC, data->primaryFailureDensity[time + idx] * data->switchReliability[time + idx], vNdRelS, vl);
        vNdTSum = __riscv_vfadd_vv_f64m1_tumu(stepMask, vNdSum, vNdSum, vNdY, vl);
        vNdC = __riscv_vfsub_vv_f64m1_tumu(stepMask, vNdC, __riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Third step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = __riscv_vle64_v_f64m1(&data->primaryFailureDensity[time], vl);
    vNdTmp = __riscv_vle64_v_f64m1(&data->switchReliability[time], vl);
    vNdFailP = __riscv_vfmul_vf_f64m1(vNdFailP, 0.5, vl);
    vNdTmp  = __riscv_vfmul_vv_f64m1(vNdFailP, vNdTmp, vl);

    /* Add the current product to the result using the Kahan's method (no need to save vNdC here) */
    vNdY = __riscv_vfmsac_vf_f64m1(vNdC, data->standbyReliabilityRev[data->numTimes - 1], vNdTmp, vl);
    vNdSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);

    /* Multiply the partial result with the delta time */
    vNdSum = __riscv_vfmul_vf_f64m1(vNdSum, data->deltaT, vl);

    /* Generate correct result vector if original time was equal to 0 */
    if (timeWasZero != 0) {
        ++vl;
        vNdTSum = __riscv_vfmv_v_f_f64m1(0.0, vl);
        vNdSum = __riscv_vslideup_vx_f64m1(vNdTSum, vNdSum, 1, vl);
    }

    return vNdSum;
}

/**
 * rbdIntegralHotStandbyVNdSve
 *
 * Compute integrals for Hot Stand-by function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes N integrals for a Hot Stand-by RBD step
 *  exploiting RISC-V 64bit RVV.
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
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *      vl: Vector Length
 *
 * Return (vfloat64m1_t):
 *  The result of the N integrals for Hot Stand-by computation
 */
HIDDEN FUNCTION_TARGET("arch=+v") vfloat64m1_t rbdIntegralHotStandbyVNdRvv(struct rbdHotStandbyData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdSum;
    vfloat64m1_t vNdC;
    vfloat64m1_t vNdY;
    vfloat64m1_t vNdTSum;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdFailP;
    vfloat64m1_t vNdZeros;
    vuint64m1_t vNuBaseIdx;
    vuint64m1_t vNuMask;
    vuint64m1_t vLaneIdx;
    vbool64_t stepMask;
    unsigned char timeWasZero;
    unsigned int idx;
    unsigned long int stepVl;
    unsigned int numLanes;
    unsigned int dist;

    /* Manage the case when the first integral is computed over the empty time domain */
    timeWasZero = 0;
    if (time == 0) {
        timeWasZero = 1;
        time = 1;
        --vl;
    }

    /* Early return if the initial mask is empty */
    if (vl == 0) {
        return __riscv_vfmv_v_f_f64m1(0.0, timeWasZero ? vl + 1 : vl);
    }

    /* Retrieve total number of Lanes */
    numLanes = __riscv_vsetvlmax_e64m1();

    vNdZeros = __riscv_vfmv_v_f_f64m1(0.0, numLanes);
    vNdSum = vNdZeros;
    vNdC = vNdZeros;

    /* For each VLEN-tuple of internal time instants... */
    idx = 1;
    while (idx < time) {
        stepVl = __riscv_vsetvl_e64m1(time - idx);

        vNdFailP = __riscv_vle64_v_f64m1(&data->primaryFailureDensity[idx], stepVl);
        vNdTmp = __riscv_vle64_v_f64m1(&data->switchReliability[idx], stepVl);
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vv_f64m1(vNdC, vNdFailP, vNdTmp, stepVl);
        vNdTSum = __riscv_vfadd_vv_f64m1_tu(vNdSum, vNdSum, vNdY, stepVl);
        vNdC = __riscv_vfsub_vv_f64m1_tu(vNdC, __riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, stepVl), vNdY, stepVl);
        vNdSum = vNdTSum;

        /* Increment index (tau) */
        idx += stepVl;
    }

    /**
     * Lane 0 gets the external left time instant 0.5 * f_{pri}(0) * R_{swi}(0)
     * All other Lanes are disabled
     */
    stepVl = 1;
    vNdFailP = __riscv_vfmv_v_f_f64m1(0.5 * data->primaryFailureDensity[0], stepVl);
    vNdTmp = __riscv_vfmv_v_f_f64m1(data->switchReliability[0], stepVl);

    /**
     * Compute $R_{swi}(0) \cdot f_{pri}(0)$
     * The weight is 0.5 (trapezoidal rule for external instants)
     * This product is implicit with FMA operation
     */

    /* Add the current product to the result using the Kahan's method */
    vNdY = __riscv_vfmsac_vv_f64m1(vNdC, vNdFailP, vNdTmp, stepVl);
    vNdTSum = __riscv_vfadd_vv_f64m1_tu(vNdSum, vNdSum, vNdY, stepVl);
    vNdC = __riscv_vfsub_vv_f64m1_tu(vNdC, __riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, stepVl), vNdY, stepVl);
    vNdSum = vNdTSum;

    /**
     * Perform Vector Length Agnostic Horizontal Reduction
     * For each iteration perform the following operations:
     * - Compute the mask to swap Lanes
     * - Swap selected Lanes
     * - Add swapped Lanes using Kahan's method
     * - Compensate the result using Kahan's method with the swapped compensation value
     */
    stepVl = __riscv_vsetvlmax_e64m1();
    vNuBaseIdx = __riscv_vid_v_u64m1(stepVl);
    for (dist = 1; dist < numLanes; dist *= 2) {
        /* Compute the mask to swap Lanes */
        vNuMask = __riscv_vxor_vx_u64m1(vNuBaseIdx, dist, stepVl);

        /* Swap selected Lanes */
        vNdTmp = __riscv_vrgather_vv_f64m1(vNdSum, vNuMask, stepVl);
        vNdFailP = __riscv_vrgather_vv_f64m1(vNdC, vNuMask, stepVl);

        /* Add swapped Lanes using Kahan's method */
        vNdY = __riscv_vfsub_vv_f64m1(vNdTmp, vNdC, stepVl);
        vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, stepVl);
        vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, stepVl), vNdY, stepVl);
        vNdSum = vNdTSum;

        /* Compensate the result using Kahan's method with the swapped compensation value */
        vNdY = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdZeros, vNdFailP, stepVl), vNdC, stepVl);
        vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, stepVl);
        vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, stepVl), vNdY, stepVl);
        vNdSum = vNdTSum;
    }

    /* Ensure that the result and the compensation values among the N Lanes are identical */
    vNdSum = __riscv_vfmv_v_f_f64m1_tu(vNdZeros, __riscv_vfmv_f_s_f64m1_f64(vNdSum), vl);
    vNdC = __riscv_vfmv_v_f_f64m1_tu(vNdZeros, __riscv_vfmv_f_s_f64m1_f64(vNdC), vl);

    /**
     * First step - Compute missing internal nodes in interval [time, currIdx)
     * The weight is 1.0 (trapezoidal rule for internal time instants)
     * This product is implicit with FMA operation
     */
    idx = 0;
    vLaneIdx = __riscv_vid_v_u64m1(vl);
    while (1) {
        /* Lane k needs a node if k > step. We dynamically mask out lanes <= step */
        stepMask = __riscv_vmsgtu_vx_u64m1_b64(vLaneIdx, idx, vl);
        if (__riscv_vcpop_m_b64(stepMask, vl) == 0) {
            /**
             * No Lane is active
             * We have completed the computation of internal nodes
             */
            break;
        }

        vNdFailP = __riscv_vfmv_v_f_f64m1(data->primaryFailureDensity[time + idx], vl);
        vNdTmp = __riscv_vfmv_v_f_f64m1(data->switchReliability[time + idx], vl);

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vv_f64m1(vNdC, vNdFailP, vNdTmp, vl);
        vNdTSum = __riscv_vfadd_vv_f64m1_tumu(stepMask, vNdSum, vNdSum, vNdY, vl);
        vNdC = __riscv_vfsub_vv_f64m1_tumu(stepMask, vNdC, __riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Second step - Compute $f_{pri}(currIdx) \cdot R_{swi}(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * One product is implicit with FMA operation
     */
    vNdFailP = __riscv_vle64_v_f64m1(&data->primaryFailureDensity[time], vl);
    vNdTmp = __riscv_vle64_v_f64m1(&data->switchReliability[time], vl);
    vNdFailP = __riscv_vfmul_vf_f64m1(vNdFailP, 0.5, vl);

    /* Add the current product to the result using the Kahan's method */
    vNdY = __riscv_vfmsac_vv_f64m1(vNdC, vNdTmp, vNdFailP, vl);
    vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);
    vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);

    /* Apply Kahan compensation to clean the accumulated sums */
    vNdSum = __riscv_vfsub_vv_f64m1(vNdTSum, vNdC, vl);

    /* Multiply the partial result with the delta time */
    vNdSum = __riscv_vfmul_vf_f64m1(vNdSum, data->deltaT, vl);

    /* Generate correct result vector if original time was equal to 0 */
    if (timeWasZero != 0) {
        ++vl;
        vNdTSum = __riscv_vfmv_v_f_f64m1(0.0, vl);
        vNdSum = __riscv_vslideup_vx_f64m1(vNdTSum, vNdSum, 1, vl);
    }

    return vNdSum;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
