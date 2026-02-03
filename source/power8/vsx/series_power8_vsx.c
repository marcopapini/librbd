/*
 *  Component: series_power8_vsx.c
 *  Series RBD management - Optimized using POWER8 VSX instruction set
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
#include "../series_power8.h"


/**
 * rbdSeriesGenericWorkerVsx
 *
 * Generic Series RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorkerVsx(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerVsx
 *
 * Identical Series RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorkerVsx(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* Align, if possible, to vector size */
    if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
        if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
            /* Compute reliability of Series RBD at current time instant */
            rbdSeriesIdenticalStepS1d(data, time);
            /* Increment current time instant */
            time += S1D;
        }
    }
    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepS1d(data, time);
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepV2dVsx
 *
 * Generic Series RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdSeriesGenericStepV2dVsx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    double64x2 v2dTmp;
    double64x2 v2dRes;

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = vectorLoad(&data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        v2dTmp = vectorLoad(&data->reliabilities[(component * data->numTimes) + time]);
        v2dRes = vec_mul(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdSeriesIdenticalStepV2dVsx
 *
 * Identical Series RBD step function with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step exploiting POWER8 VSX 128bit.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdSeriesIdenticalStepV2dVsx(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    double64x2 v2dTmp;
    double64x2 v2dRes;

    /* Load reliability */
    v2dTmp = vectorLoad(&data->reliabilities[time]);

    /* Compute reliability of Series RBD at current time instant */
    v2dRes = v2dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        v2dRes = vec_mul(v2dRes, v2dTmp);
    }

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
