/*
 *  Component: k_of_n.c
 *  KooN (K-out-of-N) RBD management
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


#include "rbd.h"

#include "binomial.h"
#include "combinations.h"
#include "rbd_internal.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#include <pthread.h>
#endif  /* CPU_SMP */


struct combinationsKooN
{
    unsigned char numKooNcombinations;              /* Number of combinations of combinations used for computation of KooN */
    struct combinations *combinations[UCHAR_MAX];   /* Combinations of combinations used for computation of KooN */
};

struct rbdKooNFillData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int batchSize;                         /* Size of work batch */
    double *output;                                 /* Array of output Reliability filled with fixed value */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    double value;                                   /* Fixed value used to fill output Reliability array */
};

struct rbdKooNGenericData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int batchSize;                         /* Size of work batch */
    double *reliabilities;                          /* Matrix of reliabilities of KooN RBD system */
    double *output;                                 /* Array of computed reliabilities */
    unsigned char numComponents;                    /* Number of components of KooN RBD system N */
    unsigned char minComponents;                    /* Minimum number of components in the KooN system (K) */
    unsigned char bComputeUnreliability;            /* Flag for KooN resolution through usage of Unreliability */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    struct combinationsKooN *combs;                 /* Possible combinations of combinations of KooN components */
};

struct rbdKooNIdenticalData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int batchSize;                         /* Size of work batch */
    double *reliabilities;                          /* Array of reliabilities of KooN RBD system */
    double *output;                                 /* Array of computed reliabilities */
    unsigned char numComponents;                    /* Number of components of KooN RBD system N */
    unsigned char minComponents;                    /* Minimum number of components in the KooN system (K) */
    unsigned char bComputeUnreliability;            /* Flag for KooN resolution through usage of Unreliability */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    unsigned long long *nCi;                        /* Array of nCi values computed for n=N and i in [K, N] */
};


static void *rbdKooNFillWorker(void *arg);
static void *rbdKooNGenericFastWorker(void *arg);
static void *rbdKooNGenericRecursiveWorker(void *arg);
static void *rbdKooNIdenticalWorker(void *arg);
static double rbdKooNGenericSuccessStep(unsigned char n, unsigned char k, unsigned char *combination, double *reliabilities);
static double rbdKooNGenericFailStep(unsigned char n, unsigned char k, unsigned char *combination, double *reliabilities);
static double rbdKooNIdenticalSuccessStep(unsigned char n, unsigned char k, unsigned long long nck, double reliability);
static double rbdKooNIdenticalFailStep(unsigned char n, unsigned char k, unsigned long long nck, double reliability);
static double rbdKooNRecursiveStep(double *r, unsigned char n, unsigned char k);


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
 *                      a TxN one, where N is the number of components of KooN RBD
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
int rbdKooNGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    unsigned char ii;
    struct combinationsKooN combs;
    int res;
    unsigned char bRecursive;
    unsigned char minFaultyComponents;
    unsigned char bComputeUnreliability;
    unsigned int nSquare;
    unsigned long long numCombinations;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNGenericData *koonData;
    struct rbdKooNFillData *fillData;
    pthread_t *thread_id;
    unsigned int numCores;
    unsigned int idx;
    unsigned int batchSize;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNGenericData koonData[1];
    struct rbdKooNFillData fillData[1];
#endif  /* CPU_SMP */

    /* If K is 1 then it is a Series block */
    if(minComponents == 1) {
        return rbdSeriesGeneric(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Parallel block */
    if(minComponents == numComponents) {
        return rbdParallelGeneric(reliabilities, output, numComponents, numTimes);
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Retrieve number of cores available in SMP system */
    numCores = getNumberOfCores();
    /* Compute batch size */
    batchSize = min(ceilDivision(numTimes, numCores), MIN_BATCH_SIZE);
    /* Compute the number of used cores given the batch size and the number of available cores */
    numCores = ceilDivision(numTimes, batchSize);

    /* Allocate fill KooN data array, return -1 in case of allocation failure */
    fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
    if(fillData == NULL) {
        return -1;
    }

    if(numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        thread_id = (pthread_t *)malloc(sizeof(pthread_t) * numCores);
        if(thread_id == NULL) {
            free(fillData);
            return -1;
        }
    }
#endif  /* CPU_SMP */

    res = 0;

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if(minComponents > numComponents) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        if(numCores > 1) {
            /* For each available core... */
            for(idx = 0; idx < numCores; ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].batchSize = batchSize;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 0.0;

                /* Create the fill output data Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Wait for created threads completion */
            for(idx = 0; idx < numCores; ++idx) {
                (void)pthread_join(thread_id[idx], NULL);
            }
        }
        else {
#endif  /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].batchSize = numTimes;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif  /* CPU_SMP */

        return res;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if(minComponents == 0) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        if(numCores > 1) {
            /* For each available core... */
            for(idx = 0; idx < numCores; ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].batchSize = batchSize;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 1.0;

                /* Create the fill output data Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Wait for created threads completion */
            for(idx = 0; idx < numCores; ++idx) {
                (void)pthread_join(thread_id[idx], NULL);
            }
        }
        else {
#endif  /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].batchSize = numTimes;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif  /* CPU_SMP */

        return res;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Free fill data structure */
    free(fillData);

    /* Allocate generic KooN RBD data array, return -1 in case of allocation failure */
    koonData = (struct rbdKooNGenericData *)malloc(sizeof(struct rbdKooNGenericData) * numCores);
    if(koonData == NULL) {
        free(thread_id);
        return -1;
    }
#endif  /* CPU_SMP */

    bComputeUnreliability = 0;
    bRecursive = 0;

    /* Compute N^2 for further optimizations (recursive formula) */
    nSquare = numComponents * numComponents;

    /* Initialize total number of combinations to 0 */
    numCombinations = 0;

    /* Compute minimum number of faulty components for having an unreliable block */
    minFaultyComponents = numComponents - minComponents + 1;
    /* Is minimum number of faulty components greater than minimum number of components? */
    if(minFaultyComponents > minComponents) {
        /* Assign minimum number of faulty components to minimum number of components */
        minComponents = minFaultyComponents;
        /* Set KooN computation through Unreliability flag */
        bComputeUnreliability = 1;
    }

    /* Initialize combinations of combinations for KooN computation */
    combs.numKooNcombinations = (numComponents - minComponents) + 1;
    ii = 0;
    do {
        combs.combinations[ii] = computeCombinations(numComponents, (ii + minComponents));
        numCombinations += combs.combinations[ii]->numCombinations;
        bRecursive  = !!(combs.combinations[ii] == NULL);
        bRecursive |= !!(numCombinations > nSquare);
        ++ii;
    }
    while((ii < combs.numKooNcombinations) && (bRecursive == 0));

    if(bRecursive != 0) {
        while(ii > 0) {
            free(combs.combinations[--ii]);
        }
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Is number of used cores greater than 1? */
    if(numCores > 1) {
        /* For each available core... */
        for(idx = 0; idx < numCores; ++idx) {
            /* Prepare generic KooN RBD koonData structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].batchSize = batchSize;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].bComputeUnreliability = bComputeUnreliability;
            koonData[idx].numTimes = numTimes;
            koonData[idx].combs = &combs;

            /* Iterative computation used to compute data? */
            if(bRecursive == 0) {
                /* Create the generic KooN RBD Fast Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNGenericFastWorker, &koonData[idx]) < 0) {
                    res = -1;
                }
            }
            else {
                /* Create the generic KooN RBD Recursive Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNGenericRecursiveWorker, &koonData[idx]) < 0) {
                    res = -1;
                }
            }
        }

        /* Wait for created threads completion */
        for(idx = 0; idx < numCores; ++idx) {
            (void)pthread_join(thread_id[idx], NULL);
        }

        /* Free Thread ID array */
        free(thread_id);
    }
    else {
#endif  /* CPU_SMP */
        /* SMP not required/needed */
        /* Prepare generic KooN RBD koonData structure */
        koonData[0].batchIdx = 0;
        koonData[0].batchSize = numTimes;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].bComputeUnreliability = bComputeUnreliability;
        koonData[0].numTimes = numTimes;
        koonData[0].combs = &combs;

        /* Iterative computation used to compute data? */
        if(bRecursive == 0) {
            /* Directly invoke the generic KooN RBD Fast Worker */
            (void)rbdKooNGenericFastWorker(&koonData[0]);
        }
        else {
            /* Directly invoke the generic KooN RBD Recursive Worker */
            (void)rbdKooNGenericRecursiveWorker(&koonData[0]);
        }
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free generic KooN RBD koonData array */
    free(koonData);
#endif  /* CPU_SMP */

    /* Free combinations if recursive approach has not been used */
    if(bRecursive == 0) {
        ii = combs.numKooNcombinations;
        while(ii > 0) {
            --ii;
            free(combs.combinations[ii]);
        }
    }

    /* If computation has been performed, ensure that output is a Reliability curve */
    if (res >= 0) {
        postProcessReliability(output, numTimes);
    }

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
int rbdKooNIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    unsigned short ii;
    unsigned int idx;
    int res;
    unsigned long long nCi[UCHAR_MAX];
    unsigned char minFaultyComponents;
    unsigned char bComputeUnreliability;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNIdenticalData *koonData;
    struct rbdKooNFillData *fillData;
    pthread_t *thread_id;
    unsigned int numCores;
    unsigned int batchSize;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNIdenticalData koonData[1];
    struct rbdKooNFillData fillData[1];
#endif  /* CPU_SMP */

    /* If K is 1 then it is a Series block */
    if(minComponents == 1) {
        return rbdSeriesIdentical(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Parallel block */
    if(minComponents == numComponents) {
        return rbdParallelIdentical(reliabilities, output, numComponents, numTimes);
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Retrieve number of cores available in SMP system */
    numCores = getNumberOfCores();
    /* Compute batch size */
    batchSize = min(ceilDivision(numTimes, numCores), MIN_BATCH_SIZE);
    /* Compute the number of used cores given the batch size and the number of available cores */
    numCores = ceilDivision(numTimes, batchSize);

    /* Allocate fill KooN data array, return -1 in case of allocation failure */
    fillData = (struct rbdKooNFillData *)malloc(sizeof(struct rbdKooNFillData) * numCores);
    if(fillData == NULL) {
        return -1;
    }

    if(numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        thread_id = (pthread_t *)malloc(sizeof(pthread_t) * numCores);
        if(thread_id == NULL) {
            free(fillData);
            return -1;
        }
    }
#endif  /* CPU_SMP */

    res = 0;

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if(minComponents > numComponents) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        if(numCores > 1) {
            /* For each available core... */
            for(idx = 0; idx < numCores; ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].batchSize = batchSize;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 0.0;

                /* Create the fill output data Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Wait for created threads completion */
            for(idx = 0; idx < numCores; ++idx) {
                (void)pthread_join(thread_id[idx], NULL);
            }
        }
        else {
#endif  /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].batchSize = numTimes;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 0.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif  /* CPU_SMP */

        return res;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if(minComponents == 0) {
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
        if(numCores > 1) {
            /* For each available core... */
            for(idx = 0; idx < numCores; ++idx) {
                /* Prepare fill Output data structure */
                fillData[idx].batchIdx = idx;
                fillData[idx].batchSize = batchSize;
                fillData[idx].output = output;
                fillData[idx].numTimes = numTimes;
                fillData[idx].value = 1.0;

                /* Create the fill output data Worker thread */
                if(pthread_create(&thread_id[idx], NULL, &rbdKooNFillWorker, &fillData[idx]) < 0) {
                    res = -1;
                }
            }

            /* Wait for created threads completion */
            for(idx = 0; idx < numCores; ++idx) {
                (void)pthread_join(thread_id[idx], NULL);
            }
        }
        else {
#endif  /* CPU_SMP */
            /* Prepare fill Output data structure */
            fillData[0].batchIdx = 0;
            fillData[0].batchSize = numTimes;
            fillData[0].output = output;
            fillData[0].numTimes = numTimes;
            fillData[0].value = 1.0;

            (void)rbdKooNFillWorker(&fillData[0]);
#if CPU_SMP != 0
        }

        free(fillData);
#endif  /* CPU_SMP */

        return res;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Free fill data structure */
    free(fillData);

    /* Allocate generic KooN RBD data array, return -1 in case of allocation failure */
    koonData = (struct rbdKooNIdenticalData *)malloc(sizeof(struct rbdKooNIdenticalData) * numCores);
    if(koonData == NULL) {
        free(thread_id);
        return -1;
    }
#endif  /* CPU_SMP */

    bComputeUnreliability = 0;

    /* Compute minimum number of faulty components for having an unreliable block */
    minFaultyComponents = numComponents - minComponents + 1;
    /* Is minimum number of faulty components greater than minimum number of components? */
    if(minFaultyComponents > minComponents) {
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
        if(nCi[idx++] == 0) {
            return -1;
        }
    }
    while(ii <= numComponents);

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    if(numCores > 1) {
        /* For each available core... */
        for(idx = 0; idx < numCores; ++idx) {
            /* Prepare identical KooN RBD data structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].batchSize = batchSize;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].bComputeUnreliability = bComputeUnreliability;
            koonData[idx].numTimes = numTimes;
            koonData[idx].nCi = &nCi[0];

            /* Create the identical KooN RBD Worker thread */
            if(pthread_create(&thread_id[idx], NULL, &rbdKooNIdenticalWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Wait for created threads completion */
        for(idx = 0; idx < numCores; ++idx) {
            (void)pthread_join(thread_id[idx], NULL);
        }

        /* Free Thread ID array */
        free(thread_id);
    }
    else {
#endif  /* CPU_SMP */
        /* SMP not required/needed */
        /* Prepare identical KooN RBD data structure */
        koonData[0].batchIdx = 0;
        koonData[0].batchSize = numTimes;
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
#endif  /* CPU_SMP */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Free identical KooN RBD data array */
    free(koonData);
#endif  /* CPU_SMP */

    /* If computation has been performed, ensure that output is a Reliability curve */
    if (res >= 0) {
        postProcessReliability(output, numTimes);
    }

    return res;
}


/**
 * rbdKooNFillWorker
 *
 * Fill output Reliability with fixed value Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker.
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
static void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;
    unsigned int timeLimit;
    unsigned int ii;

    /* Retrieve fill Output data structure */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;

    /* For each time instant... */
    for (ii = time; ii < timeLimit; ++ii) {
        /* Fill output Reliability array with fixed value */
        data->output[ii] = data->value;
    }

    return NULL;
}


/**
 * rbdKooNGenericFastWorker
 *
 * Generic KooN RBD Fast Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Fast Worker.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed combinations of K out of N components
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNGenericFastWorker(void *arg)
{
    struct rbdKooNGenericData *data;
    double stepResult;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    unsigned long long offset;
    unsigned char *buffer;
    unsigned long long numCombinations;
    struct combinationsKooN *combinations;
    unsigned short ii;
    unsigned char n;
    unsigned char k;
    unsigned char *combination;
    double *reliabilities;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve combinations used for KooN computation */
    combinations = data->combs;

    /* Prepare common data for steps computation */
    n = data->numComponents;

    /* If compute unreliability flag is not set... */
    if(data->bComputeUnreliability == 0) {
        /* For each time instant to be processed... */
        while(time < timeLimit) {
            /* Prepare time-dependent data for steps computation */
            reliabilities = &data->reliabilities[time * data->numComponents];

            /* Initialize reliability of each time instant to 0 */
            reliability = 0.0;

            for(ii = 0; ii < combinations->numKooNcombinations; ++ii) {
                /* Prepare length-dependent data for steps computation */
                k = combinations->combinations[ii]->k;

                /* Retrieve number of combinations */
                numCombinations = combinations->combinations[ii]->numCombinations;
                buffer = &combinations->combinations[ii]->buff[0];
                offset = 0;
                while(numCombinations-- > 0) {
                    /* Prepare combination-dependent data for steps computation */
                    combination = &buffer[offset];

                    /* Resolve single step for KooN (success) computation */
                    stepResult = rbdKooNGenericSuccessStep(n, k, combination, reliabilities);
                    /* Perform partial sum for computation of KooN reliability */
                    reliability += stepResult;
                    /* Increment offset for combination access */
                    offset += k;
                }
            }

            /* Cap computed reliability to accepted bounds [0, 1] */
            if(isnan(reliability) != 0) {
                reliability = 0.0;
            }
            if(reliability > 1.0) {
                reliability = 1.0;
            }
            if(reliability < 0.0) {
                reliability = 0.0;
            }

            /* Set computed reliability to output array */
            data->output[time++] = reliability;
        }
    }
    else {
        /* For each time instant to be processed... */
        while(time < timeLimit) {
            /* Prepare time-dependent data for steps computation */
            reliabilities = &data->reliabilities[time * data->numComponents];

            /* Initialize reliability of each time instant to 1 */
            reliability = 1.0;

            for(ii = 0; ii < combinations->numKooNcombinations; ++ii) {
                /* Prepare length-dependent data for steps computation */
                k = combinations->combinations[ii]->k;

                /* Retrieve number of combinations */
                numCombinations = combinations->combinations[ii]->numCombinations;
                buffer = &combinations->combinations[ii]->buff[0];
                offset = 0;
                while(numCombinations-- > 0) {
                    /* Prepare combination-dependent data for steps computation */
                    combination = &buffer[offset];

                    /* Resolve single step for KooN (fail) computation */
                    stepResult = rbdKooNGenericFailStep(n, k, combination, reliabilities);
                    /* Perform partial difference for computation of KooN reliability */
                    reliability -= stepResult;
                    /* Increment offset for combination access */
                    offset += k;
                }
            }

            /* Cap computed reliability to accepted bounds [0, 1] */
            if(isnan(reliability) != 0) {
                reliability = 0.0;
            }
            if(reliability > 1.0) {
                reliability = 1.0;
            }
            if(reliability < 0.0) {
                reliability = 0.0;
            }

            /* Set computed reliability to output array */
            data->output[time++] = reliability;
        }
    }

    return NULL;
}

/**
 * rbdKooNGenericRecursiveWorker
 *
 * Generic KooN RBD Recursive Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Slow Worker.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by
 *  recomputing the combinations of K out of N components at each iteration
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
static void *rbdKooNGenericRecursiveWorker(void *arg)
{
    struct rbdKooNGenericData *data;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    unsigned char n;
    unsigned char k;
    double *reliabilities;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve number of components (N) and minimum components needed by KooN RBD system (K) */
    n = data->numComponents;
    k = data->minComponents;

    /* For each time instant to be processed... */
    while(time < timeLimit) {
        /* Retrieve array of reliabilities over which worker shall work */
        reliabilities = &data->reliabilities[time * data->numComponents];

        /* Recursively compute reliability of each time instant */
        reliability = rbdKooNRecursiveStep(reliabilities, n, k);

        /* Cap computed reliability to accepted bounds [0, 1] */
        if(isnan(reliability) != 0) {
            reliability = 0.0;
        }
        if(reliability > 1.0) {
            reliability = 1.0;
        }
        if(reliability < 0.0) {
            reliability = 0.0;
        }

        /* Set computed reliability to output array */
        data->output[time++] = reliability;
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorker
 *
 * Identical KooN RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker.
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
static void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;
    double stepResult;
    double reliability;
    unsigned int time;
    unsigned int timeLimit;
    unsigned char ii;
    unsigned short numIterations;
    unsigned char n;
    unsigned char k;

    /* Retrieve identical KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = (data->batchSize * data->batchIdx);
    /* Retrieve last time instant to be processed by worker */
    timeLimit = ((time + data->batchSize) < data->numTimes) ? (time + data->batchSize) : data->numTimes;
    /* Retrieve number of components (N) and minimum components needed by KooN RBD system (K) */
    n = data->numComponents;
    k = data->minComponents;
    /* Compute number of iterations */
    numIterations = n - k + 1;

    /* If compute unreliability flag is not set... */
    if(data->bComputeUnreliability == 0) {
        /* For each time instant to be processed... */
        while(time < timeLimit) {
            /* Initialize reliability of each time instant to 0 */
            reliability = 0.0;

            /* For each iteration... */
            for(ii = 0; ii < numIterations; ++ii) {
                /* Resolve single step for KooN (success) computation */
                stepResult = rbdKooNIdenticalSuccessStep(n, (ii + k), data->nCi[ii], data->reliabilities[time]);
                /* Perform partial sum for computation of KooN reliability */
                reliability += stepResult;
            }

            /* Cap computed reliability to accepted bounds [0, 1] */
            if(isnan(reliability) != 0) {
                reliability = 0.0;
            }
            if(reliability > 1.0) {
                reliability = 1.0;
            }
            if(reliability < 0.0) {
                reliability = 0.0;
            }

            /* Set computed reliability to output array */
            data->output[time++] = reliability;
        }
    }
    else {
        /* For each time instant to be processed... */
        while(time < timeLimit) {
            /* Initialize reliability of each time instant to 1 */
            reliability = 1.0;

            /* For each iteration... */
            for(ii = 0; ii < numIterations; ++ii) {
                /* Resolve single step for KooN (fail) computation */
                stepResult = rbdKooNIdenticalFailStep(n, (ii + k), data->nCi[ii], data->reliabilities[time]);
                /* Perform partial difference for computation of KooN reliability */
                reliability -= stepResult;
            }

            /* Cap computed reliability to accepted bounds [0, 1] */
            if(isnan(reliability) != 0) {
                reliability = 0.0;
            }
            if(reliability > 1.0) {
                reliability = 1.0;
            }
            if(reliability < 0.0) {
                reliability = 0.0;
            }

            /* Set computed reliability to output array */
            data->output[time++] = reliability;
        }
    }

    return NULL;
}

/**
 * rbdKooNGenericSuccessStep
 *
 * Generic KooN RBD (success) single step computation function
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *      unsigned char *combination
 *      double *reliabilities
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes a single step for KooN RBD taking into account the success components.
 *
 * Parameters:
 *      n: total number of components in KooN system (N)
 *      k: current number of working components in KooN system
 *      combination: combination to be used for the single step computation
 *      reliabilities: array of reliabilities of the N components
 *
 * Return (double):
 *  Reliability computed by generic KooN (success) single step
 */
static double rbdKooNGenericSuccessStep(unsigned char n, unsigned char k, unsigned char *combination, double *reliabilities)
{
    double res;
    int ii;
    int idx;

    /* Initialize step reliability to 1 */
    res = 1.0;

    /* For each component... */
    for(ii = 0, idx = 0; ii < n; ++ii) {
        /* Does the component belong to the working components for current combination? */
        if(combination[idx] == ii) {
            /* Multiply step reliability for reliability of current component */
            res *= reliabilities[ii];
            /* Advance to next working component in combination */
            if(++idx == k) {
                idx = 0;
            }
        }
        else {
            /* Multiply step reliability for unreliability of current component */
            res *= (1.0 - reliabilities[ii]);
        }
    }

    /* Cap step reliability to accepted bounds [0, 1] */
    if(isnan(res) != 0) {
        res = 0.0;
    }
    if(res > 1.0) {
        res = 1.0;
    }
    if(res < 0.0) {
        res = 0.0;
    }

    return res;
}

/**
 * rbdKooNGenericFailStep
 *
 * Generic KooN RBD (fail) single step computation function
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *      unsigned char *combination
 *      double *reliabilities
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes a single step for KooN RBD taking into account the fail components.
 *
 * Parameters:
 *      n: total number of components in KooN system (N)
 *      k: current number of failed components in KooN system
 *      combination: combination to be used for the single step computation
 *      reliabilities: array of reliabilities of the N components
 *
 * Return (double):
 *  Unreliability computed by generic KooN (fail) single step
 */
static double rbdKooNGenericFailStep(unsigned char n, unsigned char k, unsigned char *combination, double *reliabilities)
{
    double res;
    int ii;
    int idx;

    /* Initialize step unreliability to 1 */
    res = 1.0;

    /* For each component... */
    for(ii = 0, idx = 0; ii < n; ++ii) {
        /* Does the component belong to the failed components for current combination? */
        if(combination[idx] == ii) {
            /* Multiply step unreliability for unreliability of current component */
            res *= (1.0 - reliabilities[ii]);
            /* Advance to next failed component in combination */
            if(++idx == k) {
                idx = 0;
            }
        }
        else {
            /* Multiply step unreliability for reliability of current component */
            res *= reliabilities[ii];
        }
    }

    /* Cap step unreliability to accepted bounds [0, 1] */
    if(isnan(res) != 0) {
        res = 0.0;
    }
    if(res > 1.0) {
        res = 1.0;
    }
    if(res < 0.0) {
        res = 0.0;
    }

    return res;
}

/**
 * rbdKooNIdenticalSuccessStep
 *
 * Identical KooN RBD (success) single step computation function
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *      unsigned long long nCi
 *      double reliability
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes a single step for KooN RBD taking into account the success components.
 *
 * Parameters:
 *      n: total number of components in KooN system (N)
 *      k: number of working components in KooN system
 *      nCi: nCi computed given N and i
 *      reliability: reliability of each component
 *
 * Return (double):
 *  Reliability computed by identical KooN single step
 */
static double rbdKooNIdenticalSuccessStep(unsigned char n, unsigned char k, unsigned long long nCi, double reliability)
{
    double res;
    double unreliability;
    int ii;

    /* Initialize step reliability to nCk */
    res = (double)nCi;

    /* Compute unreliability */
    unreliability = (1.0 - reliability);

    /* For each component in reliability path... */
    for(ii = (k - 1); ii >= 0; --ii) {
        /* Multiply step reliability for reliability of component */
        res *= reliability;
    }
    /* For each component in unreliability path... */
    for(ii = (n - k - 1); ii >= 0; --ii) {
        /* Multiply step reliability for unreliability of component */
        res *= unreliability;
    }

    /* Cap step reliability to accepted bounds [0, 1] */
    if(isnan(res) != 0) {
        res = 0.0;
    }
    if(res > 1.0) {
        res = 1.0;
    }
    if(res < 0.0) {
        res = 0.0;
    }

    return res;
}

/**
 * rbdKooNIdenticalFailStep
 *
 * Identical KooN RBD (fail) single step computation function
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *      unsigned long long nCi
 *      double reliability
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes a single step for KooN RBD taking into account the failed components.
 *
 * Parameters:
 *      n: total number of components in KooN system (N)
 *      k: number of working components in KooN system
 *      nCi: nCi computed given N and i
 *      reliability: reliability of each component
 *
 * Return (double):
 *  Unreliability computed by identical KooN single step
 */
static double rbdKooNIdenticalFailStep(unsigned char n, unsigned char k, unsigned long long nCi, double reliability)
{
    double res;
    double unreliability;
    int ii;

    /* Initialize step unreliability to nCk */
    res = (double)nCi;

    /* Compute unreliability */
    unreliability = (1.0 - reliability);

    /* For each component in reliability path... */
    for(ii = (k - 1); ii >= 0; --ii) {
        /* Multiply step unreliability for unreliability of component */
        res *= unreliability;
    }
    /* For each component in unreliability path... */
    for(ii = (n - k - 1); ii >= 0; --ii) {
        /* Multiply step unreliability for reliability of component */
        res *= reliability;
    }

    /* Cap step unreliability to accepted bounds [0, 1] */
    if(isnan(res) != 0) {
        res = 0.0;
    }
    if(res > 1.0) {
        res = 1.0;
    }
    if(res < 0.0) {
        res = 0.0;
    }

    return res;
}

/**
 * rbdKooNRecursiveStep
 *
 * Recursive KooN RBD Step function
 *
 * Input:
 *      double *r
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function.
 *  It is responsible to recursively compute the reliability over a single time instant of a KooN RBD system
 *
 * Parameters:
 *      r: pointer to reliabilities of components for the given time instant
 *      n: current number of components in RBD KooN block
 *      k: minimum number of working components in RBD KooN block
 *
 * Return (double):
 *  Computed reliability over the single time instant
 */
static double rbdKooNRecursiveStep(double *r, unsigned char n, unsigned char k)
{
    /* If K is 0 then reliability is 1 */
    if (k == 0)
        return 1.0;

    /* If K > N then reliability is 0 */
    if (k > n)
        return 0.0;

    /* Recursively compute the Reliability */
    return (((1.0 - r[n-1]) * rbdKooNRecursiveStep(r, n-1, k))   +
            ((r[n-1])       * rbdKooNRecursiveStep(r, n-1, k-1)));
}
