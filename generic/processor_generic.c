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


struct processor
{
    unsigned int initialized;       /* Processor information acquired */
    unsigned int numCores;          /* Number of cores available */
    unsigned int sse2Supported;     /* x86 SSE2 instruction set supported */
    unsigned int avxSupported;      /* x86 AVX instruction set supported */
    unsigned int fmaSupported;      /* x86 FMA instruction set supported */
    unsigned int avx512fSupported;  /* x86 AVX512F instruction set supported */
};


static void getProcessorInfo(void);


static struct processor processor;


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
    /* Is processor info module not initialized? */
    if (processor.initialized == 0) {
        /* Retrieve processor information */
        getProcessorInfo();
        /* Set processor info module as initialized */
        processor.initialized = 1;
    }

    /* Return number of cores in SMP system */
    return processor.numCores;
}

/**
 * x86Sse2Supported
 *
 * x86 SSE2 instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of x86 SSE2 instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if x86 SSE2 instruction set is available, 0 otherwise
 */
HIDDEN unsigned int x86Sse2Supported(void)
{
    /* Is processor info module not initialized? */
    if (processor.initialized == 0) {
        /* Retrieve processor information */
        getProcessorInfo();
        /* Set processor info module as initialized */
        processor.initialized = 1;
    }

    /* Return x86 SSE2 instruction set supported by the system */
    return processor.sse2Supported;
}

/**
 * x86AvxSupported
 *
 * x86 AVX instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of x86 AVX instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if x86 AVX instruction set is available, 0 otherwise
 */
HIDDEN unsigned int x86AvxSupported(void)
{
    /* Is processor info module not initialized? */
    if (processor.initialized == 0) {
        /* Retrieve processor information */
        getProcessorInfo();
        /* Set processor info module as initialized */
        processor.initialized = 1;
    }

    /* Return x86 AVX instruction set supported by the system */
    return processor.avxSupported;
}

/**
 * x86FmaSupported
 *
 * x86 FMA instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of x86 FMA instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if x86 FMA instruction set is available, 0 otherwise
 */
HIDDEN unsigned int x86FmaSupported(void)
{
    /* Is processor info module not initialized? */
    if (processor.initialized == 0) {
        /* Retrieve processor information */
        getProcessorInfo();
        /* Set processor info module as initialized */
        processor.initialized = 1;
    }

    /* Return x86 FMA instruction set supported by the system */
    return processor.fmaSupported;
}

/**
 * x86Avx512fSupported
 *
 * x86 AVX512F instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of x86 AVX512F instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if x86 AVX512F instruction set is available, 0 otherwise
 */
HIDDEN unsigned int x86Avx512fSupported(void)
{
    /* Is processor info module not initialized? */
    if (processor.initialized == 0) {
        /* Retrieve processor information */
        getProcessorInfo();
        /* Set processor info module as initialized */
        processor.initialized = 1;
    }

    /* Return x86 AVX512F instruction set supported by the system */
    return processor.avx512fSupported;
}

/**
 * getProcessorInfo
 *
 * Retrieve processor info (number of cores and supported instruction sets)
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
 *  - If AVX instruction set is supported
 *  - If FMA instruction set is supported
 *  To retrieve the number of cores in an SMP system, the supported OSs are:
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
static void getProcessorInfo(void)
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

    /**
     * Default processor info:
     * - 1 core
     * - x86 instruction set: no SSE2, no AVX, no FMA, no AVX512F
     */
    processor.numCores = 1;
    processor.sse2Supported = 0;
    processor.avxSupported = 0;
    processor.fmaSupported = 0;
    processor.avx512fSupported = 0;

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
    processor.numCores = (unsigned int)count;
#endif /* CPU_SMP */

#if CPU_X86_SSE2 != 0
    if (__builtin_cpu_supports("sse2") > 0) {
        processor.sse2Supported = 1;
#if CPU_X86_AVX != 0
        if (__builtin_cpu_supports("avx") > 0) {
            processor.avxSupported = 1;
#if CPU_X86_FMA != 0
            if (__builtin_cpu_supports("fma") > 0) {
                processor.fmaSupported = 1;
#if CPU_X86_AVX512F != 0
                if (__builtin_cpu_supports("avx512f") > 0) {
                    processor.avx512fSupported = 1;
                }
#endif /* CPU_X86_AVX512F */
            }
#endif /* CPU_X86_FMA */
        }
#endif /* CPU_X86_AVX */
    }
#endif /* CPU_X86_SSE2 */
}
