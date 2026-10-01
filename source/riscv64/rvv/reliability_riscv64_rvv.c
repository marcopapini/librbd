/*
 *  Component: reliability_riscv64_rvv.c
 *  Reliability RBD management - Optimized using RISC-V 64bit RVV instruction set
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
#include "../reliability_riscv64.h"


/**
 * rbdUnreliabilityWorkerRvv
 *
 * Unreliability RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdUnreliabilityWorkerRvv(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    unsigned long int vl;
    vfloat64m1_t vNdRes;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(0);

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        vl = __riscv_vsetvl_e64m1(data->numTimes - time);
        /* Prefetch for next iteration */
        prefetchRead(data->reliability, 1, data->numTimes, time + vl);
        prefetchWrite(data->unreliability, 1, data->numTimes, time + vl);
        /* Compute unreliability at current time instant */
        vNdRes = __riscv_vle64_v_f64m1(&data->reliability[time], vl);
        vNdRes = __riscv_vfrsub_vf_f64m1(vNdRes, 1.0, vl);
        __riscv_vse64_v_f64m1(&data->unreliability[time], capReliabilityVNdRvv(vNdRes, vl), vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerRvv
 *
 * Reliability Sum RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdReliabilitySumWorkerRvv(struct rbdReliabilityData *data)
{
    unsigned int time;
    unsigned long int vl;
    vfloat64m1_t vNdRes;
    vfloat64m1_t vNdTmp;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(0);

    /* Retrieve first time instant to be processed by worker */
    time = 0;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        vl = __riscv_vsetvl_e64m1(data->numTimes - time);
        /* Prefetch for next iteration */
        prefetchRead(data->r1, 1, data->numTimes, time + vl);
        prefetchRead(data->r2, 1, data->numTimes, time + vl);
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Compute reliability sum at current time instant */
        vNdRes = __riscv_vle64_v_f64m1(&data->r1[time], vl);
        vNdTmp = __riscv_vle64_v_f64m1(&data->r2[time], vl);
        vNdRes = __riscv_vfadd_vv_f64m1(vNdRes, vNdTmp, vl);
        __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
