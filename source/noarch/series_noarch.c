/*
 *  Component: series_noarch.c
 *  Series RBD management - Platform-independent implementation
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


#include "../generic/rbd_internal_generic.h"

#include "../series.h"


#if defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0
/**
 * rbdSeriesGenericWorker
 *
 * Generic Series RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorker(void *arg)
{
    struct rbdSeriesData *data;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;

    return rbdSeriesGenericWorkerNoarch(data);
}

/**
 * rbdSeriesIdenticalWorker
 *
 * Identical Series RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorker(void *arg)
{
    struct rbdSeriesData *data;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;

    return rbdSeriesIdenticalWorkerNoarch(data);
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdSeriesGenericWorkerNoarch
 *
 * Generic Series RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorkerNoarch(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesGenericStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdSeriesIdenticalWorkerNoarch
 *
 * Identical Series RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdSeriesData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      data: Series RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorkerNoarch(struct rbdSeriesData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Series RBD at current time instant */
        rbdSeriesIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdSeriesGenericStepS1d
 *
 * Generic Series RBD step function
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD step.
 *  It is responsible to compute the reliability of a Series block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN void rbdSeriesGenericStepS1d(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    double s1dRes;

    /* Compute reliability of Series RBD at current time instant */
    s1dRes = data->reliabilities[(0 * data->numTimes) + time];
    for (component = 1; component < data->numComponents; ++component) {
        s1dRes *= data->reliabilities[(component * data->numTimes) + time];
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdSeriesIdenticalStepS1d
 *
 * Identical Series RBD step function
 *
 * Input:
 *      struct rbdSeriesData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD step.
 *  It is responsible to compute the reliability of a Series block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Series RBD data structure
 *      time: current time instant over which Series RBD shall be computed
 */
HIDDEN void rbdSeriesIdenticalStepS1d(struct rbdSeriesData *data, unsigned int time)
{
    unsigned char component;
    double s1dTmp;
    double s1dRes;

    /* Load reliabilities */
    s1dTmp = data->reliabilities[time];

    /* Compute reliability of Series RBD at current time instant */
    s1dRes = s1dTmp;
    for (component = (data->numComponents - 1); component > 0; --component) {
        s1dRes *= s1dTmp;
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

