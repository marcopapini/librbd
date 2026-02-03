/*
 *  Component: bridge_aarch64_sve.c
 *  Bridge RBD management - Optimized using AArch64 SVE instruction set
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
#include "../bridge_aarch64.h"


/**
 * rbdBridgeGenericWorkerSve
 *
 * Generic Bridge RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdBridgeGenericWorkerSve(struct rbdBridgeData *data)
{
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorkerSve
 *
 * Identical Bridge RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdBridgeData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdBridgeIdenticalWorkerSve(struct rbdBridgeData *data)
{
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdBridgeGenericStepVNdSve
 *
 * Generic Bridge RBD step function with AArch64 SVE
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdBridgeGenericStepVNdSve(struct rbdBridgeData *data, unsigned int time)
{
    svbool_t pg;
    svfloat64_t vNdR1, vNdR2, vNdR3, vNdR4, vNdR5;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Load reliabilities */
    vNdR1 = svld1(pg, &data->reliabilities[(0 * data->numTimes) + time]);
    vNdR2 = svld1(pg, &data->reliabilities[(1 * data->numTimes) + time]);
    vNdR3 = svld1(pg, &data->reliabilities[(2 * data->numTimes) + time]);
    vNdR4 = svld1(pg, &data->reliabilities[(3 * data->numTimes) + time]);
    vNdR5 = svld1(pg, &data->reliabilities[(4 * data->numTimes) + time]);

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
    vNdTmp1 = svadd_f64_x(pg, vNdR1, vNdR3);
    vNdTmp2 = svadd_f64_x(pg, vNdR2, vNdR4);
    vNdTmp1 = svmls_f64_x(pg, vNdTmp1, vNdR1, vNdR3);
    vNdTmp2 = svmls_f64_x(pg, vNdTmp2, vNdR2, vNdR4);
    vNdRes  = svmul_f64_x(pg, vNdTmp1, vNdTmp2);
    /* At this point v2dRes vector contains VAL1 value */
    vNdTmp1 = svmul_f64_x(pg, vNdR3, vNdR4);
    vNdTmp2 = svmul_f64_x(pg, vNdR1, vNdR2);
    vNdTmp1 = svmls_f64_x(pg, vNdTmp1, vNdTmp1, vNdTmp2);
    vNdTmp1 = svadd_f64_x(pg, vNdTmp1, vNdTmp2);
    /* At this point v2dTmp1 vector contains VAL2 value */
    vNdRes = svsub_f64_x(pg, vNdRes, vNdTmp1);
    vNdRes = svmla_f64_x(pg, vNdTmp1, vNdR5, vNdRes);

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdBridgeIdenticalStepVNdSve
 *
 * Identical Bridge RBD step function with AArch64 SVE
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdBridgeIdenticalStepVNdSve(struct rbdBridgeData *data, unsigned int time)
{
    svbool_t pg;
    svfloat64_t vNdR, vNdU;
    svfloat64_t vNdTmp;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Load reliability */
    vNdR = svld1(pg, &data->reliabilities[time]);

    /* Compute unreliability */
    vNdU = svsub_f64_x(pg, svdup_f64(1.0), vNdR);

    /* Compute reliability of Bridge block */
    vNdRes = svmls_f64_x(pg, svdup_f64(2.0), vNdR, vNdR);
    vNdTmp = svmla_f64_x(pg, svdup_f64(-2.0), vNdU, vNdU);
    vNdRes = svmul_f64_x(pg, vNdRes, vNdR);
    vNdTmp = svmla_f64_x(pg, vNdRes, vNdTmp, vNdU);
    vNdTmp = svmla_f64_x(pg, svdup_f64(1.0), vNdTmp, vNdU);
    vNdRes = svmul_f64_x(pg, vNdTmp, vNdR);

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
