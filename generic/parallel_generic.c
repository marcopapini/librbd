/*
 *  Component: parallel_generic.c
 *  Parallel RBD management - Generic implementation
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


#include "rbd_internal_generic.h"

#include "../parallel.h"


#ifndef DISABLE_GENERIC_FUNCTIONS
/**
 * rbdParallelGenericWorker
 *
 * Parallel RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Parallel RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of a Parallel RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Parallel RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Parallel RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelGenericWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelGenericStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}

/**
 * rbdParallelIdenticalWorker
 *
 * Identical Parallel RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of an identical Parallel RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Parallel RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Parallel RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdParallelIdenticalWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Parallel RBD at current time instant */
        rbdParallelIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}
#endif /* DISABLE_GENERIC_FUNCTIONS */

/**
 * rbdParallelGenericStepS1d
 *
 * Generic Parallel RBD step function
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Parallel RBD step.
 *  It is responsible to compute the reliability of a Parallel block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN void rbdParallelGenericStepS1d(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    double s1dRes;

    /* Compute reliability of Parallel RBD at current time instant */
    s1dRes = (1.0 - data->reliabilities[(0 * data->numTimes) + time]);
    for (component = 1; component < data->numComponents; ++component) {
        s1dRes *= (1.0 - data->reliabilities[(component * data->numTimes) + time]);
    }
    s1dRes = (1.0 - s1dRes);

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdParallelIdenticalStepS1d
 *
 * Identical Parallel RBD step function
 *
 * Input:
 *      struct rbdParallelData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Parallel RBD step.
 *  It is responsible to compute the reliability of a Parallel block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Parallel RBD data structure
 *      time: current time instant over which Parallel RBD shall be computed
 */
HIDDEN void rbdParallelIdenticalStepS1d(struct rbdParallelData *data, unsigned int time)
{
    unsigned char component;
    double s1dU;
    double s1dRes;

    /* Load unreliability */
    s1dU = (1.0 - data->reliabilities[time]);

    /* Compute reliability of Parallel RBD at current time instant */
    s1dRes = s1dU;
    for (component = (data->numComponents - 1); component > 0; --component) {
        s1dRes *= s1dU;
    }
    s1dRes = (1.0 - s1dRes);

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}



