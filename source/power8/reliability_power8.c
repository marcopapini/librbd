/*
 *  Component: reliability_power8.c
 *  Reliability RBD management - POWER8 platform-specific implementation
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

#if defined(ARCH_POWER8) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_power8.h"
#include "reliability_power8.h"
#include "../reliability.h"


/**
 * rbdUnreliabilityWorker
 *
 * Unreliability RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Unreliability RBD Worker exploiting POWER8 platform-specific instruction sets.
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

    return rbdUnreliabilityWorkerVsx(data);
}

/**
 * rbdReliabilitySumWorker
 *
 * Reliability Sum RBD Worker function with POWER8 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Reliability Sum RBD Worker exploiting POWER8 platform-specific instruction sets.
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

    return rbdReliabilitySumWorkerVsx(data);
}

#endif /* defined(ARCH_POWER8) && CPU_ENABLE_SIMD != 0 */
