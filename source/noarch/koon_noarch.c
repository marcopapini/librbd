/*
 *  Component: koon_noarch.c
 *  KooN (K-out-of-N) RBD management - Platform-independent implementation
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

#include "../koon.h"
#include "../generic/combinations.h"

#include <stdlib.h>
#include <limits.h>


static double rbdKooNRecursiveStepS1d(struct rbdKooNGenericData *data, unsigned int time, short n, short k);


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
 *      arg: this parameter shall be the pointer to a fill KooN data. It is provided as a void *
 *                      to be compliant with the SMP of the Fill KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNFillWorker(void *arg)
{
    struct rbdKooNFillData *data;

    /* Retrieve fill Output data structure */
    data = (struct rbdKooNFillData *)arg;

    return rbdKooNFillWorkerNoarch(data);
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
 *      arg: this parameter shall be the pointer to a generic KooN RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Generic KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorker(void *arg)
{
    struct rbdKooNGenericData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericData *)arg;

    return rbdKooNGenericWorkerNoarch(data);
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
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to an identical KooN RBD data. It is provided as a void *
 *                      to be compliant with the SMP computation of the Identical KooN RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNIdenticalWorker(void *arg)
{
    struct rbdKooNIdenticalData *data;

    /* Retrieve identical KooN RBD data */
    data = (struct rbdKooNIdenticalData *)arg;

    return rbdKooNIdenticalWorkerNoarch(data);
}
#endif /* defined(ARCH_UNKNOWN) || CPU_ENABLE_SIMD == 0 */

/**
 * rbdKooNFillWorkerNoarch
 *
 * Fill output Reliability with fixed value Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker with platform-independent instruction sets.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: Fill KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNFillWorkerNoarch(struct rbdKooNFillData *data)
{
    unsigned int time;

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
 * rbdKooNGenericWorkerNoarch
 *
 * Generic KooN RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorkerNoarch(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerNoarch
 *
 * Identical KooN RBD Worker function with platform-independent instruction sets
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker with platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *  using the previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNIdenticalWorkerNoarch(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

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
    s1dRes = rbdKooNRecursiveStepS1d(data, time, (short)data->numComponents, (short)data->minComponents);
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
 *      short n
 *      short k
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
static double rbdKooNRecursiveStepS1d(struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    double *s1dR;
    double s1dRes;
    double s1dTmp1, s1dTmp2;
    double s1dStepTmp1, s1dStepTmp2;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;

    if (k == n) {
        /* Compute the Reliability as Series block */
        s1dRes = 1.0;
        while (n > 0) {
            s1dRes *= data->reliabilities[(--n * data->numTimes) + time];
        }
        return s1dRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        s1dRes = 1.0;
        while (n > 0) {
            s1dRes *= (1.0 - data->reliabilities[(--n * data->numTimes) + time]);
        }
        return 1.0 - s1dRes;
    }

    best = (short)minimum((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        s1dTmp1 = 1.0;
        s1dTmp2 = 1.0;
        s1dR = &data->recur.s1dR[offset];
        for (idx = 0; idx < best; idx++) {
            s1dR[idx] = data->reliabilities[(--n * data->numTimes) + time];
            s1dTmp1 *= s1dR[idx];
            s1dTmp2 *= (1.0 - s1dR[idx]);
        }
        s1dRes = s1dTmp1 * rbdKooNRecursiveStepS1d(data, time, n, k-best);
        s1dRes += s1dTmp2 * rbdKooNRecursiveStepS1d(data, time, n, k);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            s1dTmp1 = 0.0;
            s1dTmp2 = 0.0;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                s1dStepTmp1 = 1.0;
                s1dStepTmp2 = 1.0;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        s1dStepTmp1 *= (1.0 - s1dR[jj]);
                        s1dStepTmp2 *= s1dR[jj];
                        ++ii;
                    }
                    else {
                        s1dStepTmp1 *= s1dR[jj];
                        s1dStepTmp2 *= (1.0 - s1dR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    s1dStepTmp1 *= s1dR[jj];
                    s1dStepTmp2 *= (1.0 - s1dR[jj]);
                    ++jj;
                }
                s1dTmp1 += s1dStepTmp1;
                s1dTmp2 += s1dStepTmp2;
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            s1dRes += s1dTmp1 * rbdKooNRecursiveStepS1d(data, time, n, k-best+idx);
            s1dRes += s1dTmp2 * rbdKooNRecursiveStepS1d(data, time, n, k-idx);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            s1dTmp1 = 0.0;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                s1dStepTmp1 = 1.0;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        s1dStepTmp1 *= (1.0 - s1dR[jj]);
                        ++ii;
                    }
                    else {
                        s1dStepTmp1 *= s1dR[jj];
                    }
                    ++jj;
                }
                while (jj < best) {
                    s1dStepTmp1 *= s1dR[jj];
                    ++jj;
                }
                s1dTmp1 += s1dStepTmp1;
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            s1dRes += s1dTmp1 * rbdKooNRecursiveStepS1d(data, time, n, k-best+idx);
        }

        return s1dRes;
    }

    /* Recursively compute the Reliability */
    s1dTmp1 = data->reliabilities[(--n * data->numTimes) + time];
    s1dRes = s1dTmp1 * rbdKooNRecursiveStepS1d(data, time, n, k-1);
    s1dRes += (1.0 - s1dTmp1) * rbdKooNRecursiveStepS1d(data, time, n, k);
    return s1dRes;
}
