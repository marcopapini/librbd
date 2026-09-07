/*
 *  Component: cold_standby_amd64.c
 *  Cold Stand-by RBD management - amd64 platform-specific implementation
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

#if defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_amd64.h"
#include "cold_standby_amd64.h"
#include "../x86/cold_standby_x86.h"
#include "../cold_standby.h"


/**
 * rbdColdStandbyWorker
 *
 * Cold Stand-by RBD Worker function with amd64 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD Worker exploiting amd64 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a Cold Stand-by RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a Cold Stand-by RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Cold Stand-by RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdColdStandbyWorker(void *arg)
{
    struct rbdColdStandbyData *data;

    /* Retrieve Cold Stand-by RBD data */
    data = (struct rbdColdStandbyData *)arg;

    if (amd64Avx512fSupported()) {
        return rbdColdStandbyWorkerAvx512f(data);
    }

    if (amd64Fma3Supported()) {
        return rbdColdStandbyWorkerFma3(data);
    }

    if (amd64AvxSupported()) {
        return rbdColdStandbyWorkerAvx(data);
    }

    if (amd64Sse2Supported()) {
        return rbdColdStandbyWorkerSse2(data);
    }

    return rbdColdStandbyWorkerNoarch(data);
}

#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */
