/*
 *  Component: hot_standby_riscv64.c
 *  Hot Stand-by RBD management - RISC-V 64bit platform-specific implementation
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

#if defined(ARCH_RISCV64) && CPU_ENABLE_SIMD != 0
#include "rbd_internal_riscv64.h"
#include "hot_standby_riscv64.h"
#include "../hot_standby.h"


/**
 * rbdHotStandbyWorker
 *
 * Hot Stand-by RBD Worker function with RISC-V 64bit platform-specific instruction sets
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by Worker exploiting RISC-V 64bit platform-specific instruction sets.
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

    if (riscv64RvvSupported()) {
        return rbdHotStandbyWorkerRvv(data);
    }

    return rbdHotStandbyWorkerNoarch(data);
}

#endif /* defined(ARCH_RISCV64) && CPU_ENABLE_SIMD != 0 */
