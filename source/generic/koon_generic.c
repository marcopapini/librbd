/*
 *  Component: koon_generic.c
 *  KooN (K-out-of-N) RBD management - Generic implementation
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

#include "../koon.h"


static double rbdKooNRecursiveStepS1d(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k);


#if defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0
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
HIDDEN void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;
    unsigned int time;

    /* Retrieve fill Output data structure */
    data = (struct rbdKooNFillData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant... */
    while (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time] = data->value;
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorker
 *
 * Generic KooN RBD Worker function
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a
 *                      void * in order to be compliant with pthread_create API and to thus allow
 *                      SMP computation of KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorker(void *arg)
{
    struct rbdKooNGenericData *data;
    unsigned int time;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    if (data->bRecursive == 0) {
        /* If compute unreliability flag is not set... */
        if (data->bComputeUnreliability == 0) {
            /* For each time instant to be processed... */
            while (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNGenericSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += data->numCores;
            }
        }
        else {
            /* For each time instant to be processed... */
            while (time < data->numTimes) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNGenericFailStepS1d(data, time);
                /* Increment current time instant */
                time += data->numCores;
            }
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Recursively compute reliability of KooN RBD at current time instant */
            rbdKooNRecursionS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
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
HIDDEN void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;
    unsigned int time;

    /* Retrieve identical KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
    }
    else {
        /* For each time instant to be processed... */
        while (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
            /* Increment current time instant */
            time += data->numCores;
        }
    }

    return NULL;
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdKooNGenericSuccessStepS1d
 *
 * Generic KooN RBD Step function from working components
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the working components
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNGenericSuccessStepS1d(struct rbdKooNGenericData *data, unsigned int time)
{
    double s1dStep;
    double s1dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 0 */
    s1dRes = 0.0;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step reliability to 1 */
            s1dStep = 1.0;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Does the component belong to the working components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step reliability for reliability of current component */
                    s1dStep *= data->reliabilities[(jj * data->numTimes) + time];
                    /* Advance to next working component in combination */
                    if (++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step reliability for unreliability of current component */
                    s1dStep *= (1.0 - data->reliabilities[(jj * data->numTimes) + time]);
                }
            }

            /* Perform partial sum for computation of KooN reliability */
            s1dRes += s1dStep;
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdKooNGenericFailStepS1d
 *
 * Generic KooN RBD Step function from failed components
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the failed components
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNGenericFailStepS1d(struct rbdKooNGenericData *data, unsigned int time)
{
    double s1dStep;
    double s1dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 1 */
    s1dRes = 1.0;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step unreliability to 1 */
            s1dStep = 1.0;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Does the component belong to the failed components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step unreliability for unreliability of current component */
                    s1dStep *= (1.0 - data->reliabilities[(jj * data->numTimes) + time]);
                    /* Advance to next failed component in combination */
                    if(++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step unreliability for reliability of current component */
                    s1dStep *= data->reliabilities[(jj * data->numTimes) + time];
                }
            }

            /* Perform partial difference for computation of KooN reliability */
            s1dRes -= s1dStep;
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdKooNRecursionS1d
 *
 * Compute KooN RBD though Recursive method
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the reliability of KooN RBD system through recursion
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNRecursionS1d(struct rbdKooNGenericData *data, unsigned int time)
{
    double s1dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    s1dRes = rbdKooNRecursiveStepS1d(data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdKooNIdenticalSuccessStepS1d
 *
 * Identical KooN RBD Step function from working components
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the working components
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNIdenticalSuccessStepS1d(struct rbdKooNIdenticalData *data, unsigned int time)
{
    double s1dR;
    double s1dTmp1, s1dTmp2;
    double s1dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    s1dR = data->reliabilities[time];
    /* Initialize reliability to 0 */
    s1dRes = 0.0;
    /* Compute product between reliability and unreliability */
    s1dTmp2 = s1dR * (1.0 - s1dR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        s1dTmp1 = (double)data->nCi[ii];
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            s1dTmp1 *= s1dTmp2;
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            s1dTmp1 *= s1dR;
        }
        /* Add reliability of current iteration */
        s1dRes += s1dTmp1;
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdKooNIdenticalFailStepS1d
 *
 * Identical KooN RBD Step function from failed components
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the failed components
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNIdenticalFailStepS1d(struct rbdKooNIdenticalData *data, unsigned int time)
{
    double s1dU;
    double s1dTmp1, s1dTmp2;
    double s1dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    s1dTmp2 = data->reliabilities[time];
    /* Compute unreliability */
    s1dU = 1.0 - s1dTmp2;
    /* Initialize reliability to 1 */
    s1dRes = 1.0;
    /* Compute product between reliability and unreliability */
    s1dTmp2 = s1dTmp2 * s1dU;

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step unreliability to nCi */
        s1dTmp1 = (double)data->nCi[ii];
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            s1dTmp1 *= s1dTmp2;
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            s1dTmp1 *= s1dU;
        }
        /* Subtract unreliability of current iteration */
        s1dRes -= s1dTmp1;
    }

    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}


/**
 * rbdKooNRecursiveStepS1d
 *
 * Recursive KooN RBD Step function
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (double):
 *  Computed reliability
 */
static double rbdKooNRecursiveStepS1d(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k)
{
    double s1dRes;

    /* Recursively compute the Reliability */
    --n;
    s1dRes = data->reliabilities[(n * data->numTimes) + time];
    if ((k-1) > 0) {
        s1dRes *= rbdKooNRecursiveStepS1d(data, time, n, k-1);
    }
    if (k <= n) {
        s1dRes += (1.0 - data->reliabilities[(n * data->numTimes) + time]) * rbdKooNRecursiveStepS1d(data, time, n, k);
    }
    return s1dRes;
}
