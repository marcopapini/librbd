/*
 *  Component: hot_standby_power8_vsx.c
 *  Hot Stand-by RBD management - Optimized using POWER8 VSX instruction set
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

#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_power8.h"
#include "../hot_standby_power8.h"


/**
 * rbdHotStandbyWorkerVsx
 *
 * Hot Stand-by RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerVsx(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV1dVsx(data, time);
    }

    return NULL;
}

/**
 * rbdHotStandbyStepV2dVsx
 *
 * Hot Stand-by RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdHotStandbyStepV2dVsx(struct rbdHotStandbyData *data, unsigned int time)
{
    double64x2 v2dTmp;
    double64x2 v2dPri;
    double64x2 v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = vectorLoad(&data->primaryReliability[time]);
    v2dRes = vec_splats(data->pSwitch);
    v2dRes = vec_nmsub(v2dRes, v2dPri, v2dRes);
    v2dTmp = vectorLoad(&data->standbyReliability[time]);
    v2dRes = vec_madd(v2dRes, v2dTmp, v2dPri);

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdHotStandbyStepV1dVsx
 *
 * Hot Stand-by RBD step function with POWER8 VSX 64bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting POWER8 VSX 64bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdHotStandbyStepV1dVsx(struct rbdHotStandbyData *data, unsigned int time)
{
    double64x2 v2dTmp;
    double64x2 v2dPri;
    double64x2 v2dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v2dPri = vec_promote(data->primaryReliability[time], 0);
    v2dRes = vec_promote(data->pSwitch, 0);
    v2dRes = vec_nmsub(v2dRes, v2dPri, v2dRes);
    v2dTmp = vec_promote(data->standbyReliability[time], 0);
    v2dRes = vec_madd(v2dRes, v2dTmp, v2dPri);

    /* Cap the computed reliability and set it into output array */
    data->output[time] = vec_extract(capReliabilityV2dVsx(v2dRes), 0);
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
