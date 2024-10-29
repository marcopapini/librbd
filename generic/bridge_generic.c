/*
 *  Component: bridge_generic.c
 *  Bridge RBD management - Generic implementation
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


#include "../rbd_internal.h"

#include "../bridge.h"


#ifndef DISABLE_GENERIC_FUNCTIONS
/**
 * rbdBridgeGenericWorker
 *
 * Bridge RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Bridge RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of a Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeGenericStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}

/**
 * rbdBridgeIdenticalWorker
 *
 * Identical Bridge RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorker(void *arg)
{
    struct rbdBridgeData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int numCores;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;
    /* Retrieve last time instant to be processed by worker */
    timeLimit = data->numTimes;
    /* Retrieve number of cores in SMP system */
    numCores = data->numCores;

    /* For each time instant to be processed... */
    while (time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        rbdBridgeIdenticalStepS1d(data, time);
        /* Increment current time instant */
        time += numCores;
    }

    return NULL;
}
#endif /* DISABLE_GENERIC_FUNCTIONS */

/**
 * rbdBridgeGenericStepS1d
 *
 * Generic Bridge RBD step function
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD step.
 *  It is responsible to compute the reliability of a Bridge block with generic components
 *  given their reliabilities
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN void rbdBridgeGenericStepS1d(struct rbdBridgeData *data, unsigned int time)
{
    double s1dR1, s1dR2, s1dR3, s1dR4, s1dR5;
    double s1dTmp1, s1dTmp2;
    double s1dRes;

    /* Load reliabilities */
    s1dR1 = data->reliabilities[(0 * data->numTimes) + time];
    s1dR2 = data->reliabilities[(1 * data->numTimes) + time];
    s1dR3 = data->reliabilities[(2 * data->numTimes) + time];
    s1dR4 = data->reliabilities[(3 * data->numTimes) + time];
    s1dR5 = data->reliabilities[(4 * data->numTimes) + time];

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
    s1dTmp1 = s1dR1 + s1dR3;
    s1dTmp2 = s1dR2 + s1dR4;
    s1dTmp1 = s1dTmp1 - (s1dR1 * s1dR3);
    s1dTmp2 = s1dTmp2 - (s1dR2 * s1dR4);
    s1dRes = s1dTmp1 * s1dTmp2;
    /* At this point s1dRes vector contains VAL1 value */
    s1dTmp1 = s1dR3 * s1dR4;
    s1dTmp2 = s1dR1 * s1dR2;
    s1dTmp1 = s1dTmp1 + s1dTmp2 - (s1dTmp1 * s1dTmp2);
    /* At this point s1dTmp1 vector contains VAL2 value */
    s1dRes = s1dR5 * (s1dRes - s1dTmp1) + s1dTmp1;

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}


/**
 * rbdBridgeIdenticalStepS1d
 *
 * Identical Bridge RBD step function
 *
 * Input:
 *      struct rbdBridgeData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD step.
 *  It is responsible to compute the reliability of a Bridge block with identical components
 *  given their reliability
 *
 * Parameters:
 *      data: Bridge RBD data structure
 *      time: current time instant over which Bridge RBD shall be computed
 */
HIDDEN void rbdBridgeIdenticalStepS1d(struct rbdBridgeData *data, unsigned int time)
{
    double s1dR, s1dU;
    double s1dRes;

    /* Load reliability */
    s1dR = data->reliabilities[time];
    /* Compute unreliability */
    s1dU = 1.0 - s1dR;

    /* Compute reliability of Bridge block */
    s1dRes = s1dR * (1 + s1dU * (s1dU * (s1dU * s1dU - 2) + s1dR * (2 - s1dR * s1dR)));

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}
