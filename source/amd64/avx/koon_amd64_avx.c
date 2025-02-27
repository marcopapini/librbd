/*
 *  Component: koon_amd64_avx.c
 *  KooN (K-out-of-N) RBD management - Optimized using amd64 AVX instruction set
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
#include "../../generic/combinations.h"


static FUNCTION_TARGET("avx") __m256d rbdKooNRecursiveStepV4dAvx(struct rbdKooNGenericData *data, unsigned int time, short n, short k);


/**
 * rbdKooNRecursionV4dAvx
 *
 * Compute KooN RBD though Recursive method with amd64 AVX 256bit
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
 *  exploiting amd64 AVX 256bit
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx") void rbdKooNRecursionV4dAvx(struct rbdKooNGenericData *data, unsigned int time)
{
    __m256d v4dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v4dRes = rbdKooNRecursiveStepV4dAvx(data, time, (short)data->numComponents, (short)data->minComponents);
    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV4dAvx
 *
 * Identical KooN RBD Step function from working components with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting amd64 AVX 256bit.
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
HIDDEN FUNCTION_TARGET("avx") void rbdKooNIdenticalSuccessStepV4dAvx(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m256d v4dR;
    __m256d v4dTmp1, v4dTmp2;
    __m256d v4dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v4dR = _mm256_loadu_pd(&data->reliabilities[time]);
    /* Initialize reliability to 0 */
    v4dRes = v4dZeros;
    /* Compute product between reliability and unreliability */
    v4dTmp2 = _mm256_sub_pd(v4dOnes, v4dR);
    v4dTmp2 = _mm256_mul_pd(v4dR, v4dTmp2);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v4dTmp1 = _mm256_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dR);
        }
        /* Add reliability of current iteration */
        v4dRes = _mm256_add_pd(v4dRes, v4dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdKooNIdenticalFailStepV4dAvx
 *
 * Identical KooN RBD Step function from failed components with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting amd64 AVX 256bit.
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
HIDDEN FUNCTION_TARGET("avx") void rbdKooNIdenticalFailStepV4dAvx(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m256d v4dU;
    __m256d v4dTmp1, v4dTmp2;
    __m256d v4dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v4dTmp2 = _mm256_loadu_pd(&data->reliabilities[time]);
    /* Compute unreliability */
    v4dU = _mm256_sub_pd(v4dOnes, v4dTmp2);
    /* Initialize reliability to 1 */
    v4dRes = v4dOnes;
    /* Compute product between reliability and unreliability */
    v4dTmp2 = _mm256_mul_pd(v4dTmp2, v4dU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v4dTmp1 = _mm256_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dU);
        }
        /* Subtract unreliability of current iteration */
        v4dRes = _mm256_sub_pd(v4dRes, v4dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdKooNRecursiveStepV4dAvx
 *
 * Recursive KooN RBD Step function with amd64 AVX 256bit
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
 *  This function implements the recursive KooN RBD function exploiting amd64 AVX 256bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (__m256d):
 *  Computed reliability
 */
static FUNCTION_TARGET("avx") __m256d rbdKooNRecursiveStepV4dAvx(struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    __m256d *v4dR;
    __m256d v4dRes;
    __m256d v4dTmpRec;
    __m256d v4dTmp1, v4dTmp2;
    __m256d v4dStepTmp1, v4dStepTmp2;
    __m256d v4dU;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;

    best = (short)minimum((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        v4dTmp1 = v4dOnes;
        v4dTmp2 = v4dOnes;
        v4dR = &data->recur.v4dR[offset];
        for (idx = 0; idx < best; idx++) {
            v4dR[idx] = _mm256_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v4dU = _mm256_sub_pd(v4dOnes, v4dR[idx]);
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dR[idx]);
            v4dTmp2 = _mm256_mul_pd(v4dTmp2, v4dU);
        }
        v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k-best);
        v4dRes = _mm256_mul_pd(v4dTmp1, v4dTmpRec);
        v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k);
        v4dTmp2 = _mm256_mul_pd(v4dTmp2, v4dTmpRec);
        v4dRes = _mm256_add_pd(v4dRes, v4dTmp2);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            v4dTmp1 = v4dZeros;
            v4dTmp2 = v4dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v4dStepTmp1 = v4dOnes;
                v4dStepTmp2 = v4dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    v4dU = _mm256_sub_pd(v4dOnes, v4dR[jj]);
                    if (data->recur.comb[ii] == jj) {
                        v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dU);
                        v4dStepTmp2 = _mm256_mul_pd(v4dStepTmp2, v4dR[jj]);
                        ++ii;
                    }
                    else {
                        v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dR[jj]);
                        v4dStepTmp2 = _mm256_mul_pd(v4dStepTmp2, v4dU);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v4dU = _mm256_sub_pd(v4dOnes, v4dR[jj]);
                    v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dR[jj]);
                    v4dStepTmp2 = _mm256_mul_pd(v4dStepTmp2, v4dU);
                    ++jj;
                }
                v4dTmp1 = _mm256_add_pd(v4dTmp1, v4dStepTmp1);
                v4dTmp2 = _mm256_add_pd(v4dTmp2, v4dStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k-best+idx);
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dTmpRec);
            v4dRes = _mm256_add_pd(v4dRes, v4dTmp1);
            v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k-idx);
            v4dTmp2 = _mm256_mul_pd(v4dTmp2, v4dTmpRec);
            v4dRes = _mm256_add_pd(v4dRes, v4dTmp2);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            v4dTmp1 = v4dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v4dStepTmp1 = v4dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        v4dU = _mm256_sub_pd(v4dOnes, v4dR[jj]);
                        v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dU);
                        ++ii;
                    }
                    else {
                        v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v4dStepTmp1 = _mm256_mul_pd(v4dStepTmp1, v4dR[jj]);
                    ++jj;
                }
                v4dTmp1 = _mm256_add_pd(v4dTmp1, v4dStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k-best+idx);
            v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dTmpRec);
            v4dRes = _mm256_add_pd(v4dRes, v4dTmp1);
        }

        return v4dRes;
    }

    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        v4dRes = v4dOnes;
        while (n > 0) {
            v4dTmp1 = _mm256_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v4dTmp1 = _mm256_sub_pd(v4dOnes, v4dTmp1);
            v4dRes = _mm256_mul_pd(v4dRes, v4dTmp1);
        }
        return _mm256_sub_pd(v4dOnes, v4dRes);
    }
    if (k == n) {
        /* Compute the Reliability as Series block */
        v4dRes = v4dOnes;
        while (n > 0) {
            v4dTmp1 = _mm256_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v4dRes = _mm256_mul_pd(v4dRes, v4dTmp1);
        }
        return v4dRes;
    }
    /* Recursively compute the Reliability */
    v4dTmp1 = _mm256_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
    v4dRes = v4dTmp1;
    if (k > 1) {
        v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k-1);
        v4dRes = _mm256_mul_pd(v4dRes, v4dTmpRec);
    }
    if (k <= n) {
        v4dTmp1 = _mm256_sub_pd(v4dOnes, v4dTmp1);
        v4dTmpRec = rbdKooNRecursiveStepV4dAvx(data, time, n, k);
        v4dTmp1 = _mm256_mul_pd(v4dTmp1, v4dTmpRec);
        v4dRes = _mm256_add_pd(v4dRes, v4dTmp1);
    }
    return v4dRes;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
