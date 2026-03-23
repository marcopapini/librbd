/*
 *  Component: processor_riscv64_rvv.c
 *  Retrieval of CPU-related info - RISC-V 64bit RVV-specific implementation
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


#include "../../os/os.h"

#if defined(OS_LINUX)
#define _GNU_SOURCE
#endif /* defined(OS_LINUX) */
#include "../../generic/rbd_internal_generic.h"

#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#if defined(OS_LINUX)
#include <sched.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>


struct riscv64CpusetRvv
{
    uint8_t cpusetRetrieved;            /* RISC-V 64bit affinity for core retrieved flag */
    cpu_set_t compatibleAffinity;       /* RISC-V 64bit affinity associated with core */
};


struct riscv64CpusetRvv riscv64CpusetRvv[MAX_NUM_THREADS];
#endif /* defined(OS_LINUX) */


#if defined(OS_LINUX)
/**
 * getVectorLength
 *
 * Retrieve supported Vector Length
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the Vector Length, in bytes, supported by the current core
 *
 * Parameters:
 *      None && (CPU_SMP != 0)
 *
 * Return (uintptr_t):
 *  Supported Vector Length in bytes
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") uintptr_t getVectorLength()
{
    uintptr_t vl = 0;
    /* Retrieve VLENB value from CSR */
    asm volatile("csrr %0, 0xc22" : "=r"(vl));
    return vl;
}
#endif /* defined(OS_LINUX) */


/**
 * setRiscv64ThreadAffinityRvv
 *
 * Set the CPU affinity for the current thread
 *
 * Input:
 *      unsigned int coreId
 *
 * Output:
 *      None
 *
 * Description:
 *  This function sets the CPU affinity for the current thread to the set of cores compatible with
 *  the requested one
 *
 * Parameters:
 *      coreId: requested core identifier
 *
 * Return:
 *      None
 */
HIDDEN void setRiscv64ThreadAffinityRvv(unsigned int coreId)
{
#if defined(OS_LINUX)
    sched_setaffinity(0, sizeof(riscv64CpusetRvv[coreId].compatibleAffinity), &riscv64CpusetRvv[coreId].compatibleAffinity);
#endif /* defined(OS_LINUX) */
}

/**
 * retrieveRiscv64CompatibleCpusetRvv
 *
 * Retrieve all the CPU sets associated with the pool of available cores
 *
 * Input:
 *      unsigned int numCores
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves, for each core in the pool of available cores, the CPU set
 *  of compatible cores
 *
 * Parameters:
 *      numCores: number of cores belonging to the core pool
 *
 * Return (int):
 *      0 in case of success, -1 otherwise
 */
HIDDEN FUNCTION_TARGET("arch=+v") int retrieveRiscv64CompatibleCpusetRvv(unsigned int numCores)
{
#if defined(OS_LINUX)
    int coreIdx;
    int idx;
    cpu_set_t originalAffinity;
    cpu_set_t compatibleAffinity;
    cpu_set_t currAffinity;
    cpu_set_t otherAffinity;
    uintptr_t currVectorLen;
    uintptr_t otherVectorLen;

    /* Clear all CPU sets */
    for (coreIdx = 0; coreIdx < numCores; ++coreIdx) {
        CPU_ZERO(&riscv64CpusetRvv[coreIdx].compatibleAffinity);
    }

    /* Retrieve current affinity */
    if (sched_getaffinity(0, sizeof(cpu_set_t), &originalAffinity) != 0) {
        return -1;
    }

    /* For each core... */
    for (coreIdx = 0; coreIdx < numCores; ++coreIdx) {
        /* Skip already processed cores */
        if (riscv64CpusetRvv[coreIdx].cpusetRetrieved) {
            continue;
        }
        /* Clear cpuset of current group */
        CPU_ZERO(&compatibleAffinity);

        /* Try to switch to current core */
        CPU_ZERO(&currAffinity);
        CPU_SET(coreIdx, &currAffinity);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &currAffinity) != 0) {
            return -1;
        }

        /* Retrieve Vector Length of current core */
        currVectorLen = getVectorLength();

        /* Current core belongs to this cpuset group */
        CPU_SET(coreIdx, &compatibleAffinity);

        /* For every other core... */
        for (idx = coreIdx + 1; idx < numCores; ++idx) {
            /* Skip already processed cores */
            if (riscv64CpusetRvv[idx].cpusetRetrieved) {
                continue;
            }

            /* Try to switch to other core */
            CPU_ZERO(&otherAffinity);
            CPU_SET(idx, &otherAffinity);
            if (sched_setaffinity(0, sizeof(cpu_set_t), &otherAffinity) != 0) {
                return -1;
            }
            /* Retrieve Vector Length of test core */
            otherVectorLen = getVectorLength();
            /* Current and other core belong to the same cpuset if they have the same Vector Length */
            if (otherVectorLen == currVectorLen) {
                CPU_SET(idx, &compatibleAffinity);
                riscv64CpusetRvv[idx].cpusetRetrieved = 1;
            }
        }

        /* Copy the cpuset to set of compatible cores */
        for (coreIdx = 0; coreIdx < numCores; ++coreIdx) {
            if (CPU_ISSET(coreIdx, &compatibleAffinity)) {
                riscv64CpusetRvv[coreIdx].compatibleAffinity = compatibleAffinity;
            }
        }

        riscv64CpusetRvv[coreIdx].cpusetRetrieved = 1;
    }

    if (sched_setaffinity(0, sizeof(cpu_set_t), &originalAffinity) != 0) {
        return -1;
    }

    for (coreIdx = 0; coreIdx < numCores; ++coreIdx) {
        if (riscv64CpusetRvv[coreIdx].cpusetRetrieved == 0) {
            return -1;
        }
    }
#endif /* defined(OS_LINUX) */

    return 0;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
