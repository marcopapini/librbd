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


static double rbdKooNGenericShannonStepS1d(struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k);
static double *rbdKooNBddNoarch(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps);


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
 * rbdKooNGenericShannonWorker
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN for Shannon Decomposition RBD data.
 *                      It is provided as a void * to be compliant with the SMP computation of the
 *                      Generic KooN for Shannon Decomposition RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericShannonWorker(void *arg)
{
    struct rbdKooNGenericShannonData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNGenericShannonData *)arg;

    return rbdKooNGenericShannonWorkerNoarch(data);
}

/**
 * rbdKooNBddWorker
 *
 * Generic KooN RBD Worker function exploiting BDD Evaluation
 *
 * Input:
 *      void *arg
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting BDD Evaluation.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      arg: this parameter shall be the pointer to a generic KooN for BDD Evaluation RBD data.
 *                      It is provided as a void * to be compliant with the SMP computation of the
 *                      Generic KooN for BDD Evaluation RBD
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNBddWorker(void *arg)
{
    struct rbdKooNBddData *data;

    /* Retrieve generic KooN RBD data */
    data = (struct rbdKooNBddData *)arg;

    return rbdKooNBddWorkerNoarch(data);
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

    /* For each time instant... */
    for (time = 0; time < data->numTimes; ++time) {
        /* Fill output Reliability array with fixed value */
        data->output[time] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNGenericShannonWorkerNoarch
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition with platform-independent instruction sets
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition using
 *  platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericShannonWorkerNoarch(struct rbdKooNGenericShannonData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx;

    /* For each time instant to be processed... */
    while (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNGenericShannonS1d(data, time);
        /* Increment current time instant */
        time += data->numCores;
    }

    return NULL;
}

/**
 * rbdKooNBddWorkerNoarch
 *
 * Generic KooN RBD Worker function exploiting BDD Evaluation with platform-independent instruction sets
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting BDD Evaluation using
 *  platform-independent instruction sets.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNBddWorkerNoarch(struct rbdKooNBddData *data)
{
    unsigned int time;
    unsigned int steps;
    unsigned char *computedPool;
    double *reliability;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * BDD_WINDOW_SIZE;

    /* Retrieve the current computed pool */
    computedPool = bddGetComputed(data->bddmgr, data->batchIdx);

    /* For each time batch to be processed... */
    while (time < data->numTimes) {
        /* Compute the number of time instants processed during current batch */
        steps = u32min(data->numTimes - time, BDD_WINDOW_SIZE);
        /* Reset that all BDD Nodes (excluding the terminal nodes) are already evaluated */
        memset(&computedPool[BDD_NUM_TERMINAL], 0,
               (data->bddmgr->numNodes - BDD_NUM_TERMINAL) * sizeof(unsigned char));
        /* Recursively compute reliability of KooN RBD at current batch */
        if (rbdKooNBddNoarch(data, data->bddmgr->root, time, steps) == NULL) {
            return NULL;
        }
        /* Copy reliability computed with BDD to output */
        reliability = bddGetValues(data->bddmgr, data->bddmgr->root, data->batchIdx);
        memcpy(&data->output[time], reliability, steps * sizeof(double));
        /* Increment current time batch */
        time += (data->numCores * BDD_WINDOW_SIZE);
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
 * rbdKooNGenericShannonS1d
 *
 * Compute KooN RBD through Shannon Decomposition method
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the reliability of KooN RBD system through Shannon Decomposition
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNGenericShannonS1d(struct rbdKooNGenericShannonData *data, unsigned int time)
{
    double s1dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    s1dRes = rbdKooNGenericShannonStepS1d(data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    data->output[time] = capReliabilityS1d(s1dRes);
}

/**
 * rbdKooNBddStepS1d
 *
 * Compute the Reliability value for a BDD Node
 *
 * Input:
 *      double *r
 *      double *h
 *      double *l
 *
 * Output:
 *      double *o
 *
 * Description:
 *  This function computes the reliability value of KooN RBD system through BDD Evaluation
 *
 * Parameters:
 *      r: reliability value of BDD Variable under analysis
 *      h: reliability value of BDD High Node, i.e., the BDD Variable is working
 *      l: reliability value of BDD Low Node, i.e., the BDD Variable is failed
 *      o: output reliability value
 *
 * Return:
 *  None
 */
HIDDEN void rbdKooNBddStepS1d(double *r, double *h, double *l, double *o)
{
    *o = capReliabilityS1d(((*r) * (*h)) + ((1.0 - (*r)) * (*l)));
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
 * rbdKooNGenericShannonStepS1d
 *
 * Recursive KooN RBD Shannon Decomposition function
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function through Shannon Decomposition method.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (double):
 *  Computed reliability
 */
static double rbdKooNGenericShannonStepS1d(struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k)
{
    unsigned char best;
    unsigned char offset;
    unsigned char idx;
    unsigned char ii, jj;
    double *s1dR;
    double s1dRes;
    double s1dTmp1, s1dTmp2;
    double s1dStepTmp1, s1dStepTmp2;
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

    best = (unsigned char)minimum(((int)k-1), ((int)n-(int)k));
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
        s1dRes = s1dTmp1 * rbdKooNGenericShannonStepS1d(data, time, n, k-best);
        s1dRes += s1dTmp2 * rbdKooNGenericShannonStepS1d(data, time, n, k);
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
            s1dRes += s1dTmp1 * rbdKooNGenericShannonStepS1d(data, time, n, k-best+idx);
            s1dRes += s1dTmp2 * rbdKooNGenericShannonStepS1d(data, time, n, k-idx);
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
            s1dRes += s1dTmp1 * rbdKooNGenericShannonStepS1d(data, time, n, k-best+idx);
        }

        return s1dRes;
    }

    /* Recursively compute the Reliability */
    s1dTmp1 = data->reliabilities[(--n * data->numTimes) + time];
    s1dRes = s1dTmp1 * rbdKooNGenericShannonStepS1d(data, time, n, k-1);
    s1dRes += (1.0 - s1dTmp1) * rbdKooNGenericShannonStepS1d(data, time, n, k);
    return s1dRes;
}

/**
 * rbdKooNBddNoarch
 *
 * Recursively compute the Reliability curve of a BDD Node
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This recursive function computes the Reliability curve of the provided BDD Node
 *
 * Parameters:
 *      bddmgr: The BDD Manager
 *      nodeIdx: The BDD Node identified
 *      timeStart: The first time instant for which the reliability curve is computed
 *      numSteps: The number of time instants for which the reliability curve is computed
 *
 * Return (double *):
 *  The cached reliability curve of this BDD Node is successful, NULL otherwise
 */
static double *rbdKooNBddNoarch(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps)
{
    double *nodeValues;
    unsigned char *computedNodes;
    double *high;
    double *low;
    double *rel;
    struct bddnode *node;
    unsigned int tIdx;

    /* Retrieve the values array associated with the current BDD Node */
    nodeValues = bddGetValues(data->bddmgr, nodeIdx, data->batchIdx);

    /* If the BDD Node has been already evaluated, then immediately return its reliability */
    computedNodes = bddGetComputed(data->bddmgr, data->batchIdx);
    if (computedNodes[nodeIdx]) {
        return nodeValues;
    }

    /* Retrieve the BDD Node */
    node = &data->bddmgr->nodes[nodeIdx];

    /* Recursively evaluate the reliability of the high part of the current BDD Node */
    high = rbdKooNBddNoarch(data, node->high, timeStart, numSteps);
    if (high == NULL) {
        return NULL;
    }
    /* Recursively evaluate the reliability of the low part of the current BDD Node */
    low  = rbdKooNBddNoarch(data, node->low, timeStart, numSteps);
    if (low == NULL) {
        return NULL;
    }

    /* Retrieve the reliability curve associated with the variable */
    rel = &data->bddmgr->vars[node->var].reliability[timeStart];

    /* For each time instant to be evaluated... */
    for (tIdx = 0; tIdx < numSteps; ++tIdx) {
        /* Compute the (cached) reliability curve associated with the current BDD Node */
        rbdKooNBddStepS1d(&rel[tIdx], &high[tIdx], &low[tIdx], &nodeValues[tIdx]);
    }

    /* Set the BDD Node as already evaluated */
    computedNodes[nodeIdx] = 1;

    return nodeValues;
}
