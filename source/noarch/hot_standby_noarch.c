/*
 *  Component: hot_standby_noarch.c
 *  Hot Stand-by RBD management - Platform-independent implementation
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

#include "../hot_standby.h"
#include "../integral.h"


#if defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0
/**
 * rbdHotStandbyWorker
 *
 * Hot Stand-by RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Hot Stand-by RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Hot Stand-by RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorker(void *arg)
{
    struct rbdHotStandbyData *data;

    /* Retrieve Hot Stand-by RBD data */
    data = (struct rbdHotStandbyData *)arg;

    return rbdHotStandbyWorkerNoarch(data);
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdHotStandbyWorkerNoarch
 *
 * Hot Stand-by RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerNoarch(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdHotStandbyStepS1d
 *
 * Hot Stand-by RBD step function
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN void rbdHotStandbyStepS1d(struct rbdHotStandbyData *data, unsigned int time)
{
    double s1dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    s1dRes = data->primaryReliability[time] + data->standbyReliability[time] *
                                              rbdIntegralHotStandbyS1d(data, time);

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}
