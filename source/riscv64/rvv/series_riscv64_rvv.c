/*
 *  Component: series_riscv64_rvv.c
 *  Series RBD management - Optimized using RISC-V 64bit RVV instruction set
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
#include "rbd_internal_riscv64_rvv.h"
#include "../rbd_internal_riscv64.h"
#include "../series_riscv64.h"


/**
 * rbdSeriesGenericWorkerRvv
 *
 * Generic Series RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdSeriesGenericWorkerRvv(struct rbdSeriesData *data)
{
    unsigned int batchSize;
    unsigned int time;
    unsigned int timeEnd;
    unsigned long int vl;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivisionRiscv64Rvv(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimumRiscv64Rvv(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        vl = __riscv_vsetvl_e64m1(timeEnd - time);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + vl);
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepVNdRvv(data, time, vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerRvv
 *
 * Identical Series RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdSeriesIdenticalWorkerRvv(struct rbdSeriesData *data)
{
    unsigned int batchSize;
    unsigned int time;
    unsigned int timeEnd;
    unsigned long int vl;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivisionRiscv64Rvv(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimumRiscv64Rvv(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        vl = __riscv_vsetvl_e64m1(timeEnd - time);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + vl);
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepVNdRvv(data, time, vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepVNdRvv
 *
 * Generic Series RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 *      vl: Vector Length
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdSeriesGenericStepVNdRvv(struct rbdSeriesData *data, unsigned int time, unsigned long int vl)
{
    unsigned char component;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdRes;

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = __riscv_vle64_v_f64m1(&data->reliabilities[(0 * data->numTimes) + time], vl);
    for (component = 1; component < data->numComponents; ++component) {
        vNdTmp = __riscv_vle64_v_f64m1(&data->reliabilities[(component * data->numTimes) + time], vl);
        vNdRes = __riscv_vfmul_vv_f64m1(vNdRes, vNdTmp, vl);
    }

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdSeriesIdenticalStepVNdRvv
 *
 * Identical Series RBD step function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 *      vl: Vector Length
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdSeriesIdenticalStepVNdRvv(struct rbdSeriesData *data, unsigned int time, unsigned long int vl)
{
    unsigned char component;
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdRes;

    /* Load reliability */
    vNdTmp = __riscv_vle64_v_f64m1(&data->reliabilities[time], vl);

    /* Compute reliability of Series RBD at current time instant */
    vNdRes = vNdTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        vNdRes = __riscv_vfmul_vv_f64m1(vNdRes, vNdTmp, vl);
    }

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
