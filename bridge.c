/*
 *  Component: bridge.c
 *  Bridge RBD management
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

#include "bridge.h"


static int rbdBridgeInternal(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, fpWorker fpWorker);


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
EXTERN int rbdBridgeGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
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
EXTERN int rbdBridgeIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes)
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
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdBridgeData data[1];
#endif /* CPU_SMP */
    int res;

    /* If N is different from RBD_BRIDGE_COMPONENTS return -1 */
    if (numComponents != RBD_BRIDGE_COMPONENTS) {
        return -1;
    }

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);

    /* Allocate Bridge RBD data array, return -1 in case of allocation failure */
    data = (struct rbdBridgeData *)malloc(sizeof(struct rbdBridgeData) * numCores);
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
            /* Prepare Bridge RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].numCores = numCores;
            data[idx].reliabilities = reliabilities;
            data[idx].output = output;
            data[idx].numComponents = numComponents;
            data[idx].numTimes = numTimes;

            /* Create the Bridge RBD Worker thread */
            if (pthread_create(&thread_id[idx], NULL, fpWorker, &data[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare Bridge RBD data structure */
        data[idx].batchIdx = idx;
        data[idx].numCores = numCores;
        data[idx].reliabilities = reliabilities;
        data[idx].output = output;
        data[idx].numComponents = numComponents;
        data[idx].numTimes = numTimes;

        /* Directly invoke the Bridge RBD Worker */
        (void)(*fpWorker)(&data[idx]);

        /* Wait for created threads completion */
        for (idx = 0; idx < (numCores - 1); ++idx) {
            (void)pthread_join(thread_id[idx], NULL);
        }
        /* Free Thread ID array */
        free(thread_id);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare Bridge RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = 1;
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
#endif /* CPU_SMP */

    return res;
}
