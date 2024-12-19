/*
 *  Component: vs.c
 *  Compiler management - Visual Studio compiler
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


#include "compiler.h"

#if defined(COMPILER_VS)

#include <memoryapi.h>

#if CPU_SMP != 0
#include <stdlib.h>
/* Include processthreadsapi for SMP */
#include <processthreadsapi.h>
#endif /* CPU_SMP != 0 */


#define PREFETCH_SIZE           64


#if CPU_SMP != 0
struct winThreadData
{
    fpWorker fpWorker;
    void *args;
};
#endif /* CPU_SMP != 0 */


static HANDLE processHandle;


#if CPU_SMP != 0
static DWORD WINAPI threadEntryPoint(LPVOID args);
#endif /* CPU_SMP != 0 */


/**
 * prefetch
 *
 * Prefetch memory
 *
 * Input:
 *      void *address
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetches memory (PREFETCH_SIZE bytes) using Visual Studio
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
static inline ALWAYS_INLINE prefetch(void *address)
{
    WIN32_MEMORY_RANGE_ENTRY entry;

    entry.VirtualAddress = address;
    entry.NumberOfBytes = PREFETCH_SIZE;

    if (processHandle == NULL) {
        processHandle = GetCurrentProcess();
    }

    (void)PrefetchVirtualMemory(processHandle, 1, &entry, 0);
}

/**
 * compilerPrefetchRead
 *
 * Prefetch memory for read access
 *
 * Input:
 *      void *address
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetches memory for read access using Visual Studio
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
HIDDEN void compilerPrefetchRead(void *address)
{
    prefetch(address);
}

/**
 * compilerPrefetchWrite
 *
 * Prefetch memory for write access
 *
 * Input:
 *      void *address
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetches memory for write access using Visual Studio
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
HIDDEN void compilerPrefetchWrite(void *address)
{
    prefetch(address);
}


#if CPU_SMP != 0

/**
 * allocateThreadHandles
 *
 * Allocate memory for the requested thread handles
 *
 * Input:
 *      unsigned int numThreads
 *
 * Output:
 *      None
 *
 * Description:
 *  This function allocates memory for the requested thread handles using Windows model
 *
 * Parameters:
 *      numThreads: number of thread handles to allocate
 *
 * Return (void *):
 *  != NULL in case of successful thread handles allocation, NULL otherwise
 */
HIDDEN void *allocateThreadHandles(unsigned int numThreads)
{
    HANDLE *threadHandles;

    /* Allocate thread handles (Windows model) */
    threadHandles = (HANDLE *)malloc(sizeof(HANDLE) * numThreads);

    return threadHandles;
}

/**
 * createThread
 *
 * Create RBD Worker thread
 *
 * Input:
 *      void *threadHandles
 *      unsigned int threadIdx
 *      fpWorker fpWorker
 *      void *args
 *
 * Output:
 *      None
 *
 * Description:
 *  This function creates the requested RBD Worker thread using Windows model
 *
 * Parameters:
 *      threadHandles: array of thread handles
 *      threadIdx: index of requested thread
 *      fpWorker: pointer to the RBD Worker function executed by thread
 *      args: arguments provided to thread
 *
 * Return (int):
 *  0 in case of successful thread creation, -1 otherwise
 */
HIDDEN int createThread(void *threadHandles, unsigned int threadIdx, fpWorker fpWorker, void *args)
{
    struct winThreadData *data;
    HANDLE thread;
    HANDLE *pHandles = (HANDLE *)threadHandles;

    /* Allocate Windows thread data structure (Windows model) */
    data = (HANDLE*)malloc(sizeof(struct winThreadData));
    if (data == NULL) {
        return -1;
    }

    data->fpWorker = fpWorker;
    data->args = args;

    /* Create the RBD Worker thread (Windows model) */
    thread = CreateThread(NULL, 0, &threadEntryPoint, data, 0, NULL);
    if (thread == NULL) {
        return -1;
    }
    pHandles[threadIdx] = thread;
    return 0;
}

/**
 * waitThread
 *
 * Wait for the RBD Worker thread completion
 *
 * Input:
 *      void *threadHandles
 *      unsigned int threadIdx
 *
 * Output:
 *      None
 *
 * Description:
 *  This function waits for the completion of the requested RBD Worker thread using Windows model
 *
 * Parameters:
 *      threadHandles: array of thread handles
 *      threadIdx: index of requested thread
 *
 * Return:
 *  None
 */
HIDDEN void waitThread(void *threadHandles, unsigned int threadIdx)
{
    HANDLE *pHandles = (HANDLE *)threadHandles;

    /* Wait for RBD Worker thread completion (Windows model) */
    (void)WaitForSingleObject(pHandles[threadIdx], INFINITE);
}

/**
 * threadEntryPoint
 *
 * Entry point of created thread
 *
 * Input:
 *      LPVOID args
 *
 * Output:
 *      None
 *
 * Description:
 *  This function is the entry point of each thread created using Windows model
 *
 * Parameters:
 *      args: pointer to struct winThreadData provided as void *
 *
 * Return (DWORD):
 *  Always returns 0
 */
static DWORD WINAPI threadEntryPoint(LPVOID args)
{
    struct winThreadData *data = (struct winThreadData *)args;

    (void)(*data->fpWorker)(data->args);

    free(data);

    return 0;
}
#endif /* CPU_SMP != 0 */

#endif /* defined(COMPILER_VS) */
