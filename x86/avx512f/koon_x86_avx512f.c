/*
 *  Component: koon_x86_avx512f.c
 *  KooN (K-out-of-N) RBD management - Optimized using x86 AVX512F instruction set
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


#include "../../rbd_internal.h"

#if CPU_X86_AVX512F != 0
#include "../rbd_internal_x86.h"
#include "../koon_x86.h"


static FUNCTION_TARGET("avx512f") __m512d rbdKooNRecursiveStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k);


/**
 * rbdKooNGenericSuccessStepV8dAvx512f
 *
 * Generic KooN RBD Step function from working components with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function exploiting x86 AVX512F 512bit.
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
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNGenericSuccessStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time)
{
    __m512d v8dStep;
    __m512d v8dTmp;
    __m512d v8dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 0 */
    v8dRes = v8dZeros;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step reliability to 1 */
            v8dStep = v8dOnes;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Load reliabilities */
                v8dTmp = _mm512_loadu_pd(&data->reliabilities[(jj * data->numTimes) + time]);
                /* Does the component belong to the working components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step reliability for reliability of current component */
                    v8dStep = _mm512_mul_pd(v8dStep, v8dTmp);
                    /* Advance to next working component in combination */
                    if (++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step reliability for unreliability of current component */
                    v8dStep = _mm512_fnmadd_pd(v8dTmp, v8dStep, v8dStep);
                }
            }

            /* Perform partial sum for computation of KooN reliability */
            v8dRes = _mm512_add_pd(v8dRes, v8dStep);
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNGenericFailStepV8dAvx512f
 *
 * Generic KooN RBD Step function from failed components with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function exploiting x86 AVX512F 512bit.
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
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNGenericFailStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time)
{
    __m512d v8dStep;
    __m512d v8dTmp;
    __m512d v8dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 1 */
    v8dRes = v8dOnes;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step reliability to 1 */
            v8dStep = v8dOnes;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Load reliabilities */
                v8dTmp = _mm512_loadu_pd(&data->reliabilities[(jj * data->numTimes) + time]);
                /* Does the component belong to the working components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step unreliability for unreliability of current component */
                    v8dStep = _mm512_fnmadd_pd(v8dTmp, v8dStep, v8dStep);
                    /* Advance to next working component in combination */
                    if (++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step unreliability for reliability of current component */
                    v8dStep = _mm512_mul_pd(v8dStep, v8dTmp);
                }
            }

            /* Perform partial subtraction for computation of KooN reliability */
            v8dRes = _mm512_sub_pd(v8dRes, v8dStep);
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNRecursionV8dAvx512f
 *
 * Compute KooN RBD though Recursive method with x86 AVX512F 512bit
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
 *  exploiting x86 AVX512F 512bit
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdKooNRecursionV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time)
{
    __m512d v8dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v8dRes = rbdKooNRecursiveStepV8dAvx512f(data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    _mm512_storeu_pd(&data->output[time], capReliabilityV8dAvx512f(v8dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV8dAvx512f
 *
 * Identical KooN RBD Step function from working components with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting x86 AVX512F 512bit.
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
 * Identical KooN RBD Step function from failed components with x86 AVX512F 512bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting x86 AVX512F 512bit.
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
 * Recursive KooN RBD Step function with x86 AVX512F 512bit
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
 *  This function implements the recursive KooN RBD function exploiting x86 AVX512F 512bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (__m512d):
 *  Computed reliability
 */
static FUNCTION_TARGET("avx512f") __m512d rbdKooNRecursiveStepV8dAvx512f(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k)
{
    __m512d v8dTmp1, v8dTmp2;
    __m512d v8dRes;

    /* Load reliabilities and compute unreliabilities */
    --n;
    v8dRes = _mm512_loadu_pd(&data->reliabilities[(n * data->numTimes) + time]);
    v8dTmp1 = _mm512_sub_pd(v8dOnes, v8dRes);
    /* Recursively compute the reliabilities */
    if ((k-1) > 0) {
        v8dTmp2 = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k-1);
        v8dRes = _mm512_mul_pd(v8dRes, v8dTmp2);
    }
    if (k <= n) {
        v8dTmp2 = rbdKooNRecursiveStepV8dAvx512f(data, time, n, k);
        v8dRes = _mm512_fmadd_pd(v8dTmp1, v8dTmp2, v8dRes);
    }
    return v8dRes;
}


#endif /* CPU_X86_AVX512F */
