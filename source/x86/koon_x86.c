/*
 *  Component: koon_x86.c
 *  KooN (K-out-of-N) RBD management - x86 platform-specific implementation
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

#if defined(ARCH_X86) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_x86.h"
#include "koon_x86.h"
#include "../koon.h"


/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker x86 platform-specific instruction sets.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a fill KooN data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNFillData *)arg;

    if (x86Sse2Supported()) {
        return rbdKooNFillWorkerSse2(data);
    }

    return rbdKooNFillWorkerNoarch(data);
}

/**
 * rbdKooNGenericShannonWorker
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition with
 * x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition with
 *  x86 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN for Shannon Decomposition RBD data.
 *                      It is provided as a void * to be compliant with the SMP computation of the
 *                      Generic KooN for Shannon Decomposition RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericShannonWorker(void *arg)
{
    struct rbdKooNGenericShannonData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericShannonData *)arg;

    if (x86Sse2Supported()) {
        return rbdKooNGenericShannonWorkerSse2(data);
    }

    return rbdKooNGenericShannonWorkerNoarch(data);
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function with x86 platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting x86 platform-specific instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a identical KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;

    if (x86Sse2Supported()) {
        return rbdKooNIdenticalWorkerSse2(data);
    }

    return rbdKooNIdenticalWorkerNoarch(data);
}


#endif /* defined(ARCH_X86) && CPU_ENABLE_SIMD != 0 */
