/*
 *  Component: koon_amd64_avx512f.c
 *  KooN (K-out-of-N) RBD management - Optimized using amd64 AVX512F instruction set
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

#if defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_amd64.h"
#include "../koon_amd64.h"
#include "../../x86/koon_x86.h"
#include "../../generic/combinations.h"


static __m512d rbdKooNGenericShannonStepVNdAvx512f(__mmask8 mask, struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k);
static double *rbdKooNBddAvx512f(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps);


/**
 * rbdKooNFillWorkerAvx512f
 *
 * Fill output Reliability with fixed value Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker amd64 AVX512F instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: Fill KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdKooNFillWorkerAvx512f(struct rbdKooNFillData *data)
{
    unsigned int time;
    __m512d m512d;
    __mmask8 mask;

    /* Define vector (8d) with provided value */
    m512d = _mm512_set1_pd(data->value);

    /* For each time instant (blocks of 8 time instants)... */
    for (time = 0; (time + V8D) <= data->numTimes; time += V8D) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + V8D);
        /* Fill output Reliability array with fixed value */
        _mm512_storeu_pd(&data->output[time], m512d);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Fill output Reliability array with fixed value */
        _mm512_mask_storeu_pd(&data->output[time], mask, m512d);
    }

    return NULL;
}

/**
 * rbdKooNGenericShannonWorkerAvx512f
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition using
 *  amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdKooNGenericShannonWorkerAvx512f(struct rbdKooNGenericShannonData *data)
{
    unsigned int time;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNGenericShannonVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNGenericShannonVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdKooNBddWorkerAvx512f
 *
 * Generic KooN RBD Worker function exploiting BDD Evaluation with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting BDD Evaluation using
 *  amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNBddWorkerAvx512f(struct rbdKooNBddData *data)
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
        if (rbdKooNBddAvx512f(data, data->bddmgr->root, time, steps) == NULL) {
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
 * rbdKooNIdenticalWorkerAvx512f
 *
 * Identical KooN RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *  using the previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdKooNIdenticalWorkerAvx512f(struct rbdKooNIdenticalData *data)
{
    unsigned int time;
    unsigned int alignSteps;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* Are there at least 8 - 1 time instants to process? */
        if ((time + V8D) < data->numTimes) {
            /* Align, if possible, to vector size */
            if (((uintptr_t)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
                /* Compute the number of doubles to align to vector size */
                alignSteps = ((uintptr_t)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) / sizeof(double);
                alignSteps = (V8D - alignSteps) & (V8D - 1);
                if (alignSteps > 0) {
                    /* Compute mask for the management of the head */
                    mask = (__mmask8)_cvtu32_mask16((1U << alignSteps) - 1);
                    /* Compute reliability of KooN RBD at current time instant from working components */
                    rbdKooNIdenticalSuccessStepVNdAvx512f(mask, data, time);
                    /* Increment current time instant */
                    time += alignSteps;
                }
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdAvx512f((__mmask8)0xFFU, data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Is (at least) 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute mask for the management of the tail */
            mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdAvx512f(mask, data, time);
        }
    }
    else {
        /* Are there at least 8 - 1 time instants to process? */
        if ((time + V8D) < data->numTimes) {
            /* Align, if possible, to vector size */
            if (((uintptr_t)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
                /* Compute the number of doubles to align to vector size */
                alignSteps = ((uintptr_t)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) / sizeof(double);
                alignSteps = (V8D - alignSteps) & (V8D - 1);
                if (alignSteps > 0) {
                    /* Compute mask for the management of the head */
                    mask = (__mmask8)_cvtu32_mask16((1U << alignSteps) - 1);
                    /* Compute reliability of KooN RBD at current time instant from failed components */
                    rbdKooNIdenticalFailStepVNdAvx512f(mask, data, time);
                    /* Increment current time instant */
                    time += alignSteps;
                }
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdAvx512f((__mmask8)0xFFU, data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Is (at least) 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute mask for the management of the tail */
            mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdAvx512f(mask, data, time);
            /* Increment current time instant */
            time += V4D;
        }
    }

    return NULL;
}

/**
 * rbdKooNGenericShannonVNdAvx512f
 *
 * Compute KooN RBD through Shannon Decomposition method with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the reliability of KooN RBD system through Shannon Decomposition
 *  exploiting amd64 AVX512F 512bit
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNGenericShannonVNdAvx512f(__mmask8 mask, struct rbdKooNGenericShannonData *data, unsigned int time)
{
    __m512d vNdRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    vNdRes = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdKooNBddStepVNdAvx512f
 *
 * Compute the Reliability value for a BDD Node with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      double *r
 *      double *h
 *      double *l
 *
 * Output:
 *      double *o
 *
 * Description:
 *  This function computes the reliability value of KooN RBD system through BDD Evaluation
 *  using amd64 AVX512F 512bit
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      r: reliability value of BDD Variable under analysis
 *      h: reliability value of BDD High Node, i.e., the BDD Variable is working
 *      l: reliability value of BDD Low Node, i.e., the BDD Variable is failed
 *      o: output reliability value
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNBddStepVNdAvx512f(__mmask8 mask, double *r, double *h, double *l, double *o)
{
    __m512d vNdR;
    __m512d vNdH;
    __m512d vNdL;
    __m512d vNdRes;

    /* Compute the reliability of the BDD Node NODE = R * H + (1 - R) * L */
    vNdR = _mm512_maskz_loadu_pd(mask, r);
    vNdL = _mm512_maskz_loadu_pd(mask, l);
    vNdRes = _mm512_maskz_fnmadd_pd(mask, vNdR, vNdL, vNdL);
    vNdH = _mm512_maskz_loadu_pd(mask, h);
    vNdRes = _mm512_maskz_fmadd_pd(mask, vNdR, vNdH, vNdRes);
    _mm512_mask_storeu_pd(o, mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdKooNIdenticalSuccessStepVNdAvx512f
 *
 * Identical KooN RBD Step function from working components with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the working components
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNIdenticalSuccessStepVNdAvx512f(__mmask8 mask, struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m512d vNdR;
    __m512d vNdTmp1, vNdTmp2;
    __m512d vNdRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    vNdR = _mm512_maskz_loadu_pd(mask, &data->reliabilities[time]);
    /* Initialize reliability to 0 */
    vNdRes = v8dZeros;
    /* Compute product between reliability and unreliability */
    vNdTmp2 = _mm512_maskz_fnmadd_pd(mask, vNdR, vNdR, vNdR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = _mm512_maskz_mov_pd(mask, _mm512_set1_pd((double)data->nCi[ii]));
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdR);
        }
        /* Add reliability of current iteration */
        vNdRes = _mm512_maskz_add_pd(mask, vNdRes, vNdTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdKooNIdenticalFailStepVNdAvx512f
 *
 * Identical KooN RBD Step function from failed components with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a KooN RBD system
 *  taking into account the failed components
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNIdenticalFailStepVNdAvx512f(__mmask8 mask, struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m512d vNdU;
    __m512d vNdTmp1, vNdTmp2;
    __m512d vNdRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    vNdTmp2 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[time]);
    /* Compute unreliability */
    vNdU = _mm512_maskz_sub_pd(mask, v8dOnes, vNdTmp2);
    /* Initialize reliability to 1 */
    vNdRes = v8dOnes;
    /* Compute product between reliability and unreliability */
    vNdTmp2 = _mm512_maskz_mul_pd(mask, vNdTmp2, vNdU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = _mm512_maskz_mov_pd(mask, _mm512_set1_pd((double)data->nCi[ii]));
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdU);
        }
        /* Subtract unreliability of current iteration */
        vNdRes = _mm512_maskz_sub_pd(mask, vNdRes, vNdTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}

/**
 * rbdKooNGenericShannonStepVNdAvx512f
 *
 * Recursive KooN RBD Shannon Decomposition function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
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
 *  exploiting amd64 AVX512F 512bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (__m512d):
 *  Computed reliability
 */
static FUNCTION_TARGET("avx512f") __m512d rbdKooNGenericShannonStepVNdAvx512f(__mmask8 mask, struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k)
{
    unsigned char best;
    unsigned char offset;
    unsigned char idx;
    unsigned char ii, jj;
    __m512d *vNdR;
    __m512d vNdRes;
    __m512d vNdTmpRec;
    __m512d vNdTmp1, vNdTmp2;
    __m512d vNdStepTmp1, vNdStepTmp2;
    int nextCombs;

    if (k == n) {
        /* Compute the Reliability as Series block */
        vNdRes = v8dOnes;
        while (n > 0) {
            vNdTmp1 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdRes = _mm512_maskz_mul_pd(mask, vNdRes, vNdTmp1);
        }
        return vNdRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        vNdRes = v8dOnes;
        while (n > 0) {
            vNdTmp1 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdRes = _mm512_maskz_fnmadd_pd(mask, vNdRes, vNdTmp1, vNdRes);
        }
        return _mm512_maskz_sub_pd(mask, v8dOnes, vNdRes);
    }

    best = (unsigned char)minimum(((int)k-1), ((int)n-(int)k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        vNdTmp1 = v8dOnes;
        vNdTmp2 = v8dOnes;
        vNdR = &data->recur.v8dR[offset];
        for (idx = 0; idx < best; idx++) {
            vNdR[idx] = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(--n * data->numTimes) + time]);
            vNdTmp1 = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdR[idx]);
            vNdTmp2 = _mm512_maskz_fnmadd_pd(mask, vNdTmp2, vNdR[idx], vNdTmp2);
        }
        vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k-best);
        vNdRes = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdTmpRec);
        vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k);
        vNdRes = _mm512_maskz_fmadd_pd(mask, vNdTmp2, vNdTmpRec, vNdRes);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            vNdTmp1 = v8dZeros;
            vNdTmp2 = v8dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = v8dOnes;
                vNdStepTmp2 = v8dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = _mm512_maskz_fnmadd_pd(mask, vNdStepTmp1, vNdR[jj], vNdStepTmp1);
                        vNdStepTmp2 = _mm512_maskz_mul_pd(mask, vNdStepTmp2, vNdR[jj]);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = _mm512_maskz_mul_pd(mask, vNdStepTmp1, vNdR[jj]);
                        vNdStepTmp2 = _mm512_maskz_fnmadd_pd(mask, vNdStepTmp2, vNdR[jj], vNdStepTmp2);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdStepTmp1 = _mm512_maskz_mul_pd(mask, vNdStepTmp1, vNdR[jj]);
                    vNdStepTmp2 = _mm512_maskz_fnmadd_pd(mask, vNdStepTmp2, vNdR[jj], vNdStepTmp2);
                    ++jj;
                }
                vNdTmp1 = _mm512_maskz_add_pd(mask, vNdTmp1, vNdStepTmp1);
                vNdTmp2 = _mm512_maskz_add_pd(mask, vNdTmp2, vNdStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k-best+idx);
            vNdRes = _mm512_maskz_fmadd_pd(mask, vNdTmp1, vNdTmpRec, vNdRes);
            vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k-idx);
            vNdRes = _mm512_maskz_fmadd_pd(mask, vNdTmp2, vNdTmpRec, vNdRes);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            vNdTmp1 = v8dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = v8dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = _mm512_maskz_fnmadd_pd(mask, vNdStepTmp1, vNdR[jj], vNdStepTmp1);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = _mm512_maskz_mul_pd(mask, vNdStepTmp1, vNdR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdStepTmp1 = _mm512_maskz_mul_pd(mask, vNdStepTmp1, vNdR[jj]);
                    ++jj;
                }
                vNdTmp1 = _mm512_maskz_add_pd(mask, vNdTmp1, vNdStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k-best+idx);
            vNdRes = _mm512_maskz_fmadd_pd(mask, vNdTmp1, vNdTmpRec, vNdRes);
        }

        return vNdRes;
    }

    /* Recursively compute the Reliability */
    vNdTmp1 = _mm512_maskz_loadu_pd(mask, &data->reliabilities[(--n * data->numTimes) + time]);
    vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k-1);
    vNdRes = _mm512_maskz_mul_pd(mask, vNdTmp1, vNdTmpRec);
    vNdTmp1 = _mm512_maskz_sub_pd(mask, v8dOnes, vNdTmp1);
    vNdTmpRec = rbdKooNGenericShannonStepVNdAvx512f(mask, data, time, n, k);
    vNdRes = _mm512_maskz_fmadd_pd(mask, vNdTmp1, vNdTmpRec, vNdRes);
    return vNdRes;
}

/**
 * rbdKooNBddAvx512f
 *
 * Recursively compute the Reliability curve of a BDD Node with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This recursive function computes the Reliability curve of the provided BDD Node
 *  using amd64 AVX512F instruction set
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
static FUNCTION_TARGET("avx512f") double *rbdKooNBddAvx512f(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps)
{
    double *nodeValues;
    unsigned char *computedNodes;
    double *high;
    double *low;
    double *rel;
    struct bddnode *node;
    unsigned int tIdx;
    __mmask8 mask;

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
    high = rbdKooNBddAvx512f(data, node->high, timeStart, numSteps);
    if (high == NULL) {
        return NULL;
    }
    /* Recursively evaluate the reliability of the low part of the current BDD Node */
    low  = rbdKooNBddAvx512f(data, node->low, timeStart, numSteps);
    if (low == NULL) {
        return NULL;
    }

    /* Retrieve the reliability curve associated with the variable */
    rel = &data->bddmgr->vars[node->var].reliability[timeStart];

    /* For each time instant to be evaluated (blocks of 8 time instants)... */
    for (tIdx = 0; (tIdx + V8D) <= numSteps; tIdx += V8D) {
        /* Compute the (cached) reliability curve associated with the current BDD Node */
        rbdKooNBddStepVNdAvx512f((__mmask8)0xFFU, &rel[tIdx], &high[tIdx], &low[tIdx], &nodeValues[tIdx]);
    }
    /* Is (at least) 1 time instant remaining? */
    if (tIdx < numSteps) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (numSteps - tIdx)) - 1);
        /* Compute the (cached) reliability curve associated with the current BDD Node */
        rbdKooNBddStepVNdAvx512f(mask, &rel[tIdx], &high[tIdx], &low[tIdx], &nodeValues[tIdx]);
    }

    /* Set the BDD Node as already evaluated */
    computedNodes[nodeIdx] = 1;

    return nodeValues;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
