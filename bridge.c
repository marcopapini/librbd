/*
 *  Component: parallel.c
 *  Bridge RBD management
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020 by Marco Papini <papini.m@gmail.com>
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


#include "rbd.h"

#include "rbd_internal.h"

#include <math.h>
#include <stdlib.h>
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#include <pthread.h>
#endif  /* CPU_SMP */


struct rbdBridgeData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int batchSize;             /* Size of work batch */
    double *reliabilities;              /* Reliabilities of Bridge RBD system (matrix for generic Bridge, array for identical Bridge) */
    double *output;                     /* Array of computed reliabilities */
    unsigned char numComponents;        /* Number of components of Bridge RBD system N */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


static int rbdBridgeInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker);
static void *rbdBridgeGenericWorker(void *arg);
static void *rbdBridgeIdenticalWorker(void *arg);
static double rbdBridgeGenericStep(double r1, double r2, double r3, double r4, double r5);
static double rbdBridgeIdenticalStep(double r);



/**
 * rbdBridgeGeneric
 *
 * Compute reliability of a generic Bridge RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic Bridge RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Bridge RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Bridge RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Bridge RBD system (N)
 *      numTimes: number of time instants over which Bridge RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
int rbdBridgeGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdBridgeInternal(reliabilities, output, numComponents, numTimes, &rbdBridgeGenericWorker);
}

/**
 * rbdBridgeIdentical
 *
 * Compute reliability of an identical Bridge RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an identical Bridge RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Bridge RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Bridge RBD system
 *      numTimes: number of time instants over which Bridge RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
int rbdBridgeIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdBridgeInternal(reliabilities, output, numComponents, numTimes, &rbdBridgeIdenticalWorker);
}


/**
 * rbdBridgeInternal
 *
 * Compute reliability of a Bridge RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *      fpWorker fpWorker
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a Bridge RBD system using the
 *  provided Worker function
 *
 * Parameters:
 *      reliabilities: input reliabilities of all components at the provided time instants
 *      output: this array contains the reliabilities of Bridge RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Bridge RBD system
 *      numTimes: number of time instants over which Bridge RBD shall be computed
 *      fpWorker: function pointer to Worker used to compute reliability of Bridge RBD
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdBridgeInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdBridgeData *data;
    pthread_t *thread_id;
    unsigned int numCores;
    unsigned int idx;
    unsigned int batchSize;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdBridgeData data[1];
#endif  /* CPU_SMP */
    int res;

    /* If N is different from RBD_BRIDGE_COMPONENTS return -1 */
    if(numComponents != RBD_BRIDGE_COMPONENTS) {
        return -1;
    }

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Retrieve number of cores available in SMP system */
    numCores = getNumberOfCores();
    /* Compute batch size */
    batchSize = min(ceilDivision(numTimes, numCores), MIN_BATCH_SIZE);
    /* Compute the number of used cores given the batch size and the number of available cores */
    numCores = ceilDivision(numTimes, batchSize);

    /* Allocate Bridge RBD data array, return -1 in case of allocation failure */
    data = (struct rbdBridgeData *)malloc(sizeof(struct rbdBridgeData) * numCores);
    if(data == NULL) {
        return -1;
    }

    /* Is number of used cores greater than 1 (is SMP really needed)? */
    if(numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        thread_id = (pthread_t *)malloc(sizeof(pthread_t) * numCores);
        if(thread_id == NULL) {
            free(data);
            return -1;
        }

        /* For each available core... */
        for(idx = 0; idx < numCores; ++idx) {
            /* Prepare Bridge RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].batchSize = batchSize;
            data[idx].reliabilities = reliabilities;
            data[idx].output = output;
            data[idx].numComponents = numComponents;
            data[idx].numTimes = numTimes;

            /* Create the Bridge RBD Worker thread */
            if(pthread_create(&thread_id[idx], NULL, fpWorker, &data[idx]) < 0) {
                res = -1;
            }
        }

        /* Wait for created threads completion */
        for(idx = 0; idx < numCores; ++idx) {
            (void)pthread_join(thread_id[idx], NULL);
        }

        /* Free Thread ID array */
        free(thread_id);
    }
    else {
#endif  /* CPU_SMP */
        /* SMP not required/needed */
        /* Prepare Bridge RBD data structure */
        data[0].batchIdx = 0;
        data[0].batchSize = numTimes;
        data[0].reliabilities = reliabilities;
        data[0].output = output;
        data[0].numComponents = numComponents;
        data[0].numTimes = numTimes;

        /* Directly invoke the Bridge RBD Worker */
        (void)(*fpWorker)(&data[0]);

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Free Bridge RBD data array */
        free(data);
    }
#endif  /* CPU_SMP */

    /* If computation has been performed, ensure that output is a Reliability curve */
    if (res >= 0) {
        postProcessReliability(output, numTimes);
    }

    return res;
}

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
static void *rbdBridgeGenericWorker(void *arg)
{
    struct rbdBridgeData *data;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    double *reliabilities;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve matrix of reliabilities */
    reliabilities = &data->reliabilities[0];

    /* For each time instant to be processed... */
    while(time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        reliability = rbdBridgeGenericStep(reliabilities[(0 * data->numTimes) + time],
                                           reliabilities[(1 * data->numTimes) + time],
                                           reliabilities[(2 * data->numTimes) + time],
                                           reliabilities[(3 * data->numTimes) + time],
                                           reliabilities[(4 * data->numTimes) + time]);

        /* Cap computed reliability to accepted bounds [0, 1] */
        if(isnan(reliability) != 0) {
            reliability = 0.0;
        }
        if(reliability > 1.0) {
            reliability = 1.0;
        }
        if(reliability < 0.0) {
            reliability = 0.0;
        }

        /* Set computed reliability into output array */
        data->output[time++] = reliability;
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
static void *rbdBridgeIdenticalWorker(void *arg)
{
    struct rbdBridgeData *data;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    double *reliabilities;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve array of reliabilities */
    reliabilities = &data->reliabilities[0];

    /* For each time instant to be processed... */
    while(time < timeLimit) {
        /* Compute reliability of Bridge RBD at current time instant */
        reliability = rbdBridgeIdenticalStep(reliabilities[time]);

        /* Cap computed reliability to accepted bounds [0, 1] */
        if(isnan(reliability) != 0) {
            reliability = 0.0;
        }
        if(reliability > 1.0) {
            reliability = 1.0;
        }
        if(reliability < 0.0) {
            reliability = 0.0;
        }

        /* Set computed reliability into output array */
        data->output[time++] = reliability;
    }

    return NULL;
}

/**
 * rbdBridgeGenericStep
 *
 * Generic Bridge RBD step function
 *
 * Input:
 *      double r1
 *      double r2
 *      double r3
 *      double r4
 *      double r5
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
 *      r1: reliability of first Bridge block component
 *      r2: reliability of second Bridge block component
 *      r3: reliability of third Bridge block component
 *      r4: reliability of fourth Bridge block component
 *      r5: reliability of fifth Bridge block component
 *
 * Return (double):
 *  Reliability of Bridge block
 */
static double rbdBridgeGenericStep(double r1, double r2, double r3, double r4, double r5)
{
    double f1, f2, f3, f4, f5;
    double res;

    /* Compute unreliability of each component */
    f1 = 1.0 - r1;
    f2 = 1.0 - r2;
    f3 = 1.0 - r3;
    f4 = 1.0 - r4;
    f5 = 1.0 - r5;

    /* Compute reliability of Bridge block */
    res = r5 * (1 - f1 * f3) * (1 - f2 * f4) + f5 * (1 - (1 - r1 * r3) * (1 - r2 * r4));

    return res;
}

/**
 * rbdBridgeIdenticalStep
 *
 * Identical Bridge RBD step function
 *
 * Input:
 *      double r
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
 *      r: reliability of Bridge block components
 *
 * Return (double):
 *  Reliability of Bridge block
 */
static double rbdBridgeIdenticalStep(double r)
{
    double f;
    double res;

    /* Compute unreliability */
    f = 1.0 - r;

    /* Compute reliability of Bridge block */
    res = r * (1 + f * (f * (f * f - 2) + r * (2 - r * r)));

    return res;
}

