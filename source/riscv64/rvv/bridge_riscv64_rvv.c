/*
 *  Component: bridge_riscv64_rvv.c
 *  Bridge RBD management - Optimized using RISC-V 64bit RVV instruction set
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

#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_riscv64.h"
#include "../bridge_riscv64.h"


/**
 * rbdBridgeGenericWorkerRvv
 *
 * Generic Bridge RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdBridgeGenericWorkerRvv(struct rbdBridgeData *data)
{
    unsigned int time;
    unsigned long int vlmax;

    vlmax = __riscv_vsetvlmax_e64m1();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * vlmax;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * vlmax));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepVNdRvv(data, time);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorkerRvv
 *
 * Identical Bridge RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdBridgeIdenticalWorkerRvv(struct rbdBridgeData *data)
{
    unsigned int time;
    unsigned long int vlmax;

    vlmax = __riscv_vsetvlmax_e64m1();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * vlmax;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * vlmax));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepVNdRvv(data, time);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdBridgeGenericStepVNdRvv
 *
 * Generic Bridge RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdBridgeGenericStepVNdRvv(struct rbdBridgeData *data, unsigned int time)
{
    unsigned long int vl;
    vfloat64m1_t vNdR1, vNdR2, vNdR3, vNdR4, vNdR5;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdRes;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Load reliabilities */
    vNdR1 = __riscv_vle64_v_f64m1(&data->reliabilities[(0 * data->numTimes) + time], vl);
    vNdR2 = __riscv_vle64_v_f64m1(&data->reliabilities[(1 * data->numTimes) + time], vl);
    vNdR3 = __riscv_vle64_v_f64m1(&data->reliabilities[(2 * data->numTimes) + time], vl);
    vNdR4 = __riscv_vle64_v_f64m1(&data->reliabilities[(3 * data->numTimes) + time], vl);
    vNdR5 = __riscv_vle64_v_f64m1(&data->reliabilities[(4 * data->numTimes) + time], vl);

    /**
     * Formula:
     *   R = R5 * (1 - (F1 * F3)) * (1 - (F2 * F4)) + F5 * (1 - (1 - (R1 * R2)) * (1 - (R3 * R4)))
     *
     * Optimized formula:
     *   VAL1 = (R1 + R3 - (R1 * R3)) * (R2 + R4 - (R2 * R4))
     *   VAL2 = (R1 * R2) + (R3 * R4) - (R1 * R2 * R3 * R4)
     *   R = R5 * (VAL1 - VAL2) + VAL2
     */

    /* Compute reliability of Bridge block */
    vNdTmp1 = __riscv_vfadd_vv_f64m1(vNdR1, vNdR3, vl);
    vNdTmp2 = __riscv_vfadd_vv_f64m1(vNdR2, vNdR4, vl);
    vNdTmp1 = __riscv_vfnmsac_vv_f64m1(vNdTmp1, vNdR1, vNdR3, vl);
    vNdTmp2 = __riscv_vfnmsac_vv_f64m1(vNdTmp2, vNdR2, vNdR4, vl);
    vNdRes  = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmp2, vl);
    /* At this point v2dRes vector contains VAL1 value */
    vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdR3, vNdR4, vl);
    vNdTmp2 = __riscv_vfmul_vv_f64m1(vNdR1, vNdR2, vl);
    vNdTmp1 = __riscv_vfnmsac_vv_f64m1(vNdTmp1, vNdTmp1, vNdTmp2, vl);
    vNdTmp1 = __riscv_vfadd_vv_f64m1(vNdTmp1, vNdTmp2, vl);
    /* At this point v2dTmp1 vector contains VAL2 value */
    vNdRes = __riscv_vfsub_vv_f64m1(vNdRes, vNdTmp1, vl);
    vNdRes = __riscv_vfmacc_vv_f64m1(vNdTmp1, vNdR5, vNdRes, vl);

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdBridgeIdenticalStepVNdRvv
 *
 * Identical Bridge RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdBridgeIdenticalStepVNdRvv(struct rbdBridgeData *data, unsigned int time)
{
    unsigned long int vl;
    vfloat64m1_t vNdR, vNdU;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdRes;
    vfloat64m1_t vNdTwo;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Load reliability */
    vNdR = __riscv_vle64_v_f64m1(&data->reliabilities[time], vl);

    /* Compute unreliability */
    vNdU = __riscv_vfrsub_vf_f64m1(vNdR, 1.0, vl);

    /* Compute reliability of Bridge block */
    vNdTwo = __riscv_vfmv_v_f_f64m1(2.0, vl);
    vNdRes = __riscv_vfnmsac_vv_f64m1(vNdTwo, vNdR, vNdR, vl);
    vNdTmp = __riscv_vfmsac_vv_f64m1(vNdTwo, vNdU, vNdU, vl);
    vNdRes = __riscv_vfmul_vv_f64m1(vNdRes, vNdR, vl);
    vNdTmp = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp, vNdU, vl);
    vNdTmp = __riscv_vfmacc_vv_f64m1(__riscv_vfmv_v_f_f64m1(1.0, vl), vNdTmp, vNdU, vl);
    vNdRes = __riscv_vfmul_vv_f64m1(vNdTmp, vNdR, vl);

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
