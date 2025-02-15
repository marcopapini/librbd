/*
 *  Component: koon_aarch64_neon.c
 *  KooN (K-out-of-N) RBD management - Optimized using AArch64 NEON instruction set
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


static FUNCTION_TARGET("arch=armv8-a") float64x2_t rbdKooNRecursiveStepV2dNeon(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k);


/**
 * rbdKooNGenericSuccessStepV2dNeon
 *
 * Generic KooN RBD Step function from working components with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function exploiting AArch64 NEON 128bit.
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
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdKooNGenericSuccessStepV2dNeon(struct rbdKooNGenericData *data, unsigned int time)
{
    float64x2_t v2dStep;
    float64x2_t v2dTmp;
    float64x2_t v2dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 0 */
    v2dRes = v2dZeros;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step reliability to 1 */
            v2dStep = v2dOnes;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Load reliabilities */
                v2dTmp = vld1q_f64(&data->reliabilities[(jj * data->numTimes) + time]);
                /* Does the component belong to the working components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step reliability for reliability of current component */
                    v2dStep = vmulq_f64(v2dStep, v2dTmp);
                    /* Advance to next working component in combination */
                    if (++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step reliability for unreliability of current component */
                    v2dStep = vfmsq_f64(v2dStep, v2dTmp, v2dStep);
                }
            }

            /* Perform partial sum for computation of KooN reliability */
            v2dRes = vaddq_f64(v2dRes, v2dStep);
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdKooNGenericFailStepV2dNeon
 *
 * Generic KooN RBD Step function from failed components with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD function exploiting AArch64 NEON 128bit.
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
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdKooNGenericFailStepV2dNeon(struct rbdKooNGenericData *data, unsigned int time)
{
    float64x2_t v2dStep;
    float64x2_t v2dTmp;
    float64x2_t v2dRes;
    int ii, jj, idx;
    unsigned long long numCombinations;
    unsigned long long offset;

    /* Initialize reliability of current time instant to 1 */
    v2dRes = v2dOnes;

    /* For each possible set of combinations... */
    for (ii = 0; ii < data->combs->numKooNcombinations; ++ii) {
        /* Retrieve number of combinations */
        numCombinations = data->combs->combinations[ii]->numCombinations;
        offset = 0;
        /* For each combination... */
        while (numCombinations-- > 0) {
            /* Initialize step reliability to 1 */
            v2dStep = v2dOnes;
            idx = 0;
            /* For each component... */
            for (jj = 0; jj < data->numComponents; ++jj) {
                /* Load reliabilities */
                v2dTmp = vld1q_f64(&data->reliabilities[(jj * data->numTimes) + time]);
                /* Does the component belong to the working components for current combination? */
                if (data->combs->combinations[ii]->buff[offset + idx] == jj) {
                    /* Multiply step unreliability for unreliability of current component */
                    v2dStep = vfmsq_f64(v2dStep, v2dTmp, v2dStep);
                    /* Advance to next working component in combination */
                    if (++idx == data->combs->combinations[ii]->k) {
                        idx = 0;
                    }
                }
                else {
                    /* Multiply step unreliability for reliability of current component */
                    v2dStep = vmulq_f64(v2dStep, v2dTmp);
                }
            }

            /* Perform partial subtraction for computation of KooN reliability */
            v2dRes = vsubq_f64(v2dRes, v2dStep);
            /* Increment offset for combination access */
            offset += data->combs->combinations[ii]->k;
        }
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdKooNRecursionV2dNeon
 *
 * Compute KooN RBD though Recursive method with AArch64 NEON 128bit
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
 *  exploiting AArch64 NEON 128bit
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdKooNRecursionV2dNeon(struct rbdKooNGenericData *data, unsigned int time)
{
    float64x2_t v2dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v2dRes = rbdKooNRecursiveStepV2dNeon(data, time, data->numComponents, data->minComponents);
    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV2dNeon
 *
 * Identical KooN RBD Step function from working components with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting AArch64 NEON 128bit.
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
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdKooNIdenticalSuccessStepV2dNeon(struct rbdKooNIdenticalData *data, unsigned int time)
{
    float64x2_t v2dR;
    float64x2_t v2dTmp1, v2dTmp2;
    float64x2_t v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dR = vld1q_f64(&data->reliabilities[time]);
    /* Initialize reliability to 0 */
    v2dRes = v2dZeros;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = vfmsq_f64(v2dR, v2dR, v2dR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = vdupq_n_f64((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            v2dTmp1 = vmulq_f64(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            v2dTmp1 = vmulq_f64(v2dTmp1, v2dR);
        }
        /* Add reliability of current iteration */
        v2dRes = vaddq_f64(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdKooNIdenticalFailStepV2dNeon
 *
 * Identical KooN RBD Step function from failed components with AArch64 NEON 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting AArch64 NEON 128bit.
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
HIDDEN FUNCTION_TARGET("arch=armv8-a") void rbdKooNIdenticalFailStepV2dNeon(struct rbdKooNIdenticalData *data, unsigned int time)
{
    float64x2_t v2dU;
    float64x2_t v2dTmp1, v2dTmp2;
    float64x2_t v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dTmp2 = vld1q_f64(&data->reliabilities[time]);
    /* Compute unreliability */
    v2dU = vsubq_f64(v2dOnes, v2dTmp2);
    /* Initialize reliability to 1 */
    v2dRes = v2dOnes;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = vmulq_f64(v2dTmp2, v2dU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = vdupq_n_f64((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            v2dTmp1 = vmulq_f64(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            v2dTmp1 = vmulq_f64(v2dTmp1, v2dU);
        }
        /* Subtract unreliability of current iteration */
        v2dRes = vsubq_f64(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    vst1q_f64(&data->output[time], capReliabilityV2dNeon(v2dRes));
}

/**
 * rbdKooNRecursiveStepV2dNeon
 *
 * Recursive KooN RBD Step function with AArch64 NEON 128bit
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
 *  This function implements the recursive KooN RBD function exploiting AArch64 NEON 128bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (float64x2_t):
 *  Computed reliability
 */
static FUNCTION_TARGET("arch=armv8-a") float64x2_t rbdKooNRecursiveStepV2dNeon(struct rbdKooNGenericData *data, unsigned int time, unsigned char n, unsigned char k)
{
    float64x2_t v2dTmp1, v2dTmp2;
    float64x2_t v2dRes;

    /* Load reliabilities and compute unreliabilities */
    --n;
    v2dRes = vld1q_f64(&data->reliabilities[(n * data->numTimes) + time]);
    v2dTmp1 = vsubq_f64(v2dOnes, v2dRes);
    /* Recursively compute the reliabilities */
    if ((k-1) > 0) {
        v2dTmp2 = rbdKooNRecursiveStepV2dNeon(data, time, n, k-1);
        v2dRes = vmulq_f64(v2dRes, v2dTmp2);
    }
    if (k <= n) {
        v2dTmp2 = rbdKooNRecursiveStepV2dNeon(data, time, n, k);
        v2dRes = vfmaq_f64(v2dRes, v2dTmp1, v2dTmp2);
    }
    return v2dRes;
}


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */
