/*
 *  Component: xlc.c
 *  Compiler management - XL C compiler
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


#include "xlc.h"

#if defined(COMPILER_XLC)

#if CPU_SMP != 0
#include <stdlib.h>
/* Include pthread for SMP */
#include <pthread.h>


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
 *  This function allocates memory for the requested thread handles using pthread model
 *
 * Parameters:
 *      numThreads: number of thread handles to allocate
 *
 * Return (void *):
 *  != NULL in case of successful thread handles allocation, NULL otherwise
 */
HIDDEN void *allocateThreadHandles(unsigned int numThreads)
{
    pthread_t *threadHandles;

    /* Allocate thread handles (pthread model) */
    threadHandles = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);

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
 *  This function creates the requested RBD Worker thread using pthread model
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
    pthread_t *pHandles = (pthread_t *)threadHandles;

    /* Create the RBD Worker thread (pthread model) */
    if (pthread_create(&pHandles[threadIdx], NULL, fpWorker, args) < 0) {
        return -1;
    }
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
 *  This function waits for the completion of the requested RBD Worker thread using pthread model
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
    pthread_t *pHandles = (pthread_t *)threadHandles;

    /* Wait for RBD Worker thread completion (pthread model) */
    (void)pthread_join(pHandles[threadIdx], NULL);
}
#endif /* CPU_SMP != 0 */

#endif /* defined(COMPILER_XLC) */
