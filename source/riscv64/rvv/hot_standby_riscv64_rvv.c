/*
 *  Component: hot_standby_riscv64_rvv.c
 *  Hot Stand-by RBD management - Optimized using RISC-V 64bit RVV instruction set
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
#include "../hot_standby_riscv64.h"
#include "../integral_riscv64.h"


static void rbdHotStandbyStepVNdRvv(struct rbdHotStandbyData *data, unsigned int time, unsigned long int vl);


/**
 * rbdHotStandbyWorkerRvv
 *
 * Hot Stand-by RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdHotStandbyWorkerRvv(struct rbdHotStandbyData *data)
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
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + vl);
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + vl);
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepVNdRvv(data, time, vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdHotStandbyStepVNdRvv
 *
 * Hot Stand-by RBD step function with RISC-V 64bit RVV
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
 *  This function implements the Hot Stand-by RBD step exploiting RISC-V 64bit RVV.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *      vl: Vector Length
 */
static FUNCTION_TARGET("arch=+v") void rbdHotStandbyStepVNdRvv(struct rbdHotStandbyData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdTmp;
    vfloat64m1_t vNdPri;
    vfloat64m1_t vNdRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    vNdPri = __riscv_vle64_v_f64m1(&data->primaryReliability[time], vl);
    vNdTmp = __riscv_vle64_v_f64m1(&data->standbyReliability[time], vl);
    vNdRes = rbdIntegralHotStandbyVNdRvv(data, time, vl);
    vNdRes = __riscv_vfmacc_vv_f64m1(vNdPri, vNdRes, vNdTmp, vl);

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
