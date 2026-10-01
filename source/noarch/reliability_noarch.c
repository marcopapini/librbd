/*
 *  Component: reliability_noarch.c
 *  Reliability RBD management - Platform-independent implementation
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


#include "../generic/rbd_internal_generic.h"

#include "../reliability.h"


#if defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0
/**
 * rbdUnreliabilityWorker
 *
 * Unreliability RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker.
 *  It is responsible to compute the unreliability curve given the corresponding reliability curve
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to an Unreliability RBD data provided as a void *
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdUnreliabilityWorker(void *arg)
{
    struct rbdUnreliabilityData *data;

    /* Retrieve Unreliability RBD data */
    data = (struct rbdUnreliabilityData *)arg;

    return rbdUnreliabilityWorkerNoarch(data);
}

/**
 * rbdReliabilitySumWorker
 *
 * Reliability Sum RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Reliability RBD data provided as a void *
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdReliabilitySumWorker(void *arg)
{
    struct rbdReliabilityData *data;

    /* Retrieve Reliability RBD data */
    data = (struct rbdReliabilityData *)arg;

    return rbdReliabilitySumWorkerNoarch(data);
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdUnreliabilityWorkerNoarch
 *
 * Unreliability RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdUnreliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the unreliability curve given its corresponding reliability curve
 *
 * Parameters:
 *      data: Unreliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdUnreliabilityWorkerNoarch(struct rbdUnreliabilityData *data)
{
    unsigned int time;
    double s1dRes;

    /* For each time instant to be processed... */
    for (time = 0; time < data->numTimes; ++time) {
        /* Compute unreliability at current time instant */
        s1dRes = 1.0 - data->reliability[time];
        data->unreliability[time] = capReliabilityS1d(s1dRes);
    }

    return NULL;
}

/**
 * rbdReliabilitySumWorkerNoarch
 *
 * Reliability Sum RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdReliabilityData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the sum of the given reliability curves
 *
 * Parameters:
 *      data: Reliability RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdReliabilitySumWorkerNoarch(struct rbdReliabilityData *data)
{
    unsigned int time;
    double s1dRes;

    /* For each time instant to be processed... */
    for (time = 0; time < data->numTimes; ++time) {
        /* Compute reliability sum at current time instant */
        s1dRes = data->r1[time] + data->r2[time];
        data->output[time] = capReliabilityS1d(s1dRes);
    }

    return NULL;
}
