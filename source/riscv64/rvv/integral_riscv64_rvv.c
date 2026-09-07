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
 *  - $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$
 *  - $\int_0^{t+1}{f_P(\tau) R_S(t+1-\tau) d\tau}$
 *  ...
 *  - $\int_0^{t+N-1}{f_P(\tau) R_S(t+N-1-\tau) d\tau}$
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
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
    vfloat64m1_t vNdRelS;
    vbool64_t stepMask;
    vuint64m1_t vLaneIdx;
    unsigned int idx;
    unsigned int startRevIdxT;
    unsigned char timeWasZero;

    /* Manage the case when the first integral is computed over the empty time domain */
    timeWasZero = 0;
    if (time == 0) {
        timeWasZero = 1;
        time = 1;
        /* Early return if the initial mask is empty */
        if (--vl == 0) {
            return __riscv_vfmv_v_f_f64m1(0.0, vl + 1);
        }
    }

    vNdSum = __riscv_vfmv_v_f_f64m1(0.0, vl);
    vNdC = __riscv_vfmv_v_f_f64m1(0.0, vl);

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdxT = data->numTimes - 1 - time;

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        vNdRelS = __riscv_vlse64_v_f64m1(&data->standbyReliabilityRev[startRevIdxT + idx], -8, vl);
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         * This product is implicit with FMA operation
         */

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vf_f64m1(vNdC, data->primaryFailureDensity[idx], vNdRelS, vl);
        vNdTSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);
        vNdC = __riscv_vfsub_vv_f64m1(__riscv_vfsub_vv_f64m1(vNdTSum, vNdSum, vl), vNdY, vl);
        vNdSum = vNdTSum;
    }

    /**
     * First step - Compute $f_P(0) \cdot R_S(currIdx)$
     * The weight is 0.5 (trapezoidal rule for external left instant)
     * This product is implicit with FMA operation
     */
    vNdRelS = __riscv_vlse64_v_f64m1(&data->standbyReliabilityRev[startRevIdxT], -8, vl);

    /* Add the current product to the result using the Kahan's method */
    vNdY = __riscv_vfmsac_vf_f64m1(vNdC, 0.5 * data->primaryFailureDensity[0], vNdRelS, vl);
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

        vNdRelS = __riscv_vlse64_v_f64m1_m(stepMask, &data->standbyReliabilityRev[data->numTimes - 1 + idx], -8, vl);

        /* Add the current product to the result using the Kahan's method */
        vNdY = __riscv_vfmsac_vf_f64m1_m(stepMask, vNdC, data->primaryFailureDensity[time + idx], vNdRelS, vl);
        vNdTSum = __riscv_vfadd_vv_f64m1_tumu(stepMask, vNdSum, vNdSum, vNdY, vl);
        vNdC = __riscv_vfsub_vv_f64m1_tumu(stepMask, vNdC, __riscv_vfsub_vv_f64m1_m(stepMask, vNdTSum, vNdSum, vl), vNdY, vl);
        vNdSum = vNdTSum;

        ++idx;
    }

    /**
     * Third step - Compute $f_P(currIdx) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external right instant)
     * This product is implicit with FMA operation
     */
    vNdFailP = __riscv_vle64_v_f64m1(&data->primaryFailureDensity[time], vl);
    vNdFailP = __riscv_vfmul_vf_f64m1(vNdFailP, 0.5, vl);

    /* Add the current product to the result using the Kahan's method (no need to save vNdC here) */
    vNdY = __riscv_vfmsac_vf_f64m1(vNdC, data->standbyReliabilityRev[data->numTimes - 1], vNdFailP, vl);
    vNdSum = __riscv_vfadd_vv_f64m1(vNdSum, vNdY, vl);

    /* Multiply the partial result with the delta time */
    vNdSum = __riscv_vfmul_vf_f64m1(vNdSum, data->deltaT, vl);

    /* Generate correct result vector if original time was equal to 0 */
    if (timeWasZero != 0) {
        vNdTSum = __riscv_vfmv_v_f_f64m1(0.0, vl + 1);
        vNdSum = __riscv_vslideup_vx_f64m1(vNdTSum, vNdSum, 1, vl + 1);
    }

    return vNdSum;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
