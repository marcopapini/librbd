/*
 *  Component: cores.c
 *  Number of cores retrieval in an SMP system
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020 by Marco Papini <papini.m@gmail.com>
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


#include "rbd_internal.h"

#if CPU_SMP != 0                                /* Under SMP conditional compiling */

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif


struct cores
{
    unsigned int initialized;       /* Flag used to know if number of cores has already been acquired from OS */
    unsigned int numCores;          /* Number of cores available */
};


static unsigned int getNumberOfCoresInternal(void);


static struct cores cores;


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
__attribute__((visibility ("hidden"))) unsigned int getNumberOfCores(void)
{
    /* Is cores module not initialized? */
    if(cores.initialized == 0) {
        /* Retrieve number of cores from OS */
        cores.numCores = getNumberOfCoresInternal();
        /* Set cores module as initialized */
        cores.initialized = 1;
    }

    /* Return number of cores in SMP system */
    return cores.numCores;
}


/**
 * getNumberOfCoresInternal
 *
 * Retrieve number of cores retrieval in an SMP system by interfacing with the OS
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the number of cores in an SMP system by interfacing with the OS
 *  The supported OSs are:
 *  - Windows
 *  - Mac OS
 *  - Linux
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  Number of cores in SMP system returned by OS APIs
 */
static unsigned int getNumberOfCoresInternal(void)
{
#ifdef WIN32

    /* Windows code */
    DWORD count;
    SYSTEM_INFO sysinfo;

    /* Retrieve number of cores using the proper API */
    GetSystemInfo(&sysinfo);
    count = sysinfo.dwNumberOfProcessors;

#elif MACOS

    /* Mac OS code */
    int nm[2];
    size_t len = 4;
    uint32_t count;

    /* Retrieve number of cores using the proper APIs */
    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
    }

#else

    /* Linux code */
    long int count;

    /* Retrieve number of cores using the proper API */
    count = sysconf(_SC_NPROCESSORS_ONLN);

#endif

    /* In case of error, set number of cores to 1 */
    if(count < 1) {
        count = 1;
    }

    /* Return number of retrieved cores */
    return (unsigned int)count;
}

#endif  /* CPU_SMP */
