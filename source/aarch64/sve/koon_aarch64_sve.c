/*
 *  Component: koon_aarch64_sve.c
 *  KooN (K-out-of-N) RBD management - Optimized using AArch64 SVE instruction set
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


#include "../../generic/rbd_internal_generic.h"

#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_aarch64.h"
#include "../koon_aarch64.h"
#include "../../generic/combinations.h"


#if !defined(COMPILER_VS)
static svfloat64_t rbdKooNGenericShannonStepVNdSve(svbool_t pg, struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k);
static double *rbdKooNBddSve(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps);
#endif /* !defined(COMPILER_VS) */


/**
 * rbdKooNFillWorkerSve
 *
 * Fill output Reliability with fixed value Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: Fill KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdKooNFillWorkerSve(struct rbdKooNFillData *data)
{
#if !defined(COMPILER_VS)
    svbool_t pg;
    unsigned int time;
    unsigned int vectorSize;
    svfloat64_t vNdR;

    /* Define vector (Nd) with provided value */
    vNdR = svdup_f64(data->value);

    /* Set first time instant to be processed */
    time = 0;
    /* For each time instant (blocks of N time instants)... */
    while (time < data->numTimes) {
        pg = svwhilelt_b64(time, data->numTimes);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Fill output Reliability array with fixed value */
        svst1(pg, &data->output[time], vNdR);
        /* Increment current time instant */
        time += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

/**
 * rbdKooNGenericShannonWorkerSve
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition using
 *  AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdKooNGenericShannonWorkerSve(struct rbdKooNGenericShannonData *data)
{
#if !defined(COMPILER_VS)
    unsigned int batchSize;
    unsigned int time;
    unsigned int vectorSize;
    unsigned int timeEnd;
    svbool_t pg;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivision(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimum(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        pg = svwhilelt_b64(time, timeEnd);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + vectorSize);
        prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNGenericShannonVNdSve(pg, data, time);
        /* Increment current time instant */
        time += vectorSize;
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

/**
 * rbdKooNBddWorkerSve
 *
 * Generic KooN RBD Worker function exploiting BDD Evaluation with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting BDD Evaluation using
 *  AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdKooNBddWorkerSve(struct rbdKooNBddData *data)
{
#if !defined(COMPILER_VS)
    unsigned int time;
    unsigned int steps;
    unsigned char *computedPool;
    double *reliability;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(data->batchIdx);

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
        if (rbdKooNBddSve(data, data->bddmgr->root, time, steps) == NULL) {
            return NULL;
        }
        /* Copy reliability computed with BDD to output */
        reliability = bddGetValues(data->bddmgr, data->bddmgr->root, data->batchIdx);
        memcpy(&data->output[time], reliability, steps * sizeof(double));
        /* Increment current time batch */
        time += (data->numCores * BDD_WINDOW_SIZE);
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerSve
 *
 * Identical KooN RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *  using the previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdKooNIdenticalWorkerSve(struct rbdKooNIdenticalData *data)
{
#if !defined(COMPILER_VS)
    unsigned int batchSize;
    unsigned int time;
    unsigned int vectorSize;
    unsigned int timeEnd;
    svbool_t pg;

    /* Set CPU affinity */
    setAArch64ThreadAffinitySve(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivision(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimum(time + batchSize, data->numTimes);

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < timeEnd) {
            pg = svwhilelt_b64(time, timeEnd);
            vectorSize = svcntp_b64(svptrue_b64(), pg);
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + vectorSize);
            prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdSve(pg, data, time);
            /* Increment current time instant */
            time += vectorSize;
        }
    }
    else {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < timeEnd) {
            pg = svwhilelt_b64(time, timeEnd);
            vectorSize = svcntp_b64(svptrue_b64(), pg);
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + vectorSize);
            prefetchWrite(data->output, 1, data->numTimes, time + vectorSize);
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdSve(pg, data, time);
            /* Increment current time instant */
            time += vectorSize;
        }
    }
#endif /* !defined(COMPILER_VS) */

    return NULL;
}

#if !defined(COMPILER_VS)
/**
 * rbdKooNGenericShannonVNdSve
 *
 * Compute KooN RBD through Shannon Decomposition method with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the reliability of KooN RBD system through Shannon Decomposition
 *  exploiting AArch64 SVE
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNGenericShannonVNdSve(svbool_t pg, struct rbdKooNGenericShannonData *data, unsigned int time)
{
    svfloat64_t vNdRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    vNdRes = rbdKooNGenericShannonStepVNdSve(pg, data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdKooNBddStepV2dNeon
 *
 * Compute the Reliability value for a BDD Node with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      double *r
 *      double *h
 *      double *l
 *
 * Output:
 *      double *o
 *
 * Description:
 *  This function computes the reliability value of KooN RBD system through BDD Evaluation
 *  using AArch64 SVE
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      r: reliability value of BDD Variable under analysis
 *      h: reliability value of BDD High Node, i.e., the BDD Variable is working
 *      l: reliability value of BDD Low Node, i.e., the BDD Variable is failed
 *      o: output reliability value
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNBddStepVNdSve(svbool_t pg, double *r, double *h, double *l, double *o)
{
    svfloat64_t vNdR;
    svfloat64_t vNdH;
    svfloat64_t vNdL;
    svfloat64_t vNdRes;

    /* Compute the reliability of the BDD Node NODE = R * H + (1 - R) * L */
    vNdR = svld1(pg, r);
    vNdL = svld1(pg, l);
    vNdRes = svmls_f64_x(pg, vNdL, vNdR, vNdL);
    vNdH = svld1(pg, h);
    vNdRes = svmla_f64_x(pg, vNdRes, vNdR, vNdH);
    svst1(pg, o, capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdKooNIdenticalSuccessStepVNdSve
 *
 * Identical KooN RBD Step function from working components with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the working components
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNIdenticalSuccessStepVNdSve(svbool_t pg, struct rbdKooNIdenticalData *data, unsigned int time)
{
    svfloat64_t vNdR;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    vNdR = svld1(pg, &data->reliabilities[time]);
    /* Initialize reliability to 0 */
    vNdRes = svdup_f64(0.0);
    /* Compute product between reliability and unreliability */
    vNdTmp2 = svmls_f64_x(pg, vNdR, vNdR, vNdR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = svdup_f64((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            vNdTmp1 = svmul_f64_x(pg, vNdTmp1, vNdTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            vNdTmp1 = svmul_f64_x(pg, vNdTmp1, vNdR);
        }
        /* Add reliability of current iteration */
        vNdRes = svadd_f64_x(pg, vNdRes, vNdTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdKooNIdenticalFailStepVNdSve
 *
 * Identical KooN RBD Step function from failed components with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting AArch64 SVE.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the failed components
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNIdenticalFailStepVNdSve(svbool_t pg, struct rbdKooNIdenticalData *data, unsigned int time)
{
    svfloat64_t vNdU;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    /* Compute unreliability */
    vNdTmp2 = svld1(pg, &data->reliabilities[time]);
    vNdU = svsub_f64_x(pg, svdup_f64(1.0), vNdTmp2);
    /* Initialize reliability to 1 */
    vNdRes = svdup_f64(1.0);
    /* Compute product between reliability and unreliability */
    vNdTmp2 = svmul_f64_x(pg, vNdTmp2, vNdU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = svdup_f64((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            vNdTmp1 = svmul_f64_x(pg, vNdTmp1, vNdTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            vNdTmp1 = svmul_f64_x(pg, vNdTmp1, vNdU);
        }
        /* Subtract unreliability of current iteration */
        vNdRes = svsub_f64_x(pg, vNdRes, vNdTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}


/**
 * rbdKooNGenericShannonStepVNdSve
 *
 * Recursive KooN RBD Shannon Decomposition function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function through Shannon Decomposition method
 *  exploiting AArch64 SVE.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (float64x2_t):
 *  Computed reliability
 */
static FUNCTION_TARGET("+sve") svfloat64_t rbdKooNGenericShannonStepVNdSve(svbool_t pg, struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k)
{
    unsigned char best;
    unsigned char offset;
    unsigned char idx;
    unsigned char ii, jj;
    double *s1dPtr;
    svfloat64_t vNdR;
    svfloat64_t vNdRes;
    svfloat64_t vNdTmpRec;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdStepTmp1, vNdStepTmp2;
    int nextCombs;
    unsigned long cntd;

    cntd = svcntd();

    if (k == n) {
        /* Compute the Reliability as Series block */
        vNdRes = svdup_f64(1.0);
        while (n > 0) {
            vNdTmp1 = svld1(pg, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdRes = svmul_f64_x(pg, vNdRes, vNdTmp1);
        }
        return vNdRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        vNdRes = svdup_f64(1.0);
        while (n > 0) {
            vNdTmp1 = svld1(pg, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdRes = svmls_f64_x(pg, vNdRes, vNdTmp1, vNdRes);
        }
        return svsub_f64_x(pg, svdup_f64(1.0), vNdRes);
    }

    best = (unsigned char)minimum(((int)k-1), ((int)n-(int)k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        vNdTmp1 = svdup_f64(1.0);
        vNdTmp2 = svdup_f64(1.0);
        s1dPtr = &data->recur.s1dR[offset * cntd];
        for (idx = 0; idx < best; idx++) {
            vNdR = svld1(pg, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdTmp1 = svmul_f64_x(pg, vNdTmp1, vNdR);
            vNdTmp2 = svmls_f64_x(pg, vNdTmp2, vNdR, vNdTmp2);
            svst1(pg, &s1dPtr[idx * cntd], vNdR);
        }
        vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k-best);
        vNdRes = svmul_f64_x(pg, vNdTmp1, vNdTmpRec);
        vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k);
        vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp2, vNdTmpRec);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            vNdTmp1 = svdup_f64(0.0);
            vNdTmp2 = svdup_f64(0.0);
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = svdup_f64(1.0);
                vNdStepTmp2 = svdup_f64(1.0);
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    vNdR = svld1(pg, &s1dPtr[jj * cntd]);
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = svmls_f64_x(pg, vNdStepTmp1, vNdR, vNdStepTmp1);
                        vNdStepTmp2 = svmul_f64_x(pg, vNdStepTmp2, vNdR);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = svmul_f64_x(pg, vNdStepTmp1, vNdR);
                        vNdStepTmp2 = svmls_f64_x(pg, vNdStepTmp2, vNdR, vNdStepTmp2);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdR = svld1(pg, &s1dPtr[jj * cntd]);
                    vNdStepTmp1 = svmul_f64_x(pg, vNdStepTmp1, vNdR);
                    vNdStepTmp2 = svmls_f64_x(pg, vNdStepTmp2, vNdR, vNdStepTmp2);
                    ++jj;
                }
                vNdTmp1 = svadd_f64_x(pg, vNdTmp1, vNdStepTmp1);
                vNdTmp2 = svadd_f64_x(pg, vNdTmp2, vNdStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k-best+idx);
            vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
            vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k-idx);
            vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp2, vNdTmpRec);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            vNdTmp1 = svdup_f64(0.0);
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = svdup_f64(1.0);
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    vNdR = svld1(pg, &s1dPtr[jj * cntd]);
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = svmls_f64_x(pg, vNdStepTmp1, vNdR, vNdStepTmp1);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = svmul_f64_x(pg, vNdStepTmp1, vNdR);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdR = svld1(pg, &s1dPtr[jj * cntd]);
                    vNdStepTmp1 = svmul_f64_x(pg, vNdStepTmp1, vNdR);
                    ++jj;
                }
                vNdTmp1 = svadd_f64_x(pg, vNdTmp1, vNdStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k-best+idx);
            vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
        }

        return vNdRes;
    }

    /* Recursively compute the Reliability */
    vNdTmp1 = svld1(pg, &data->reliabilities[(--n * data->numTimes) + time]);
    vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k-1);
    vNdRes = svmul_f64_x(pg, vNdTmp1, vNdTmpRec);
    vNdTmp1 = svsub_f64_x(pg, svdup_f64(1.0), vNdTmp1);
    vNdTmpRec = rbdKooNGenericShannonStepVNdSve(pg, data, time, n, k);
    vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
    return vNdRes;
}

/**
 * rbdKooNBddSve
 *
 * Recursively compute the Reliability curve of a BDD Node with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This recursive function computes the Reliability curve of the provided BDD Node
 *  using AArch64 SVE instruction set
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
static FUNCTION_TARGET("+sve") double *rbdKooNBddSve(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps)
{
    double *nodeValues;
    unsigned char *computedNodes;
    double *high;
    double *low;
    double *rel;
    struct bddnode *node;
    unsigned int tIdx;
    unsigned int vectorSize;
    svbool_t pg;

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
    high = rbdKooNBddSve(data, node->high, timeStart, numSteps);
    if (high == NULL) {
        return NULL;
    }
    /* Recursively evaluate the reliability of the low part of the current BDD Node */
    low  = rbdKooNBddSve(data, node->low, timeStart, numSteps);
    if (low == NULL) {
        return NULL;
    }

    /* Retrieve the reliability curve associated with the variable */
    rel = &data->bddmgr->vars[node->var].reliability[timeStart];

    /* For each time instant to be evaluated (blocks of N time instants)... */
    tIdx = 0;
    while (tIdx < numSteps) {
        pg = svwhilelt_b64(tIdx, numSteps);
        vectorSize = svcntp_b64(svptrue_b64(), pg);
        /* Compute the (cached) reliability curve associated with the current BDD Node */
        rbdKooNBddStepVNdSve(pg, &rel[tIdx], &high[tIdx], &low[tIdx], &nodeValues[tIdx]);
        /* Increment current time instant */
        tIdx += vectorSize;
    }

    /* Set the BDD Node as already evaluated */
    computedNodes[nodeIdx] = 1;

    return nodeValues;
}
#endif /* !defined(COMPILER_VS) */


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
