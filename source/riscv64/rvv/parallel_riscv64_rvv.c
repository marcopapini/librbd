/*
 *  Component: parallel_riscv64_rvv.c
 *  Parallel RBD management - Optimized using RISC-V 64bit RVV instruction set
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
#include "../parallel_riscv64.h"


/**
 * rbdParallelGenericWorkerRvv
 *
 * Generic Parallel RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdParallelGenericWorkerRvv(struct rbdParallelData *data)
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
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepVNdRvv(data, time);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorkerRvv
 *
 * Identical Parallel RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdParallelIdenticalWorkerRvv(struct rbdParallelData *data)
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
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepVNdRvv(data, time);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdParallelGenericStepVNdRvv
 *
 * Generic Parallel RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdParallelGenericStepVNdRvv(struct rbdParallelData *data, unsigned int time)
{
    unsigned long int vl;
    unsigned char component;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdRes;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = __riscv_vle64_v_f64m1(&data->reliabilities[(0 * data->numTimes) + time], vl);
    vNdRes = __riscv_vfrsub_vf_f64m1(vNdRes, 1.0, vl);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = __riscv_vle64_v_f64m1(&data->reliabilities[(component * data->numTimes) + time], vl);
        vNdRes = __riscv_vfnmsac_vv_f64m1(vNdRes, vNdRes, vNdTmp, vl);
    }
    vNdRes = __riscv_vfrsub_vf_f64m1(vNdRes, 1.0, vl);

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdParallelIdenticalStepVNdRvv
 *
 * Identical Parallel RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdParallelIdenticalStepVNdRvv(struct rbdParallelData *data, unsigned int time)
{
    unsigned long int vl;
    unsigned char component;
    vfloat64m1_t vNdU;
    vfloat64m1_t vNdRes;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Load unreliability */
    vNdU = __riscv_vle64_v_f64m1(&data->reliabilities[time], vl);
    vNdU = __riscv_vfrsub_vf_f64m1(vNdU, 1.0, vl);

    /* Compute reliability of Parallel RBD at current time instant */
    vNdRes = vNdU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = __riscv_vfmul_vv_f64m1(vNdRes, vNdU, vl);
    }
    vNdRes = __riscv_vfrsub_vf_f64m1(vNdRes, 1.0, vl);

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
