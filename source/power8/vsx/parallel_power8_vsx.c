/*
 *  Component: parallel_power8_vsx.c
 *  Parallel RBD management - Optimized using POWER8 VSX instruction set
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
#include "../parallel_power8.h"


/**
 * rbdParallelGenericWorkerVsx
 *
 * Parallel RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelGenericWorkerVsx(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorkerVsx
 *
 * Identical Parallel RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdParallelData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelIdenticalWorkerVsx(struct rbdParallelData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Parallel RBD at current time instant */
            rbdParallelIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdParallelGenericStepV2dVsx
 *
 * Generic Parallel RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdParallelGenericStepV2dVsx(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    double64x2 v2dTmp;
    double64x2 v2dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    v2dRes = vectorLoad(&data->reliabilities[(0 * data->numTimes) + time]);
    v2dRes = vec_sub(v2dOnes, v2dRes);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = vectorLoad(&data->reliabilities[(component * data->numTimes) + time]);
        v2dRes = vec_nmadd(v2dRes, v2dTmp, v2dRes);
    }
    v2dRes = vec_sub(v2dOnes, v2dRes);

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdParallelIdenticalStepV2dVsx
 *
 * Identical Parallel RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdParallelIdenticalStepV2dVsx(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    double64x2 v2dU;
    double64x2 v2dRes;

    /* Load unreliability */
    v2dU = vectorLoad(&data->reliabilities[time]);
    v2dU = vec_sub(v2dOnes, v2dU);

    /* Compute reliability of Parallel RBD at current time instant */
    v2dRes = v2dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v2dRes = vec_mul(v2dRes, v2dU);
    }
    v2dRes = vec_sub(v2dOnes, v2dRes);

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
