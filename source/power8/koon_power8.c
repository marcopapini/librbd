/*
 *  Component: koon_power8.c
 *  KooN (K-out-of-N) RBD management - POWER8 platform-specific implementation
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
#include "koon_power8.h"
#include "../koon.h"


/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker POWER8 platform-specific instruction sets.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a fill KooN data. It is provided as a void *
 *                      to be compliant with the SMP of the Fill KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNFillData *)arg;

    return rbdKooNFillWorkerVsx(data);
}

/**
 * rbdKooNGenericWorker
 *
 * Generic KooN RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting POWER8 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Generic KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorker(void *arg)
{
    struct rbdKooNGenericData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;

    return rbdKooNGenericWorkerVsx(data);
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting POWER8 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to an identical KooN RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Identical KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;

    return rbdKooNIdenticalWorkerVsx(data);
}

#endif /* defined(ARCH_POWER8) && CPU_ENABLE_SIMD != 0 */
