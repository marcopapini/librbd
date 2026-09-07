/*
 *  Component: hot_standby.c
 *  Hot Stand-by RBD management
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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


#include "generic/rbd_internal_generic.h"

#include "hot_standby.h"


/**
 * rbdHotStandby
 *
 * Compute reliability of a Hot Stand-by RBD system
 *
 * Input:
 *      double *primaryReliability
 *      double *standbyReliability
 *      double pSwitch
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a Hot Stand-by RBD system
 *
 * Parameters:
 *      primaryReliability: this array contains the input reliability of the primary
 *                      component at the provided time instants
 *      standbyReliability: this array contains the input reliability of the stand-by
 *                      component at the provided time instants
 *      pSwitch: probability that the switch is correctly performed
 *      output: this array contains the reliabilities of Hot Stand-by RBD system computed at
 *                      the provided time instants
 *      numTimes: number of time instants over which Hot Stand-by RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdHotStandby(double *primaryReliability, double *standbyReliability, double pSwitch, double *output, unsigned int numTimes)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdHotStandbyData *data;
    void *threadHandles;
    unsigned int numCores;
    unsigned int idx;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdHotStandbyData data[1];
#endif /* CPU_SMP */
    int res;

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);

    /* Allocate Hot Stand-by RBD data array, return -1 in case of allocation failure */
    data = (struct rbdHotStandbyData *)malloc(sizeof(struct rbdHotStandbyData) * numCores);
    if (data == NULL) {
        return -1;
    }

    /* Is number of used cores greater than 1 (is SMP really needed)? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(data);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Prepare Hot Stand-by RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].numCores = numCores;
            data[idx].primaryReliability = primaryReliability;
            data[idx].standbyReliability = standbyReliability;
            data[idx].pSwitch = pSwitch;
            data[idx].output = output;
            data[idx].numTimes = numTimes;

            /* Create the Hot Stand-by RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdHotStandbyWorker, &data[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare Hot Stand-by RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = numCores;
        data[0].primaryReliability = primaryReliability;
        data[0].standbyReliability = standbyReliability;
        data[0].pSwitch = pSwitch;
        data[0].output = output;
        data[0].numTimes = numTimes;

        /* Directly invoke the Hot Stand-by RBD Worker */
        (void)rbdHotStandbyWorker(&data[0]);

        /* Wait for created threads completion */
        for (idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare Hot Stand-by RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = 1;
        data[0].primaryReliability = primaryReliability;
        data[0].standbyReliability = standbyReliability;
        data[0].pSwitch = pSwitch;
        data[0].output = output;
        data[0].numTimes = numTimes;

        /* Directly invoke the Hot Stand-by RBD Worker */
        (void)rbdHotStandbyWorker(&data[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free Parallel RBD data array */
    free(data);
#endif /* CPU_SMP */

    return res;
}
