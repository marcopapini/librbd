/*
 *  Component: bridge_aarch64.c
 *  Bridge RBD management - AArch64 platform-specific implementation
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

#if defined(ARCH_AARCH64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_aarch64.h"
#include "bridge_aarch64.h"
#include "../bridge.h"


/**
 * rbdBridgeGenericWorker
 *
 * Generic Bridge RBD Worker function with AArch64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic Bridge RBD Worker exploiting AArch64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeGenericWorker(void *arg)
{
    struct rbdBridgeData *data;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;

    return rbdBridgeGenericWorkerNeon(data);
}

/**
 * rbdBridgeIdenticalWorker
 *
 * Identical Bridge RBD Worker function with AArch64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical Bridge RBD Worker exploiting AArch64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical Bridge RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Bridge RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Bridge RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdBridgeIdenticalWorker(void *arg)
{
    struct rbdBridgeData *data;

    /* Retrieve Bridge RBD data */
    data = (struct rbdBridgeData *)arg;

    return rbdBridgeIdenticalWorkerNeon(data);
}

#endif /* defined(ARCH_AARCH64) && CPU_ENABLE_SIMD != 0 */
