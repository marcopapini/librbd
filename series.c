/*
 *  Component: series.c
 *  Series RBD management
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


#include "rbd_internal.h"

#include "series.h"


static int rbdSeriesInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker);


/**
 * rbdSeriesGeneric
 *
 * Compute reliability of a generic Series RBD system
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
 *  This function computes the reliabilities over time of a generic Series RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Series RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Series RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Series RBD system (N)
 *      numTimes: number of time instants over which Series RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdSeriesGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdSeriesInternal(reliabilities, output, numComponents, numTimes, &rbdSeriesGenericWorker);
}

/**
 * rbdSeriesIdentical
 *
 * Compute reliability of an identical Series RBD system
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
 *  This function computes the reliabilities over time of an identical Series RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Series RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Series RBD system
 *      numTimes: number of time instants over which Series RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdSeriesIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
{
    return rbdSeriesInternal(reliabilities, output, numComponents, numTimes, &rbdSeriesIdenticalWorker);
}


/**
 * rbdSeriesInternal
 *
 * Compute reliability of a Series RBD system
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
 *  This function computes the reliabilities over time of a Series RBD system using the
 *  provided Worker function
 *
 * Parameters:
 *      reliabilities: input reliabilities of all components at the provided time instants
 *      output: this array contains the reliabilities of Series RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Series RBD system
 *      numTimes: number of time instants over which Series RBD shall be computed
 *      fpWorker: function pointer to Worker used to compute reliability of Series RBD
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdSeriesInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdSeriesData *data;
    pthread_t *thread_id;
    unsigned int numCores;
    unsigned int idx;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdSeriesData data[1];
#endif /* CPU_SMP */
    int res;

    /* If N is equal to 0 return -1 */
    if (numComponents == 0) {
        return -1;
    }

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);

    /* Allocate Series RBD data array, return -1 in case of allocation failure */
    data = (struct rbdSeriesData *)malloc(sizeof(struct rbdSeriesData) * numCores);
    if (data == NULL) {
        return -1;
    }

    /* Is number of used cores greater than 1 (is SMP really needed)? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        thread_id = (pthread_t *)malloc(sizeof(pthread_t) * (numCores - 1));
        if (thread_id == NULL) {
            free(data);
            return -1;
        }

        /* For each available core... */
        for (idx = 0; idx < (numCores - 1); ++idx) {
            /* Prepare Series RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].numCores = numCores;
            data[idx].reliabilities = reliabilities;
            data[idx].output = output;
            data[idx].numComponents = numComponents;
            data[idx].numTimes = numTimes;

            /* Create the Series RBD Worker thread */
            if (pthread_create(&thread_id[idx], NULL, fpWorker, &data[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare Series RBD data structure */
        data[idx].batchIdx = idx;
        data[idx].numCores = numCores;
        data[idx].reliabilities = reliabilities;
        data[idx].output = output;
        data[idx].numComponents = numComponents;
        data[idx].numTimes = numTimes;

        /* Directly invoke the Series RBD Worker */
        (void)(*fpWorker)(&data[idx]);

        /* Wait for created threads completion */
        for (idx = 0; idx < (numCores - 1); idx++) {
            (void)pthread_join(thread_id[idx], NULL);
        }

        /* Free Thread ID array */
        free(thread_id);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare Series RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = 1;
        data[0].reliabilities = reliabilities;
        data[0].output = output;
        data[0].numComponents = numComponents;
        data[0].numTimes = numTimes;

        /* Directly invoke the Series RBD Worker */
        (void)(*fpWorker)(&data[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Free Series RBD data array */
        free(data);
    }
#endif /* CPU_SMP */

    return res;
}
