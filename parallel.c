/*
 *  Component: parallel.c
 *  Parallel RBD management
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


struct rbdParallelData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int batchSize;             /* Size of work batch */
    double *reliabilities;              /* Reliabilities of Parallel RBD system (matrix for generic Series, array for identical Series) */
    double *output;                     /* Array of computed reliabilities */
    unsigned char numComponents;        /* Number of components of Parallel RBD system N */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


static int rbdParallelInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker);
static void *rbdParallelGenericWorker(void *arg);
static void *rbdParallelIdenticalWorker(void *arg);


/**
 * rbdParallelGeneric
 *
 * Compute reliability of a generic Parallel RBD system
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
 *  This function computes the reliabilities over time of a generic Parallel RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a TxN one, where N is the number of components of Parallel RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Parallel RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Parallel RBD system (N)
 *      numTimes: number of time instants over which Parallel RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
int rbdParallelGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdParallelInternal(reliabilities, output, numComponents, numTimes, &rbdParallelGenericWorker);
}

/**
 * rbdParallelIdentical
 *
 * Compute reliability of an identical Parallel RBD system
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
 *  This function computes the reliabilities over time of an identical Parallel RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Parallel RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Parallel RBD system
 *      numTimes: number of time instants over which Parallel RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
int rbdParallelIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdParallelInternal(reliabilities, output, numComponents, numTimes, &rbdParallelIdenticalWorker);
}


/**
 * rbdParallelInternal
 *
 * Compute reliability of a Parallel RBD system
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
 *  This function computes the reliabilities over time of a Parallel RBD system using the
 *  provided Worker function
 *
 * Parameters:
 *      reliabilities: input reliabilities of all components at the provided time instants
 *      output: this array contains the reliabilities of Parallel RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Parallel RBD system
 *      numTimes: number of time instants over which Parallel RBD shall be computed
 *      fpWorker: function pointer to Worker used to compute reliability of Parallel RBD
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdParallelInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdParallelData *data;
    pthread_t *thread_id;
    unsigned int numCores;
    unsigned int idx;
    unsigned int batchSize;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdParallelData data[1];
#endif  /* CPU_SMP */
    int res;

    /* If N is equal to 0 return -1 */
    if(numComponents == 0) {
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

    /* Allocate Parallel RBD data array, return -1 in case of allocation failure */
    data = (struct rbdParallelData *)malloc(sizeof(struct rbdParallelData) * numCores);
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
            /* Prepare Parallel RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].batchSize = batchSize;
            data[idx].reliabilities = reliabilities;
            data[idx].output = output;
            data[idx].numComponents = numComponents;
            data[idx].numTimes = numTimes;

            /* Create the Parallel RBD Worker thread */
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
        /* Prepare Parallel RBD data structure */
        data[0].batchIdx = 0;
        data[0].batchSize = numTimes;
        data[0].reliabilities = reliabilities;
        data[0].output = output;
        data[0].numComponents = numComponents;
        data[0].numTimes = numTimes;

        /* Directly invoke the Parallel RBD Worker */
        (void)(*fpWorker)(&data[0]);

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Free Parallel RBD data array */
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
static void *rbdParallelGenericWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned char component;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    double *reliabilities;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;

    /* For each time instant to be processed... */
    while(time < timeLimit) {
        /* Retrieve array of reliabilities over which worker shall work */
        reliabilities = &data->reliabilities[time * data->numComponents];

        /* Compute reliability of Parallel RBD at current time instant */
        reliability = 1.0;
        for(component = 0; component < data->numComponents; ++component) {
            reliability *= (1.0 - reliabilities[component]);
        }

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
        data->output[time++] = (1.0 - reliability);
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
 *                      SMP computation of Series RBD
 *
 * Return (void *):
 *  NULL
 */
static void *rbdParallelIdenticalWorker(void *arg)
{
    struct rbdParallelData *data;
    unsigned char component;
    double reliability;
    double unreliability;
    unsigned int time;
    unsigned int timeLimit;
    double *reliabilities;

    /* Retrieve Parallel RBD data */
    data = (struct rbdParallelData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve array of reliabilities over which worker shall work */
    reliabilities = &data->reliabilities[0];

    /* For each time instant to be processed... */
    while(time < timeLimit) {
        /* Compute unreliability for component at current time instant */
        unreliability = (1.0 - reliabilities[time]);

        /* Compute reliability of Parallel RBD at current time instant */
        reliability = 1.0;
        for(component = data->numComponents; component > 0; --component) {
            reliability *= unreliability;
        }

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
        data->output[time++] = (1.0 - reliability);
    }

    return NULL;
}
