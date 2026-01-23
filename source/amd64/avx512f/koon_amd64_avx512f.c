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


static __m512d rbdKooNRecursiveStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time, short n, short k);


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
    __m256d m256d;
    __m128d m128d;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* Define vector (8d, 4d and 2d) with provided value */
    m512d = _mm512_set1_pd(data->value);
    m256d = _mm256_set1_pd(data->value);
    m128d = _mm_set1_pd(data->value);

    /* For each time instant (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Fill output Reliability array with fixed value */
        _mm512_storeu_pd(&data->output[time], m512d);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Fill output Reliability array with fixed value */
        _mm256_storeu_pd(&data->output[time], m256d);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Fill output Reliability array with fixed value */
        _mm_storeu_pd(&data->output[time], m128d);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time++] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerAvx512f
 *
 * Generic KooN RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorkerAvx512f(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV8dAvx512f(data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Are (at least) 4 time instants remaining? */
    if ((time + V4D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV4dFma3(data, time);
        /* Increment current time instant */
        time += V4D;
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV2dFma3(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
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
HIDDEN void *rbdKooNIdenticalWorkerAvx512f(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV2dFma3(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from working components */
                rbdKooNIdenticalSuccessStepV4dFma3(data, time);
                /* Increment current time instant */
                time += V4D;
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV4dFma3(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dFma3(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepS1d(data, time);
        }
    }
    else {
        /* Align, if possible, to vector size */
        if (((long)&data->reliabilities[time] & (S1D * sizeof(double) - 1)) == 0) {
            if (((long)&data->reliabilities[time] & (V2D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepS1d(data, time);
                /* Increment current time instant */
                time += S1D;
            }
            if (((long)&data->reliabilities[time] & (V4D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV2dSse2(data, time);
                /* Increment current time instant */
                time += V2D;
            }
            if (((long)&data->reliabilities[time] & (V8D * sizeof(double) - 1)) != 0) {
                /* Compute reliability of KooN RBD at current time instant from failed components */
                rbdKooNIdenticalFailStepV4dAvx(data, time);
                /* Increment current time instant */
                time += V4D;
            }
        }
        /* For each time instant to be processed (blocks of 8 time instants)... */
        while ((time + V8D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V8D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV8dAvx512f(data, time);
            /* Increment current time instant */
            time += (data->numCores * V8D);
        }
        /* Are (at least) 4 time instants remaining? */
        if ((time + V4D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV4dAvx(data, time);
            /* Increment current time instant */
            time += V4D;
        }
        /* Are (at least) 2 time instants remaining? */
        if ((time + V2D) <= data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dSse2(data, time);
            /* Increment current time instant */
            time += V2D;
        }
        /* Is 1 time instant remaining? */
        if (time < data->numTimes) {
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepS1d(data, time);
        }
    }

    return NULL;
}

/**
 * rbdKooNRecursionV8dAvx512f
 *
 * Compute KooN RBD though Recursive method with amd64 AVX512F 512bit
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
 *  exploiting amd64 AVX512F 512bit
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNRecursionV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time)
{
    __m512d v8dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v8dRes = rbdKooNRecursiveStepV8dAvx512f(data, time, (short)data->numComponents, (short)data->minComponents);
    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV8dAvx512f
 *
 * Identical KooN RBD Step function from working components with amd64 AVX512F 512bit
 *
 * Input:
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
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNIdenticalSuccessStepV8dAvx512f(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m512d v8dR;
    __m512d v8dTmp1, v8dTmp2;
    __m512d v8dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v8dR = _mm512_loadu_pd(&data->reliabilities[time]);
    /* Initialize reliability to 0 */
    v8dRes = v8dZeros;
    /* Compute product between reliability and unreliability */
    v8dTmp2 = _mm512_fnmadd_pd(v8dR, v8dR, v8dR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v8dTmp1 = _mm512_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            v8dTmp1 = _mm512_mul_pd(v8dTmp1, v8dTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            v8dTmp1 = _mm512_mul_pd(v8dTmp1, v8dR);
        }
        /* Add reliability of current iteration */
        v8dRes = _mm512_add_pd(v8dRes, v8dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNIdenticalFailStepV8dAvx512f
 *
 * Identical KooN RBD Step function from failed components with amd64 AVX512F 512bit
 *
 * Input:
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
 *      data: Identical KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNIdenticalFailStepV8dAvx512f(struct rbdKooNIdenticalData *data, unsigned int time)
{
    __m512d v8dU;
    __m512d v8dTmp1, v8dTmp2;
    __m512d v8dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v8dTmp2 = _mm512_loadu_pd(&data->reliabilities[time]);
    /* Compute unreliability */
    v8dU = _mm512_sub_pd(v8dOnes, v8dTmp2);
    /* Initialize reliability to 1 */
    v8dRes = v8dOnes;
    /* Compute product between reliability and unreliability */
    v8dTmp2 = _mm512_mul_pd(v8dTmp2, v8dU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v8dTmp1 = _mm512_set1_pd((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            v8dTmp1 = _mm512_mul_pd(v8dTmp1, v8dTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            v8dTmp1 = _mm512_mul_pd(v8dTmp1, v8dU);
        }
        /* Subtract unreliability of current iteration */
        v8dRes = _mm512_sub_pd(v8dRes, v8dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNRecursiveStepV8dAvx512f
 *
 * Recursive KooN RBD Step function with amd64 AVX512F 512bit
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
 *  This function implements the recursive KooN RBD function exploiting amd64 AVX512F 512bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (__m512d):
 *  Computed reliability
 */
static FUNCTION_TARGET("avx512f") __m512d rbdKooNRecursiveStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    __m512d *v8dR;
    __m512d v8dRes;
    __m512d v8dTmpRec;
    __m512d v8dTmp1, v8dTmp2;
    __m512d v8dStepTmp1, v8dStepTmp2;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;

    if (k == n) {
        /* Compute the Reliability as Series block */
        v8dRes = v8dOnes;
        while (n > 0) {
            v8dTmp1 = _mm512_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v8dRes = _mm512_mul_pd(v8dRes, v8dTmp1);
        }
        return v8dRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        v8dRes = v8dOnes;
        while (n > 0) {
            v8dTmp1 = _mm512_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v8dRes = _mm512_fnmadd_pd(v8dRes, v8dTmp1, v8dRes);
        }
        return _mm512_sub_pd(v8dOnes, v8dRes);
    }

    best = (short)minimum((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        v8dTmp1 = v8dOnes;
        v8dTmp2 = v8dOnes;
        v8dR = &data->recur.v8dR[offset];
        for (idx = 0; idx < best; idx++) {
            v8dR[idx] = _mm512_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
            v8dTmp1 = _mm512_mul_pd(v8dTmp1, v8dR[idx]);
            v8dTmp2 = _mm512_fnmadd_pd(v8dTmp2, v8dR[idx], v8dTmp2);
        }
        v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-best);
        v8dRes = _mm512_mul_pd(v8dTmp1, v8dTmpRec);
        v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k);
        v8dRes = _mm512_fmadd_pd(v8dTmp2, v8dTmpRec, v8dRes);
        for (idx = 1; idx < ceilDivision(best, 2); ++idx) {
            v8dTmp1 = v8dZeros;
            v8dTmp2 = v8dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v8dStepTmp1 = v8dOnes;
                v8dStepTmp2 = v8dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        v8dStepTmp1 = _mm512_fnmadd_pd(v8dStepTmp1, v8dR[jj], v8dStepTmp1);
                        v8dStepTmp2 = _mm512_mul_pd(v8dStepTmp2, v8dR[jj]);
                        ++ii;
                    }
                    else {
                        v8dStepTmp1 = _mm512_mul_pd(v8dStepTmp1, v8dR[jj]);
                        v8dStepTmp2 = _mm512_fnmadd_pd(v8dStepTmp2, v8dR[jj], v8dStepTmp2);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v8dStepTmp1 = _mm512_mul_pd(v8dStepTmp1, v8dR[jj]);
                    v8dStepTmp2 = _mm512_fnmadd_pd(v8dStepTmp2, v8dR[jj], v8dStepTmp2);
                    ++jj;
                }
                v8dTmp1 = _mm512_add_pd(v8dTmp1, v8dStepTmp1);
                v8dTmp2 = _mm512_add_pd(v8dTmp2, v8dStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-best+idx);
            v8dRes = _mm512_fmadd_pd(v8dTmp1, v8dTmpRec, v8dRes);
            v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-idx);
            v8dRes = _mm512_fmadd_pd(v8dTmp2, v8dTmpRec, v8dRes);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            v8dTmp1 = v8dZeros;
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                v8dStepTmp1 = v8dOnes;
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    if (data->recur.comb[ii] == jj) {
                        v8dStepTmp1 = _mm512_fnmadd_pd(v8dStepTmp1, v8dR[jj], v8dStepTmp1);
                        ++ii;
                    }
                    else {
                        v8dStepTmp1 = _mm512_mul_pd(v8dStepTmp1, v8dR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v8dStepTmp1 = _mm512_mul_pd(v8dStepTmp1, v8dR[jj]);
                    ++jj;
                }
                v8dTmp1 = _mm512_add_pd(v8dTmp1, v8dStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-best+idx);
            v8dRes = _mm512_fmadd_pd(v8dTmp1, v8dTmpRec, v8dRes);
        }

        return v8dRes;
    }

    /* Recursively compute the Reliability */
    v8dTmp1 = _mm512_loadu_pd(&data->reliabilities[(--n * data->numTimes) + time]);
    v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-1);
    v8dRes = _mm512_mul_pd(v8dTmp1, v8dTmpRec);
    v8dTmp1 = _mm512_sub_pd(v8dOnes, v8dTmp1);
    v8dTmpRec = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k);
    v8dRes = _mm512_fmadd_pd(v8dTmp1, v8dTmpRec, v8dRes);
    return v8dRes;
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
