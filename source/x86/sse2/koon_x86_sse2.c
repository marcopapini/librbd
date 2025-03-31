/*
 *  Component: koon_x86_sse2.c
 *  KooN (K-out-of-N) RBD management - Optimized using x86 SSE2 instruction set
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

#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_x86.h"
#include "../koon_x86.h"
#include "../../generic/combinations.h"


static FUNCTION_TARGET("sse2") __m128d rbdKooNRecursiveStepV2dSse2(struct rbdKooNGenericData *data, unsigned int time, short n, short k);


/**
 * rbdKooNRecursionV2dSse2
 *
 * Compute KooN RBD though Recursive method with x86 SSE2 128bit
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
 *  exploiting x86 SSE2 128bit
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("sse2") void rbdKooNRecursionV2dSse2(struct rbdKooNGenericData *data, unsigned int time)
{
    __m128d v2dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v2dRes = rbdKooNRecursiveStepV2dSse2(data, time, (short)data->numComponents, (short)data->minComponents);
    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV2dSse2
 *
 * Identical KooN RBD Step function from working components with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting x86 SSE2 128bit.
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
HIDDEN FUNCTION_TARGET("sse2") void rbdKooNIdenticalSuccessStepV2dSse2(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m128d v2dR;
    __m128d v2dTmp1, v2dTmp2;
    __m128d v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dR = _mm_loadu_pd(&data->reliabilities[time]);
    /* Initialize reliability to 0 */
    v2dRes = v2dZeros;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = _mm_sub_pd(v2dOnes, v2dR);
    v2dTmp2 = _mm_mul_pd(v2dR, v2dTmp2);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = _mm_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dR);
        }
        /* Add reliability of current iteration */
        v2dRes = _mm_add_pd(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdKooNIdenticalFailStepV2dSse2
 *
 * Identical KooN RBD Step function from failed components with x86 SSE2 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting x86 SSE2 128bit.
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
HIDDEN FUNCTION_TARGET("sse2") void rbdKooNIdenticalFailStepV2dSse2(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m128d v2dU;
    __m128d v2dTmp1, v2dTmp2;
    __m128d v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dTmp2 = _mm_loadu_pd(&data->reliabilities[time]);
    /* Compute unreliability */
    v2dU = _mm_sub_pd(v2dOnes, v2dTmp2);
    /* Initialize reliability to 1 */
    v2dRes = v2dOnes;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = _mm_mul_pd(v2dTmp2, v2dU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = _mm_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dU);
        }
        /* Subtract unreliability of current iteration */
        v2dRes = _mm_sub_pd(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm_storeu_pd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}

/**
 * rbdKooNRecursiveStepV2dSse2
 *
 * Recursive KooN RBD Step function with x86 SSE2 128bit
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
 *  This function implements the recursive KooN RBD function exploiting x86 SSE2 128bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (__m128d):
 *  Computed reliability
 */
static FUNCTION_TARGET("sse2") __m128d rbdKooNRecursiveStepV2dSse2(struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    __m128d *v2dR;
    __m128d v2dRes;
    __m128d v2dTmpRec;
    __m128d v2dTmp1, v2dTmp2;
    __m128d v2dStepTmp1, v2dStepTmp2;
    __m128d v2dU;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;

    if (k == n) {
        /* Compute the Reliability as Series block */
        v2dRes = v2dOnes;
        while (n > 0) {
            v2dTmp1 = _mm_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dRes = _mm_mul_pd(v2dRes, v2dTmp1);
        }
        return v2dRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        v2dRes = v2dOnes;
        while (n > 0) {
            v2dTmp1 = _mm_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dTmp1 = _mm_sub_pd(v2dOnes, v2dTmp1);
            v2dRes = _mm_mul_pd(v2dRes, v2dTmp1);
        }
        return _mm_sub_pd(v2dOnes, v2dRes);
    }

    best = (short)minimum((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        v2dTmp1 = v2dOnes;
        v2dTmp2 = v2dOnes;
        v2dR = &data->recur.v2dR[offset];
        for (idx = 0; idx < best; idx++) {
            v2dR[idx] = _mm_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dU = _mm_sub_pd(v2dOnes, v2dR[idx]);
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dR[idx]);
            v2dTmp2 = _mm_mul_pd(v2dTmp2, v2dU);
        }
        v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k-best);
        v2dRes = _mm_mul_pd(v2dTmp1, v2dTmpRec);
        v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k);
        v2dTmp2 = _mm_mul_pd(v2dTmp2, v2dTmpRec);
        v2dRes = _mm_add_pd(v2dRes, v2dTmp2);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            v2dTmp1 = v2dZeros;
            v2dTmp2 = v2dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v2dStepTmp1 = v2dOnes;
                v2dStepTmp2 = v2dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    v2dU = _mm_sub_pd(v2dOnes, v2dR[jj]);
                    if (data->recur.comb[ii] == jj) {
                        v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dU);
                        v2dStepTmp2 = _mm_mul_pd(v2dStepTmp2, v2dR[jj]);
                        ++ii;
                    }
                    else {
                        v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dR[jj]);
                        v2dStepTmp2 = _mm_mul_pd(v2dStepTmp2, v2dU);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v2dU = _mm_sub_pd(v2dOnes, v2dR[jj]);
                    v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dR[jj]);
                    v2dStepTmp2 = _mm_mul_pd(v2dStepTmp2, v2dU);
                    ++jj;
                }
                v2dTmp1 = _mm_add_pd(v2dTmp1, v2dStepTmp1);
                v2dTmp2 = _mm_add_pd(v2dTmp2, v2dStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k-best+idx);
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dTmpRec);
            v2dRes = _mm_add_pd(v2dRes, v2dTmp1);
            v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k-idx);
            v2dTmp2 = _mm_mul_pd(v2dTmp2, v2dTmpRec);
            v2dRes = _mm_add_pd(v2dRes, v2dTmp2);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            v2dTmp1 = v2dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v2dStepTmp1 = v2dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        v2dU = _mm_sub_pd(v2dOnes, v2dR[jj]);
                        v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dU);
                        ++ii;
                    }
                    else {
                        v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v2dStepTmp1 = _mm_mul_pd(v2dStepTmp1, v2dR[jj]);
                    ++jj;
                }
                v2dTmp1 = _mm_add_pd(v2dTmp1, v2dStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k-best+idx);
            v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dTmpRec);
            v2dRes = _mm_add_pd(v2dRes, v2dTmp1);
        }

        return v2dRes;
    }

    /* Recursively compute the Reliability */
    v2dTmp1 = _mm_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
    v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k-1);
    v2dRes = _mm_mul_pd(v2dTmp1, v2dTmpRec);
    v2dTmp1 = _mm_sub_pd(v2dOnes, v2dTmp1);
    v2dTmpRec = rbdKooNRecursiveStepV2dSse2(data, time, n, k);
    v2dTmp1 = _mm_mul_pd(v2dTmp1, v2dTmpRec);
    v2dRes = _mm_add_pd(v2dRes, v2dTmp1);
    return v2dRes;
}


#endif /* (defined(ARCH_AMD64) || defined(ARCH_X86)) && (CPU_ENABLE_SIMD != 0) */
