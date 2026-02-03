/*
 *  Component: koon_power8_vsx.c
 *  KooN (K-out-of-N) RBD management - Optimized using POWER8 VSX instruction set
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

#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_power8.h"
#include "../koon_power8.h"
#include "../../generic/combinations.h"


static double64x2 rbdKooNRecursiveStepV2dVsx(struct rbdKooNGenericData *data, unsigned int time, short n, short k);


/**
 * rbdKooNFillWorkerVsx
 *
 * Fill output Reliability with fixed value Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: Fill KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("vsx") void *rbdKooNFillWorkerVsx(struct rbdKooNFillData *data)
{
    unsigned int time;
    double64x2 m128d;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;
    /* Define vector (2d) with provided value */
    m128d = vec_splats(data->value);

    /* For each time instant (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Fill output Reliability array with fixed value */
        vectorStore(&data->output[time], m128d);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Fill output Reliability array with fixed value */
        data->output[time++] = data->value;
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerVsx
 *
 * Generic KooN RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNGenericWorkerVsx(struct rbdKooNGenericData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

    /* For each time instant to be processed (blocks of 2 time instants)... */
    while ((time + V2D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * V2D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionV2dVsx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V2D);
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionS1d(data, time);
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerVsx
 *
 * Identical KooN RBD Worker function with POWER8 VSX instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting POWER8 VSX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system by using
 *  previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNIdenticalWorkerVsx(struct rbdKooNIdenticalData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V2D;

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
        }
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepV2dVsx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V2D);
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
        }
        /* For each time instant to be processed (blocks of 2 time instants)... */
        while ((time + V2D) <= data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * V2D));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V2D));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepV2dVsx(data, time);
            /* Increment current time instant */
            time += (data->numCores * V2D);
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
 * rbdKooNRecursionV2dVsx
 *
 * Compute KooN RBD though Recursive method with POWER8 VSX 128bit
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
 *  exploiting POWER8 VSX 128bit
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("vsx") void rbdKooNRecursionV2dVsx(struct rbdKooNGenericData *data, unsigned int time)
{
    double64x2 v2dRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    v2dRes = rbdKooNRecursiveStepV2dVsx(data, time, (short)data->numComponents, (short)data->minComponents);
    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdKooNIdenticalSuccessStepV2dVsx
 *
 * Identical KooN RBD Step function from working components with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting POWER8 VSX 128bit.
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
HIDDEN FUNCTION_TARGET("vsx") void rbdKooNIdenticalSuccessStepV2dVsx(struct rbdKooNIdenticalData *data, unsigned int time)
{
    double64x2 v2dR;
    double64x2 v2dTmp1, v2dTmp2;
    double64x2 v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dR = vectorLoad(&data->reliabilities[time]);
    /* Initialize reliability to 0 */
    v2dRes = v2dZeros;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = vec_nmadd(v2dR, v2dR, v2dR);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = vec_splats((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            v2dTmp1 = vec_mul(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            v2dTmp1 = vec_mul(v2dTmp1, v2dR);
        }
        /* Add reliability of current iteration */
        v2dRes = vec_add(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdKooNIdenticalFailStepV2dVsx
 *
 * Identical KooN RBD Step function from failed components with POWER8 VSX 128bit
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting POWER8 VSX 128bit.
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
HIDDEN FUNCTION_TARGET("vsx") void rbdKooNIdenticalFailStepV2dVsx(struct rbdKooNIdenticalData *data, unsigned int time)
{
    double64x2 v2dU;
    double64x2 v2dTmp1, v2dTmp2;
    double64x2 v2dRes;
    int numWork, numFail;
    int ii, jj;

    /* Retrieve reliability */
    v2dTmp2 = vectorLoad(&data->reliabilities[time]);
    /* Compute unreliability */
    v2dU = vec_sub(v2dOnes, v2dTmp2);
    /* Initialize reliability to 1 */
    v2dRes = v2dOnes;
    /* Compute product between reliability and unreliability */
    v2dTmp2 = vec_mul(v2dTmp2, v2dU);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        v2dTmp1 = vec_splats((double)data->nCi[ii]);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            v2dTmp1 = vec_mul(v2dTmp1, v2dTmp2);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            v2dTmp1 = vec_mul(v2dTmp1, v2dU);
        }
        /* Subtract unreliability of current iteration */
        v2dRes = vec_sub(v2dRes, v2dTmp1);
    }

    /* Cap the computed reliability and set it into output array */
    vectorStore(&data->output[time], capReliabilityV2dVsx(v2dRes));
}

/**
 * rbdKooNRecursiveStepV2dVsx
 *
 * Recursive KooN RBD Step function with POWER8 VSX 128bit
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
 *  This function implements the recursive KooN RBD function exploiting POWER8 VSX 128bit.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *
 * Return (vector double):
 *  Computed reliability
 */
static FUNCTION_TARGET("vsx") double64x2 rbdKooNRecursiveStepV2dVsx(struct rbdKooNGenericData *data, unsigned int time, short n, short k)
{
    short best;
    double64x2 *v2dR;
    double64x2 v2dRes;
    double64x2 v2dTmpRec;
    double64x2 v2dTmp1, v2dTmp2;
    double64x2 v2dStepTmp1, v2dStepTmp2;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;

    if (k == n) {
        /* Compute the Reliability as Series block */
        v2dRes = v2dOnes;
        while (n > 0) {
            v2dTmp1 = vectorLoad(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dRes = vec_mul(v2dRes, v2dTmp1);
        }
        return v2dRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        v2dRes = v2dOnes;
        while (n > 0) {
            v2dTmp1 = vectorLoad(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dRes = vec_nmadd(v2dRes, v2dTmp1, v2dRes);
        }
        return vec_sub(v2dOnes, v2dRes);
    }

    best = (short)minimum((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        v2dTmp1 = v2dOnes;
        v2dTmp2 = v2dOnes;
        v2dR = &data->recur.v2dR[offset];
        for (idx = 0; idx < best; idx++) {
            v2dR[idx] = vectorLoad(&data->reliabilities[(--n * data->numTimes) + time]);
            v2dTmp1 = vec_mul(v2dTmp1, v2dR[idx]);
            v2dTmp2 = vec_nmadd(v2dTmp2, v2dR[idx], v2dTmp2);
        }
        v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k-best);
        v2dRes = vec_mul(v2dTmp1, v2dTmpRec);
        v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k);
        v2dRes = vec_madd(v2dTmp2, v2dTmpRec, v2dRes);
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
                    if (data->recur.comb[ii] == jj) {
                        v2dStepTmp1 = vec_nmadd(v2dStepTmp1, v2dR[jj], v2dStepTmp1);
                        v2dStepTmp2 = vec_mul(v2dStepTmp2, v2dR[jj]);
                        ++ii;
                    }
                    else {
                        v2dStepTmp1 = vec_mul(v2dStepTmp1, v2dR[jj]);
                        v2dStepTmp2 = vec_nmadd(v2dStepTmp2, v2dR[jj], v2dStepTmp2);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v2dStepTmp1 = vec_mul(v2dStepTmp1, v2dR[jj]);
                    v2dStepTmp2 = vec_nmadd(v2dStepTmp2, v2dR[jj], v2dStepTmp2);
                    ++jj;
                }
                v2dTmp1 = vec_add(v2dTmp1, v2dStepTmp1);
                v2dTmp2 = vec_add(v2dTmp2, v2dStepTmp2);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k-best+idx);
            v2dRes = vec_madd(v2dTmp1, v2dTmpRec, v2dRes);
            v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k-idx);
            v2dRes = vec_madd(v2dTmp2, v2dTmpRec, v2dRes);
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
                        v2dStepTmp1 = vec_nmadd(v2dStepTmp1, v2dR[jj], v2dStepTmp1);
                        ++ii;
                    }
                    else {
                        v2dStepTmp1 = vec_mul(v2dStepTmp1, v2dR[jj]);
                    }
                    ++jj;
                }
                while (jj < best) {
                    v2dStepTmp1 = vec_mul(v2dStepTmp1, v2dR[jj]);
                    ++jj;
                }
                v2dTmp1 = vec_add(v2dTmp1, v2dStepTmp1);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k-best+idx);
            v2dRes = vec_madd(v2dTmp1, v2dTmpRec, v2dRes);
        }

        return v2dRes;
    }

    /* Recursively compute the Reliability */
    v2dTmp1 = vectorLoad(&data->reliabilities[(--n * data->numTimes) + time]);
    v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k-1);
    v2dRes = vec_mul(v2dTmp1, v2dTmpRec);
    v2dTmp1 = vec_sub(v2dOnes, v2dTmp1);
    v2dTmpRec = rbdKooNRecursiveStepV2dVsx(data, time, n, k);
    v2dRes = vec_madd(v2dTmp1, v2dTmpRec, v2dRes);
    return v2dRes;
}


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */
