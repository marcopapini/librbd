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


static svfloat64_t rbdKooNRecursiveStepVNdSve(svbool_t pg, struct rbdKooNGenericData *data, unsigned int time, short n, short k);


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
    svbool_t pg;
    unsigned int time;
    unsigned long cntd;
    svfloat64_t vNdR;

    cntd = svcntd();

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Define vector (Nd) with provided value */
    vNdR = svdup_f64(data->value);

    /* For each time instant (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Fill output Reliability array with fixed value */
        svst1(pg, &data->output[time], vNdR);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerSve
 *
 * Generic KooN RBD Worker function with AArch64 SVE instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting AArch64 SVE instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("+sve") void *rbdKooNGenericWorkerSve(struct rbdKooNGenericData *data)
{
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * cntd));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionVNdSve(data, time);
        /* Increment current time instant */
        time += (data->numCores * cntd);
    }

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
    unsigned int time;
    unsigned long cntd;

    cntd = svcntd();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * cntd;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * cntd));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdSve(data, time);
            /* Increment current time instant */
            time += (data->numCores * cntd);
        }
    }
    else {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * cntd));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * cntd));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdSve(data, time);
            /* Increment current time instant */
            time += (data->numCores * cntd);
        }
    }

    return NULL;
}

/**
 * rbdKooNRecursionVNdSve
 *
 * Compute KooN RBD through Recursive method with AArch64 SVE
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
 *  exploiting AArch64 SVE
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNRecursionVNdSve(struct rbdKooNGenericData *data, unsigned int time)
{
    svbool_t pg;
    svfloat64_t vNdRes;

    pg = svwhilelt_b64(time, data->numTimes);

    /* Recursively compute reliability of KooN RBD at current time instant */
    vNdRes = rbdKooNRecursiveStepVNdSve(pg, data, time, (short)data->numComponents, (short)data->minComponents);
    /* Cap the computed reliability and set it into output array */
    svst1(pg, &data->output[time], capReliabilityVNdSve(pg, vNdRes));
}

/**
 * rbdKooNIdenticalSuccessStepVNdSve
 *
 * Identical KooN RBD Step function from working components with AArch64 SVE
 *
 * Input:
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
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNIdenticalSuccessStepVNdSve(struct rbdKooNIdenticalData *data, unsigned int time)
{
    svbool_t pg;
    svfloat64_t vNdR;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    pg = svwhilelt_b64(time, data->numTimes);

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
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("+sve") void rbdKooNIdenticalFailStepVNdSve(struct rbdKooNIdenticalData *data, unsigned int time)
{
    svbool_t pg;
    svfloat64_t vNdU;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    pg = svwhilelt_b64(time, data->numTimes);

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
 * rbdKooNRecursiveStepVNdSve
 *
 * Recursive KooN RBD Step function with AArch64 SVE
 *
 * Input:
 *      svbool_t pg
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *      short n
 *      short k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function exploiting AArch64 SVE.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      pg: SVE Predicate for lane access
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (float64x2_t):
 *  Computed reliability
 */
static FUNCTION_TARGET("+sve") svfloat64_t rbdKooNRecursiveStepVNdSve(svbool_t pg, struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    double *s1dPtr; // TODO: questo va corretto
    svfloat64_t vNdR;
    svfloat64_t vNdRes;
    svfloat64_t vNdTmpRec;
    svfloat64_t vNdTmp1, vNdTmp2;
    svfloat64_t vNdStepTmp1, vNdStepTmp2;
    int idx;
    int offset;
    int ii, jj;
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

    best = (short)minimum((int)(k-1), (int)(n-k));
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
        vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k-best);
        vNdRes = svmul_f64_x(pg, vNdTmp1, vNdTmpRec);
        vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k);
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
            vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k-best+idx);
            vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
            vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k-idx);
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
            vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k-best+idx);
            vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
        }

        return vNdRes;
    }

    /* Recursively compute the Reliability */
    vNdTmp1 = svld1(pg, &data->reliabilities[(--n * data->numTimes) + time]);
    vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k-1);
    vNdRes = svmul_f64_x(pg, vNdTmp1, vNdTmpRec);
    vNdTmp1 = svsub_f64_x(pg, svdup_f64(1.0), vNdTmp1);
    vNdTmpRec = rbdKooNRecursiveStepVNdSve(pg, data, time, n, k);
    vNdRes = svmla_f64_x(pg, vNdRes, vNdTmp1, vNdTmpRec);
    return vNdRes;
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
