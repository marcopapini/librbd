/*
 *  Component: processor_riscv64.c
 *  Retrieval of CPU-related info - RISC-V 64bit platform-specific implementation
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

#include "../os/os.h"

#if defined(OS_LINUX)
#include <signal.h>
#include <setjmp.h>
#endif /* defined(OS_LINUX) */


struct riscv64Cpu
{
    unsigned int rvvSupported;      /* RISC-V 64bit RVV instruction set supported */
};


static struct riscv64Cpu riscv64Cpu;
#if defined(OS_LINUX)
static sigjmp_buf jmpbuf;
#endif /* defined(OS_LINUX) */


#if defined(OS_LINUX)
static void sigill_handler(int signo);
#endif /* defined(OS_LINUX) */


/**
 * riscv64RvvSupported
 *
 * RVV instruction set supported by the RISC-V 64bit system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of RVV instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if RVV instruction set is available, 0 otherwise
 */
HIDDEN unsigned int riscv64RvvSupported(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return RISC-V 64bit RVV instruction set supported by the system */
    return riscv64Cpu.rvvSupported;
}

/**
 * retrieveRiscv64CpuInfo
 *
 * Retrieve RISC-V 64bit-specific CPU info (supported instruction sets)
 *
 * Input:
 *      unsigned int numCores
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - If RVV instruction set is supported
 *
 * Parameters:
 *      numCores: number of cores in SMP system
 *
 * Return (unsigned int):
 *      Number of cores in SMP system
 */
HIDDEN unsigned int retrieveRiscv64CpuInfo(unsigned int numCores)
{
#if defined(OS_LINUX)
    struct sigaction sa_old, sa_new;
    unsigned long int vl;
#endif /* defined(OS_LINUX) */

    /**
     * Default processor info:
     * - no RVV
     */
    riscv64Cpu.rvvSupported = 0;

#if defined(OS_LINUX)
    /* Initialize new sigaction */
    sa_new.sa_handler = sigill_handler;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;

    /* Update sigaction for SIGILL */
    sigaction(SIGILL, &sa_new, &sa_old);

    if (sigsetjmp(jmpbuf, 1) == 0) {
        /* Try a harmless RVV instruction */
        vl = __riscv_vsetvl_e8m1(1);
        (void)vl;

        /* Restore sigaction for SIGILL */
        sigaction(SIGILL, &sa_old, NULL);

        /* RVV is supported */
        riscv64Cpu.rvvSupported = 1;
    }

    /* Restore sigaction for SIGILL */
    sigaction(SIGILL, &sa_old, NULL);
#endif /* defined(OS_LINUX) */

    /* Retrieve CPU sets for CPU affinity on RISC-V 64bit systems supporting RVV */
    if (riscv64Cpu.rvvSupported) {
        if (retrieveRiscv64CompatibleCpusetRvv(numCores) < 0) {
            riscv64Cpu.rvvSupported = 0;
        }
    }

    return numCores;
}


#if defined(OS_LINUX)
/**
 * sigill_handler
 *
 * Custom handler for SIGILL signal
 *
 * Input:
 *      int signo
 *
 * Output:
 *      None
 *
 * Description:
 *  This custom signal SIGILL handler is used during test for RVV support only
 *
 * Parameters:
 *      signo: signal number
 *
 * Return:
 *      None
 */
static void sigill_handler(int signo)
{
    /* RVV test KO - Perform a signal long jump to restore context */
    siglongjmp(jmpbuf, 1);
}
#endif /* defined(OS_LINUX) */

#endif /* defined(ARCH_RISCV64) && CPU_ENABLE_SIMD != 0 */
