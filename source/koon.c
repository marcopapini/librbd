/*
 *  Component: koon.c
 *  KooN (K-out-of-N) RBD management
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


#include "generic/rbd_internal_generic.h"

#include "generic/binomial.h"
#include "koon.h"


/**
 * rbdKooNGeneric
 *
 * Compute reliability of a generic KooN (K-out-of-N) RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of KooN RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdKooNGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    int res;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNGenericData *koonData;
    struct rbdKooNFillData *fillData;
    void *threadHandles;
    unsigned int idx;
    unsigned int numCores;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNGenericData koonData[1];
    struct rbdKooNFillData fillData[1];
#endif /* CPU_SMP */

    /* If K is 1 then it is a Parallel block */
    if (minComponents == 1) {
        return rbdParallelGeneric(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Series block */
    if (minComponents == numComponents) {
        return rbdSeriesGeneric(reliabilities, output, numComponents, numTimes);
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);
#endif /* CPU_SMP */

    res = 0;

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if (minComponents > numComponents) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Allocate fill KooN data array, return -1 in case of allocation failure */
        fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
        if (fillData == NULL) {
            return -1;
        }

        if (numCores > 1) {
            /* Allocate Thread ID array, return -1 in case of allocation failure */
            threadHandles = allocateThreadHandles(numCores - 1);
            if (threadHandles == NULL) {
                free(fillData);
                return -1;
            }

            /* For each available core... */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].numCores = numCores;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 0.0;

                /* Create the fill output data Worker thread */
                if (createThread(threadHandles, idx, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Prepare fill Output data structure */
            fillData[idx].batchIdx = idx;
            fillData[idx].numCores = numCores;
            fillData[idx].output = output;
            fillData[idx].numTimes = numTimes;
            fillData[idx].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[idx]);

            /* Wait for created threads completion */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                waitThread(threadHandles, idx);
            }
            free(threadHandles);
        }
        else {
#endif /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].numCores = 1;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        }

        free(fillData);
#endif /* CPU_SMP */

        return res;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if (minComponents == 0) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Allocate fill KooN data array, return -1 in case of allocation failure */
        fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
        if (fillData == NULL) {
            return -1;
        }

        if (numCores > 1) {
            /* Allocate Thread ID array, return -1 in case of allocation failure */
            threadHandles = allocateThreadHandles(numCores - 1);
            if (threadHandles == NULL) {
                free(fillData);
                return -1;
            }

            /* For each available core... */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].numCores = numCores;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 1.0;

                /* Create the fill output data Worker thread */
                if (createThread(threadHandles, idx, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Prepare fill Output data structure */
            fillData[idx].batchIdx = idx;
            fillData[idx].numCores = numCores;
            fillData[idx].output = output;
            fillData[idx].numTimes = numTimes;
            fillData[idx].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[idx]);

            /* Wait for created threads completion */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                waitThread(threadHandles, idx);
            }
            free(threadHandles);
        }
        else {
#endif /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].numCores = 1;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        }

        free(fillData);
#endif /* CPU_SMP */

        return res;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Allocate generic KooN RBD data array, return -1 in case of allocation failure */
    koonData = (struct rbdKooNGenericData *)malloc(sizeof(struct rbdKooNGenericData) * numCores);
    if (koonData == NULL) {
        return -1;
    }

    /* Is number of used cores greater than 1? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 0; idx < (numCores - 1); ++idx) {
            /* Prepare generic KooN RBD koonData structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].numCores = numCores;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].numTimes = numTimes;
            initKooNRecursionData(&koonData[idx].recur);

            /* Create the generic KooN RBD Worker thread */
            if (createThread(threadHandles, idx, &rbdKooNGenericWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare generic KooN RBD koonData structure */
        koonData[idx].batchIdx = idx;
        koonData[idx].numCores = numCores;
        koonData[idx].reliabilities = reliabilities;
        koonData[idx].output = output;
        koonData[idx].numComponents = numComponents;
        koonData[idx].minComponents = minComponents;
        koonData[idx].numTimes = numTimes;
        initKooNRecursionData(&koonData[idx].recur);

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNGenericWorker(&koonData[idx]);

        /* Wait for created threads completion */
        for(idx = 0; idx < (numCores - 1); ++idx) {
            waitThread(threadHandles, idx);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare generic KooN RBD koonData structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = 1;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].numTimes = numTimes;
        initKooNRecursionData(&koonData[0].recur);

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNGenericWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free generic KooN RBD koonData array */
    free(koonData);
#endif /* CPU_SMP */

    return res;
}

/**
 * rbdKooNIdentical
 *
 * Compute reliability of an identical KooN (K-out-of-N) RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdKooNIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    unsigned short ii;
    unsigned int idx;
    int res;
    unsigned long long nCi[SCHAR_MAX];
    unsigned char minFaultyComponents;
    unsigned char bComputeUnreliability;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNIdenticalData *koonData;
    struct rbdKooNFillData *fillData;
    void *threadHandles;
    unsigned int numCores;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNIdenticalData koonData[1];
    struct rbdKooNFillData fillData[1];
#endif /* CPU_SMP */

    /* If K is 1 then it is a Parallel block */
    if (minComponents == 1) {
        return rbdParallelIdentical(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Series block */
    if (minComponents == numComponents) {
        return rbdSeriesIdentical(reliabilities, output, numComponents, numTimes);
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);
#endif /* CPU_SMP */

    res = 0;

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if (minComponents > numComponents) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Allocate fill KooN data array, return -1 in case of allocation failure */
        fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
        if(fillData == NULL) {
            return -1;
        }

        if (numCores > 1) {
            /* Allocate Thread ID array, return -1 in case of allocation failure */
            threadHandles = allocateThreadHandles(numCores - 1);
            if (threadHandles == NULL) {
                free(fillData);
                return -1;
            }

            /* For each available core... */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].numCores = numCores;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 0.0;

                /* Create the fill output data Worker thread */
                if (createThread(threadHandles, idx, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Prepare fill Output data structure */
            fillData[idx].batchIdx = idx;
            fillData[idx].numCores = numCores;
            fillData[idx].output = output;
            fillData[idx].numTimes = numTimes;
            fillData[idx].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[idx]);

            /* Wait for created threads completion */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                waitThread(threadHandles, idx);
            }
            free(threadHandles);
        }
        else {
#endif /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].numCores = 1;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif /* CPU_SMP */

        return res;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if (minComponents == 0) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        /* Allocate fill KooN data array, return -1 in case of allocation failure */
        fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
        if(fillData == NULL) {
            return -1;
        }

        if (numCores > 1) {
            /* Allocate Thread ID array, return -1 in case of allocation failure */
            threadHandles = allocateThreadHandles(numCores - 1);
            if (threadHandles == NULL) {
                free(fillData);
                return -1;
            }

            /* For each available core... */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].numCores = numCores;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 1.0;

                /* Create the fill output data Worker thread */
                if (createThread(threadHandles, idx, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Prepare fill Output data structure */
            fillData[idx].batchIdx = idx;
            fillData[idx].numCores = numCores;
            fillData[idx].output = output;
            fillData[idx].numTimes = numTimes;
            fillData[idx].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[idx]);

            /* Wait for created threads completion */
            for (idx = 0; idx < (numCores - 1); ++idx) {
                waitThread(threadHandles, idx);
            }
            free(threadHandles);
        }
        else {
#endif /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].numCores = 1;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif /* CPU_SMP */

        return res;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Allocate generic KooN RBD data array, return -1 in case of allocation failure */
    koonData = (struct rbdKooNIdenticalData *)malloc(sizeof(struct rbdKooNIdenticalData) * numCores);
    if (koonData == NULL) {
        return -1;
    }
#endif /* CPU_SMP */

    bComputeUnreliability = 0;

    /* Compute minimum number of faulty components for having an unreliable block */
    minFaultyComponents = numComponents - minComponents + 1;
    /* Is minimum number of faulty components greater than minimum number of components? */
    if (minFaultyComponents > minComponents) {
        /* Assign minimum number of faulty components to minimum number of components */
        minComponents = minFaultyComponents;
        /* Set KooN computation through Unreliability flag */
        bComputeUnreliability = 1;
    }

    /* Compute all binomial coefficients nCi for i in [k, n] */
    ii = minComponents;
    idx = 0;
    do {
        nCi[idx] = binomialCoefficient(numComponents, ii++);
        if (nCi[idx++] == 0) {
            return -1;
        }
    }
    while (ii <= numComponents);

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 0; idx < (numCores - 1); ++idx) {
            /* Prepare identical KooN RBD data structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].numCores = numCores;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].bComputeUnreliability = bComputeUnreliability;
            koonData[idx].numTimes = numTimes;
            koonData[idx].nCi = &nCi[0];

            /* Create the identical KooN RBD Worker thread */
            if (createThread(threadHandles, idx, &rbdKooNIdenticalWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare identical KooN RBD data structure */
        koonData[idx].batchIdx = idx;
        koonData[idx].numCores = numCores;
        koonData[idx].reliabilities = reliabilities;
        koonData[idx].output = output;
        koonData[idx].numComponents = numComponents;
        koonData[idx].minComponents = minComponents;
        koonData[idx].bComputeUnreliability = bComputeUnreliability;
        koonData[idx].numTimes = numTimes;
        koonData[idx].nCi = &nCi[0];

        /* Directly invoke the identical KooN RBD Worker */
        (void)rbdKooNIdenticalWorker(&koonData[idx]);

        /* Wait for created threads completion */
        for (idx = 0; idx < (numCores - 1); ++idx) {
            waitThread(threadHandles, idx);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare identical KooN RBD data structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = 1;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].bComputeUnreliability = bComputeUnreliability;
        koonData[0].numTimes = numTimes;
        koonData[0].nCi = &nCi[0];

        /* Directly invoke the identical KooN RBD Worker */
        (void)rbdKooNIdenticalWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free identical KooN RBD data array */
    free(koonData);
#endif /* CPU_SMP */

    return res;
}
