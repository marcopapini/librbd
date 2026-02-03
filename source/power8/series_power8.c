/*
 *  Component: series_power8.c
 *  Series RBD management - POWER8 platform-specific implementation
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


#include "../generic/rbd_internal_generic.h"

#if defined(ARCH_POWER8) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_power8.h"
#include "series_power8.h"
#include "../series.h"


/**
 * rbdSeriesGenericWorker
 *
 * Generic Series RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Series RBD Worker exploiting POWER8 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesGenericWorker(void *arg)
{
    struct rbdSeriesData *data;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;

    return rbdSeriesGenericWorkerVsx(data);
}

/**
 * rbdSeriesIdenticalWorker
 *
 * Identical Series RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Series RBD Worker exploiting POWER8 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Series RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Series RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Series RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdSeriesIdenticalWorker(void *arg)
{
    struct rbdSeriesData *data;

    /* Retrieve Series RBD data */
    data = (struct rbdSeriesData *)arg;

    return rbdSeriesIdenticalWorkerVsx(data);
}

#endif /* defined(ARCH_POWER8) && CPU_ENABLE_SIMD != 0 */
