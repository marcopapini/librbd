/*
 *  Component: cold_standby.c
 *  Cold Stand-by RBD management
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


#include "generic/rbd_internal_generic.h"

#include "cold_standby.h"

#include "failure_density.h"


/**
 * rbdColdStandby
 *
 * Compute reliability of a Cold Stand-by RBD system
 *
 * Input:
 *      double *primaryReliability
 *      double *standbyReliability
 *      double pSwitch
 *      unsigned int numTimes
 *      double deltaT
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a Cold Stand-by RBD system
 *
 * Parameters:
 *      primaryReliability: this array contains the input reliability of the primary
 *                      component at the provided time instants
 *      standbyReliability: this array contains the input reliability of the stand-by
 *                      component at the provided time instants
 *      pSwitch: probability that the switch is correctly performed
 *      output: this array contains the reliabilities of Cold Stand-by RBD system computed at
 *                      the provided time instants
 *      numTimes: number of time instants over which Cold Stand-by RBD shall be computed (T)
 *      deltaT: time difference between two consecutive time instants
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdColdStandby(double *primaryReliability, double *standbyReliability, double pSwitch, double *output, unsigned int numTimes, double deltaT)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdColdStandbyData *data;
    void *threadHandles;
    unsigned int numCores;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdColdStandbyData data[1];
#endif /* CPU_SMP */
    unsigned int idx;
    int res;
    double *primaryFailureDensity;
    double *standbyReliabilityRev;

    res = 0;

    /* Check validity of time difference between two consecutive time instants */
    if (deltaT <= 0.0) {
        return -1;
    }

    /* Allocate array for Primary Failure Density, return -1 in case of allocation failure */
    primaryFailureDensity = (double *)malloc(sizeof(double) * numTimes);
    if (primaryFailureDensity == NULL) {
        return -1;
    }

    /* Allocate array for Reversed Stand-by Reliability, return -1 in case of allocation failure */
    standbyReliabilityRev = (double *)malloc(sizeof(double) * numTimes);
    if (standbyReliabilityRev == NULL) {
        free(primaryFailureDensity);
        return -1;
    }

    /* Compute Failure Density of the Primary component */
    rbdFailureDensityWorker(&primaryReliability[0], &primaryFailureDensity[0], numTimes, deltaT);

    /* Compute Reversed Stand-by Reliability */
    for (idx = 0; idx < numTimes; ++idx) {
        standbyReliabilityRev[numTimes - idx - 1] = standbyReliability[idx];
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);

    /* Allocate Cold Stand-by RBD data array, return -1 in case of allocation failure */
    data = (struct rbdColdStandbyData *)malloc(sizeof(struct rbdColdStandbyData) * numCores);
    if (data == NULL) {
        free(primaryFailureDensity);
        free(standbyReliabilityRev);
        return -1;
    }

    /* Is number of used cores greater than 1 (is SMP really needed)? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(primaryFailureDensity);
            free(standbyReliabilityRev);
            free(data);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Prepare Cold Stand-by RBD data structure */
            data[idx].batchIdx = idx;
            data[idx].numCores = numCores;
            data[idx].primaryReliability = primaryReliability;
            data[idx].primaryFailureDensity = primaryFailureDensity;
            data[idx].standbyReliabilityRev = standbyReliabilityRev;
            data[idx].pSwitch = pSwitch;
            data[idx].output = output;
            data[idx].numTimes = numTimes;
            data[idx].deltaT = deltaT;

            /* Create the Cold Stand-by RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdColdStandbyWorker, &data[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare Cold Stand-by RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = numCores;
        data[0].primaryReliability = primaryReliability;
        data[0].primaryFailureDensity = primaryFailureDensity;
        data[0].standbyReliabilityRev = standbyReliabilityRev;
        data[0].pSwitch = pSwitch;
        data[0].output = output;
        data[0].numTimes = numTimes;
        data[0].deltaT = deltaT;

        /* Directly invoke the Cold Stand-by RBD Worker */
        (void)rbdColdStandbyWorker(&data[0]);

        /* Wait for created threads completion */
        for (idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare Cold Stand-by RBD data structure */
        data[0].batchIdx = 0;
        data[0].numCores = 1;
        data[0].primaryReliability = primaryReliability;
        data[0].primaryFailureDensity = primaryFailureDensity;
        data[0].standbyReliabilityRev = standbyReliabilityRev;
        data[0].pSwitch = pSwitch;
        data[0].output = output;
        data[0].numTimes = numTimes;
        data[0].deltaT = deltaT;

        /* Directly invoke the Cold Stand-by RBD Worker */
        (void)rbdColdStandbyWorker(&data[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free Parallel RBD data array */
    free(data);
#endif /* CPU_SMP */

    /* Free Primary Failure Density and Reversed Stand-by Reliability arrays */
    free(primaryFailureDensity);
    free(standbyReliabilityRev);

    return res;
}
