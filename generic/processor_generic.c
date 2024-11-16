/*
 *  Component: processor_generic.c
 *  Retrieval of CPU-related info - Generic implementation
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


#include "rbd_internal_generic.h"


#if defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0
#include "../amd64/rbd_internal_amd64.h"
#endif /* defined(ARCH_AMD64) && CPU_ENABLE_SIMD != 0 */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif
#endif /* CPU_SMP */


struct cpu
{
    unsigned int initialized;       /* Processor information acquired */
    unsigned int numCores;          /* Number of cores available */
};


static void retrieveCpuInfo(void);


static struct cpu cpu;


/**
 * getNumberOfCores
 *
 * Number of cores retrieval in an SMP system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the number of cores in an SMP system
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  Number of cores in SMP system
 */
HIDDEN unsigned int getNumberOfCores(void)
{
    /* Get CPU-specific information */
    getCpuInfo();

    /* Return number of cores in SMP system */
    return cpu.numCores;
}


/**
 * getCpuInfo
 *
 * Get CPU-specific information
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves CPU-specific information from the OS
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
HIDDEN void getCpuInfo(void)
{
    /* Is processor info module not initialized? */
    if (cpu.initialized == 0) {
        /* Retrieve processor information */
        retrieveCpuInfo();
        /* Set processor info module as initialized */
        cpu.initialized = 1;
    }
}

/**
 * retrieveCpuInfo
 *
 * Retrieve CPU info (number of cores and architecture-specific supported SIMD)
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - The number of cores in an SMP system by interfacing with the OS
 *  - The set of architecture-specific supported SIMD extensions
 *  To retrieve the number of cores in an SMP system, the supported OSs are:
 *  - Windows
 *  - Mac OS
 *  - Linux
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
static void retrieveCpuInfo(void)
{
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#ifdef WIN32
    /* Windows code */
    DWORD count;
    SYSTEM_INFO sysinfo;
#elif MACOS
    /* Mac OS code */
    int nm[2];
    size_t len = 4;
    uint32_t count;
#else
    /* Linux code */
    long int count;
#endif
#endif /* CPU_SMP */

    /* By default assume that only one core is used */
    cpu.numCores = 1;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#ifdef WIN32
    /* Windows code */
    /* Retrieve number of cores using the proper API */
    GetSystemInfo(&sysinfo);
    count = sysinfo.dwNumberOfProcessors;
#elif MACOS
    /* Mac OS code */
    /* Retrieve number of cores using the proper APIs */
    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
    }
#else
    /* Linux code */
    /* Retrieve number of cores using the proper API */
    count = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    /* In case of error, set number of cores to 1 */
    if (count < 1) {
        count = 1;
    }

    /* Store number of cores */
    cpu.numCores = (unsigned int)count;
#endif /* CPU_SMP */

#if CPU_ENABLE_SIMD != 0
#if defined(ARCH_AMD64)
    retrieveAmd64CpuInfo();
#elif defined(ARCH_X86)
    retrieveX86CpuInfo();
#endif
#endif /* CPU_ENABLE_SIMD != 0 */
}
