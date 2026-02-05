/*
 *  Component: koon_riscv64_rvv.c
 *  KooN (K-out-of-N) RBD management - Optimized using RISC-V 64bit RVV instruction set
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

#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)
#include "../rbd_internal_riscv64.h"
#include "../koon_riscv64.h"
#include "../../generic/combinations.h"


static vfloat64m1_t rbdKooNRecursiveStepVNdRvv(struct rbdKooNGenericData *data, unsigned int time, short n, short k, unsigned long int vl);


/**
 * minimumRvv
 *
 * Compute minimum between two numbers for RISC-V 64b RVV extension
 *
 * Input:
 *      int a
 *      int b
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the minimum between two numbers for RISC-V 64b RVV extension
 *
 * Parameters:
 *      a: first value for minimum computation
 *      b: second value for minimum computation
 *
 * Return (int):
 *  minimum value
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int minimumRvv(int a, int b) {
    return (a <= b) ? a : b;
}

/**
 * floorDivisionRvv
 *
 * Compute floor value of division for RISC-V 64b RVV extension
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the floor value of the requested division for RISC-V 64b RVV extension
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Floor value of division
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int floorDivisionRvv(int dividend, int divisor) {
    return (dividend / divisor);
}

/**
 * ceilDivisionRvv
 *
 * Compute ceil value of division for RISC-V 64b RVV extension
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the ceil value of the requested division for RISC-V 64b RVV extension
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Ceil value of division
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("arch=+v") int ceilDivisionRvv(int dividend, int divisor) {
    return floorDivisionRvv(dividend + divisor - 1, divisor);
}


/**
 * rbdKooNFillWorkerRvv
 *
 * Fill output Reliability with fixed value Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNFillData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function fills Reliability with fixed value for KooN Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to fill a given batch of output Reliabilities with a given fixed value
 *
 * Parameters:
 *      data: Fill KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNFillWorkerRvv(struct rbdKooNFillData *data)
{
    unsigned long int vl;
    unsigned int time;
    unsigned long vlmax;
    vfloat64m1_t vNdR;

    vlmax = __riscv_vsetvlmax_e64m1();

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * vlmax;

    /* For each time instant (blocks of N time instants)... */
    while (time < data->numTimes) {
        vl = __riscv_vsetvl_e64m1(data->numTimes - time);
        /* Define vector (Nd) with provided value */
        vNdR = __riscv_vfmv_v_f_f64m1(data->value, vl);

        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
        /* Fill output Reliability array with fixed value */
        __riscv_vse64_v_f64m1(&data->output[time], vNdR, vl);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdKooNGenericWorkerRvv
 *
 * Generic KooN RBD Worker function with RISC-V 64bit 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNGenericData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting RISC-V 64bit 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNGenericWorkerRvv(struct rbdKooNGenericData *data)
{
    unsigned int time;
    unsigned long int vlmax;

    vlmax = __riscv_vsetvlmax_e64m1();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * vlmax;

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + (data->numCores * vlmax));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNRecursionVNdRvv(data, time);
        /* Increment current time instant */
        time += (data->numCores * vlmax);
    }

    return NULL;
}

/**
 * rbdKooNIdenticalWorkerRvv
 *
 * Identical KooN RBD Worker function with RISC-V RVV instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting RISC-V RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *  using the previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNIdenticalWorkerRvv(struct rbdKooNIdenticalData *data)
{
    unsigned int time;
    unsigned long int vlmax;

    vlmax = __riscv_vsetvlmax_e64m1();
    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * vlmax;

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * vlmax));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdRvv(data, time);
            /* Increment current time instant */
            time += (data->numCores * vlmax);
        }
    }
    else {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < data->numTimes) {
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + (data->numCores * vlmax));
            prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * vlmax));
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdRvv(data, time);
            /* Increment current time instant */
            time += (data->numCores * vlmax);
        }
    }

    return NULL;
}

/**
 * rbdKooNRecursionVNdRvv
 *
 * Compute KooN RBD through Recursive method with RISC-V 64bit RVV
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
 *  exploiting RISC-V 64bit RVV
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNRecursionVNdRvv(struct rbdKooNGenericData *data, unsigned int time)
{
    unsigned long int vl;
    vfloat64m1_t vNdRes;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Recursively compute reliability of KooN RBD at current time instant */
    vNdRes = rbdKooNRecursiveStepVNdRvv(data, time, (short)data->numComponents, (short)data->minComponents, vl);
    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdKooNIdenticalSuccessStepVNdRvv
 *
 * Identical KooN RBD Step function from working components with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting RISC-V 64bit RVV.
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
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNIdenticalSuccessStepVNdRvv(struct rbdKooNIdenticalData *data, unsigned int time)
{
    unsigned long int vl;
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Retrieve reliability */
    vNdR = __riscv_vle64_v_f64m1(&data->reliabilities[time], vl);
    /* Initialize reliability to 0 */
    vNdRes = __riscv_vfmv_v_f_f64m1(0.0, vl);
    /* Compute product between reliability and unreliability */
    vNdTmp2 = __riscv_vfnmsac_vv_f64m1(vNdR, vNdR, vNdR, vl);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = __riscv_vfmv_v_f_f64m1((double)data->nCi[ii], vl);
        /* Compute number of working and failed components */
        numWork = data->minComponents + ii;
        numFail = data->numComponents - data->minComponents - ii;
        /* For each failed component... */
        for (jj = (numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for product of reliability and unreliability of component */
            vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmp2, vl);
        }
        /* For each non-considered working component... */
        for (jj = (numWork - numFail - 1); jj >= 0; --jj) {
            /* Multiply step reliability for reliability of component */
            vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdR, vl);
        }
        /* Add reliability of current iteration */
        vNdRes = __riscv_vfadd_vv_f64m1(vNdRes, vNdTmp1, vl);
    }

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdKooNIdenticalFailStepVNdRvv
 *
 * Identical KooN RBD Step function from failed components with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD function exploiting RISC-V 64bit RVV.
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
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNIdenticalFailStepVNdRvv(struct rbdKooNIdenticalData *data, unsigned int time)
{
    unsigned long int vl;
    vfloat64m1_t vNdU;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdRes;
    int numWork, numFail;
    int ii, jj;

    vl = __riscv_vsetvl_e64m1(data->numTimes - time);

    /* Compute unreliability */
    vNdTmp2 = __riscv_vle64_v_f64m1(&data->reliabilities[time], vl);
    vNdU = __riscv_vfrsub_vf_f64m1(vNdTmp2, 1.0, vl);
    /* Initialize reliability to 1 */
    vNdRes = __riscv_vfmv_v_f_f64m1(1.0, vl);
    /* Compute product between reliability and unreliability */
    vNdTmp2 = __riscv_vfmul_vv_f64m1(vNdTmp2, vNdU, vl);

    /* For each iteration... */
    for (ii = data->numComponents - data->minComponents; ii >= 0; --ii) {
        /* Initialize step reliability to nCi */
        vNdTmp1 = __riscv_vfmv_v_f_f64m1((double)data->nCi[ii], vl);
        /* Compute number of working and failed components */
        numWork = data->numComponents - data->minComponents - ii;
        numFail = data->minComponents + ii;
        /* For each working component... */
        for (jj = (numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for product of reliability and unreliability of component */
            vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmp2, vl);
        }
        /* For each non-considered failed component... */
        for (jj = (numFail - numWork - 1); jj >= 0; --jj) {
            /* Multiply step unreliability for unreliability of component */
            vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdU, vl);
        }
        /* Subtract unreliability of current iteration */
        vNdRes = __riscv_vfsub_vv_f64m1(vNdRes, vNdTmp1, vl);
    }

    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdKooNRecursiveStepVNdRvv
 *
 * Recursive KooN RBD Step function with RISC-V 64bit RVV
 *
 * Input:
 *      unsigned long int vl
 *      struct rbdKooNGenericData *data
 *      unsigned int time
 *      short n
 *      short k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function exploiting RISC-V 64bit RVV.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *      vl: Vector Length
 *
 * Return (float64x2_t):
 *  Computed reliability
 */
static FUNCTION_TARGET("arch=+v") vfloat64m1_t rbdKooNRecursiveStepVNdRvv(struct rbdKooNGenericData *data, unsigned int time, short n, short k, unsigned long int vl)
{
    short best;
    double *s1dPtr;
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdRes;
    vfloat64m1_t vNdTmpRec;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdStepTmp1, vNdStepTmp2;
    int idx;
    int offset;
    int ii, jj;
    int nextCombs;
    unsigned long int vlmax;

    vlmax = __riscv_vsetvlmax_e64m1();

    if (k == n) {
        /* Compute the Reliability as Series block */
        vNdRes = __riscv_vfmv_v_f_f64m1(1.0, vl);
        while (n > 0) {
            vNdTmp1 = __riscv_vle64_v_f64m1(&data->reliabilities[(--n * data->numTimes) + time], vl);
            vNdRes = __riscv_vfmul_vv_f64m1(vNdRes, vNdTmp1, vl);
        }
        return vNdRes;
    }
    if (k == 1) {
        /* Compute the Reliability as Parallel block */
        vNdRes = __riscv_vfmv_v_f_f64m1(1.0, vl);
        while (n > 0) {
            vNdTmp1 = __riscv_vle64_v_f64m1(&data->reliabilities[(--n * data->numTimes) + time], vl);
            vNdRes = __riscv_vfnmsac_vv_f64m1(vNdRes, vNdTmp1, vNdRes, vl);
        }
        return __riscv_vfrsub_vf_f64m1(vNdRes, 1.0, vl);
    }

    best = (short)minimumRvv((int)(k-1), (int)(n-k));
    if (best > 1) {
        /* Recursively compute the Reliability - Minimize number of recursive calls */
        offset = n - best;
        vNdTmp1 = __riscv_vfmv_v_f_f64m1(1.0, vl);
        vNdTmp2 = __riscv_vfmv_v_f_f64m1(1.0, vl);
        s1dPtr = &data->recur.s1dR[offset * vlmax];
        for (idx = 0; idx < best; idx++) {
            vNdR = __riscv_vle64_v_f64m1(&data->reliabilities[(--n * data->numTimes) + time], vl);
            vNdTmp1 = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdR, vl);
            vNdTmp2 = __riscv_vfnmsac_vv_f64m1(vNdTmp2, vNdR, vNdTmp2, vl);
            __riscv_vse64_v_f64m1(&s1dPtr[idx * vlmax], vNdR, vl);
        }
        vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k-best, vl);
        vNdRes = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmpRec, vl);
        vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k, vl);
        vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp2, vNdTmpRec, vl);
        for (idx = 1; idx < ceilDivisionRvv(best, 2); ++idx) {
            vNdTmp1 = __riscv_vfmv_v_f_f64m1(0.0, vl);
            vNdTmp2 = __riscv_vfmv_v_f_f64m1(0.0, vl);
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = __riscv_vfmv_v_f_f64m1(1.0, vl);
                vNdStepTmp2 = __riscv_vfmv_v_f_f64m1(1.0, vl);
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    vNdR = __riscv_vle64_v_f64m1(&s1dPtr[jj * vlmax], vl);
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = __riscv_vfnmsac_vv_f64m1(vNdStepTmp1, vNdR, vNdStepTmp1, vl);
                        vNdStepTmp2 = __riscv_vfmul_vv_f64m1(vNdStepTmp2, vNdR, vl);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = __riscv_vfmul_vv_f64m1(vNdStepTmp1, vNdR, vl);
                        vNdStepTmp2 = __riscv_vfnmsac_vv_f64m1(vNdStepTmp2, vNdR, vNdStepTmp2, vl);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdR = __riscv_vle64_v_f64m1(&s1dPtr[jj * vlmax], vl);
                    vNdStepTmp1 = __riscv_vfmul_vv_f64m1(vNdStepTmp1, vNdR, vl);
                    vNdStepTmp2 = __riscv_vfnmsac_vv_f64m1(vNdStepTmp2, vNdR, vNdStepTmp2, vl);
                    ++jj;
                }
                vNdTmp1 = __riscv_vfadd_vv_f64m1(vNdTmp1, vNdStepTmp1, vl);
                vNdTmp2 = __riscv_vfadd_vv_f64m1(vNdTmp2, vNdStepTmp2, vl);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k-best+idx, vl);
            vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
            vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k-idx, vl);
            vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp2, vNdTmpRec, vl);
        }
        if ((best & 1) == 0) {
            idx = best / 2;
            vNdTmp1 = __riscv_vfmv_v_f_f64m1(0.0, vl);
            firstCombination((unsigned char)idx, data->recur.comb);
            do {
                vNdStepTmp1 = __riscv_vfmv_v_f_f64m1(1.0, vl);
                ii = 0;
                jj = 0;
                while (ii < idx) {
                    vNdR = __riscv_vle64_v_f64m1(&s1dPtr[jj * vlmax], vl);
                    if (data->recur.comb[ii] == jj) {
                        vNdStepTmp1 = __riscv_vfnmsac_vv_f64m1(vNdStepTmp1, vNdR, vNdStepTmp1, vl);
                        ++ii;
                    }
                    else {
                        vNdStepTmp1 = __riscv_vfmul_vv_f64m1(vNdStepTmp1, vNdR, vl);
                    }
                    ++jj;
                }
                while (jj < best) {
                    vNdR = __riscv_vle64_v_f64m1(&s1dPtr[jj * vlmax], vl);
                    vNdStepTmp1 = __riscv_vfmul_vv_f64m1(vNdStepTmp1, vNdR, vl);
                    ++jj;
                }
                vNdTmp1 = __riscv_vfadd_vv_f64m1(vNdTmp1, vNdStepTmp1, vl);
                nextCombs = nextCombination(best, idx, data->recur.comb);
            } while(nextCombs == 0);
            vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k-best+idx, vl);
            vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
        }

        return vNdRes;
    }

    /* Recursively compute the Reliability */
    vNdTmp1 = __riscv_vle64_v_f64m1(&data->reliabilities[(--n * data->numTimes) + time], vl);
    vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k-1, vl);
    vNdRes = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmpRec, vl);
    vNdTmp1 = __riscv_vfrsub_vf_f64m1(vNdTmp1, 1.0, vl);
    vNdTmpRec = rbdKooNRecursiveStepVNdRvv(data, time, n, k, vl);
    vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
    return vNdRes;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
