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
#include "rbd_internal_riscv64_rvv.h"
#include "../rbd_internal_riscv64.h"
#include "../koon_riscv64.h"
#include "../../generic/combinations.h"


static vfloat64m1_t rbdKooNGenericShannonStepVNdRvv(struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k, unsigned long int vl);
static double *rbdKooNBddRvv(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps);


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
 *      NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNFillWorkerRvv(struct rbdKooNFillData *data)
{
    unsigned int time;
    unsigned long int vl;
    vfloat64m1_t vNdR;

    /* Set first time instant to be processed */
    time = 0;
    /* For each time instant (blocks of N time instants)... */
    while (time < data->numTimes) {
        vl = __riscv_vsetvl_e64m1(data->numTimes - time);
        /* Define vector (Nd) with provided value */
        vNdR = __riscv_vfmv_v_f_f64m1(data->value, vl);
        /* Prefetch for next iteration */
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Fill output Reliability array with fixed value */
        __riscv_vse64_v_f64m1(&data->output[time], vNdR, vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdKooNGenericShannonWorkerRvv
 *
 * Generic KooN RBD Worker function exploiting Shannon Decomposition with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting Shannon Decomposition using
 *  RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *
 * Return (void *):
 *      NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNGenericShannonWorkerRvv(struct rbdKooNGenericShannonData *data)
{
    unsigned int batchSize;
    unsigned int time;
    unsigned int timeEnd;
    unsigned long int vl;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivisionRiscv64Rvv(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimumRiscv64Rvv(time + batchSize, data->numTimes);

    /* For each time instant to be processed (blocks of N time instants)... */
    while (time < timeEnd) {
        vl = __riscv_vsetvl_e64m1(timeEnd - time);
        /* Prefetch for next iteration */
        prefetchRead(data->reliabilities, data->numComponents, data->numTimes, time + vl);
        prefetchWrite(data->output, 1, data->numTimes, time + vl);
        /* Recursively compute reliability of KooN RBD at current time instant */
        rbdKooNGenericShannonVNdRvv(data, time, vl);
        /* Increment current time instant */
        time += vl;
    }

    return NULL;
}

/**
 * rbdKooNBddWorkerSse2
 *
 * Generic KooN RBD Worker function exploiting BDD Evaluation with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the generic KooN RBD Worker exploiting BDD Evaluation using
 *  RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a generic KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdKooNBddWorkerRvv(struct rbdKooNBddData *data)
{
    unsigned int time;
    unsigned int steps;
    unsigned char *computedPool;
    double *reliability;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(data->batchIdx);

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
        if (rbdKooNBddRvv(data, data->bddmgr->root, time, steps) == NULL) {
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
 * rbdKooNIdenticalWorkerRvv
 *
 * Identical KooN RBD Worker function with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the identical KooN RBD Worker exploiting RISC-V 64bit RVV instruction set.
 *  It is responsible to compute the reliabilities over a given batch of an identical KooN RBD system
 *  using the previously computed nCk values
 *
 * Parameters:
 *      data: Identical KooN RBD data structure
 *
 * Return (void *):
 *      NULL
 */
HIDDEN FUNCTION_TARGET("arch=+v") void *rbdKooNIdenticalWorkerRvv(struct rbdKooNIdenticalData *data)
{
    unsigned int batchSize;
    unsigned int time;
    unsigned int timeEnd;
    unsigned long int vl;

    /* Set CPU affinity */
    setRiscv64ThreadAffinityRvv(data->batchIdx);

    /* Retrieve size of data batch to be processed by worker */
    batchSize = ceilDivisionRiscv64Rvv(data->numTimes, data->numCores);
    /* Retrieve first time instant to be processed by worker */
    time = batchSize * data->batchIdx;
    /* Compute last time instant (excluded) to be processed by worker */
    timeEnd = minimumRiscv64Rvv(time + batchSize, data->numTimes);

    /* If compute unreliability flag is not set... */
    if (data->bComputeUnreliability == 0) {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < timeEnd) {
            vl = __riscv_vsetvl_e64m1(timeEnd - time);
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + vl);
            prefetchWrite(data->output, 1, data->numTimes, time + vl);
            /* Compute reliability of KooN RBD at current time instant from working components */
            rbdKooNIdenticalSuccessStepVNdRvv(data, time, vl);
            /* Increment current time instant */
            time += vl;
        }
    }
    else {
        /* For each time instant to be processed (blocks of N time instants)... */
        while (time < timeEnd) {
            vl = __riscv_vsetvl_e64m1(timeEnd - time);
            /* Prefetch for next iteration */
            prefetchRead(data->reliabilities, 1, data->numTimes, time + vl);
            prefetchWrite(data->output, 1, data->numTimes, time + vl);
            /* Compute reliability of KooN RBD at current time instant from failed components */
            rbdKooNIdenticalFailStepVNdRvv(data, time, vl);
            /* Increment current time instant */
            time += vl;
        }
    }

    return NULL;
}

/**
 * rbdKooNGenericShannonVNdRvv
 *
 * Compute KooN RBD through Shannon Decomposition method with with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the reliability of KooN RBD system through Shannon Decomposition
 *  exploiting RISC-V 64bit RVV
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      vl: Vector Length
 *
 * Return:
 *      None
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNGenericShannonVNdRvv(struct rbdKooNGenericShannonData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdRes;

    /* Recursively compute reliability of KooN RBD at current time instant */
    vNdRes = rbdKooNGenericShannonStepVNdRvv(data, time, data->numComponents, data->minComponents, vl);
    /* Cap the computed reliability and set it into output array */
    __riscv_vse64_v_f64m1(&data->output[time], capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdKooNBddStepVNdRvv
 *
 * Compute the Reliability value for a BDD Node with RISC-V 64bit RVV
 *
 * Input:
 *      double *r
 *      double *h
 *      double *l
 *
 * Output:
 *      double *o
 *
 * Description:
 *  This function computes the reliability value of KooN RBD system through BDD Evaluation
 *  using RISC-V 64bit RVV
 *
 * Parameters:
 *      r: reliability value of BDD Variable under analysis
 *      h: reliability value of BDD High Node, i.e., the BDD Variable is working
 *      l: reliability value of BDD Low Node, i.e., the BDD Variable is failed
 *      o: output reliability value
 *
 * Return:
 *  None
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNBddStepVNdRvv(double *r, double *h, double *l, double *o, unsigned long int vl)
{
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdH;
    vfloat64m1_t vNdL;
    vfloat64m1_t vNdRes;

    /* Compute the reliability of the BDD Node NODE = R * H + (1 - R) * L */
    vNdR = __riscv_vle64_v_f64m1(r, vl);
    vNdL = __riscv_vle64_v_f64m1(l, vl);
    vNdRes = __riscv_vfnmsac_vv_f64m1(vNdL, vNdR, vNdL, vl);
    vNdH = __riscv_vle64_v_f64m1(h, vl);
    vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdR, vNdH, vl);
    __riscv_vse64_v_f64m1(o, capReliabilityVNdRvv(vNdRes, vl), vl);
}

/**
 * rbdKooNIdenticalSuccessStepVNdRvv
 *
 * Identical KooN RBD Step function from working components with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdKooNIdenticalData *data
 *      unsigned int time
 *      unsigned long int vl
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
 *      vl: Vector Length
 *
 * Return:
 *      None
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNIdenticalSuccessStepVNdRvv(struct rbdKooNIdenticalData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdRes;
    int numWork, numFail;
    int ii, jj;

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
 *      unsigned long int vl
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
 *      vl: Vector Length
 *
 * Return:
 *      None
 */
HIDDEN FUNCTION_TARGET("arch=+v") void rbdKooNIdenticalFailStepVNdRvv(struct rbdKooNIdenticalData *data, unsigned int time, unsigned long int vl)
{
    vfloat64m1_t vNdU;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdRes;
    int numWork, numFail;
    int ii, jj;

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
 * rbdKooNGenericShannonStepVNdRvv
 *
 * Recursive KooN RBD Shannon Decomposition function with RISC-V 64bit RVV
 *
 * Input:
 *      struct rbdKooNGenericShannonData *data
 *      unsigned int time
 *      unsigned char n
 *      unsigned char k
 *      unsigned long int vl
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the recursive KooN RBD function through Shannon Decomposition method
 *  exploiting RISC-V 64bit RVV.
 *  It is responsible to recursively compute the reliability of a KooN RBD system
 *
 * Parameters:
 *      data: Generic KooN for Shannon Decomposition RBD data structure
 *      time: current time instant over which KooN RBD shall be computed
 *      n: current number of components in KooN RBD
 *      k: minimum number of working components in KooN RBD
 *      vl: Vector Length
 *
 * Return (vfloat64m1_t):
 *      Computed reliability
 */
static FUNCTION_TARGET("arch=+v") vfloat64m1_t rbdKooNGenericShannonStepVNdRvv(struct rbdKooNGenericShannonData *data, unsigned int time, unsigned char n, unsigned char k, unsigned long int vl)
{
    unsigned char best;
    unsigned char offset;
    unsigned char idx;
    unsigned char ii, jj;
    double *s1dPtr;
    vfloat64m1_t vNdR;
    vfloat64m1_t vNdRes;
    vfloat64m1_t vNdTmpRec;
    vfloat64m1_t vNdTmp1, vNdTmp2;
    vfloat64m1_t vNdStepTmp1, vNdStepTmp2;
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

    best = (unsigned char)minimumRiscv64Rvv(((int)k-1), ((int)n-(int)k));
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
        vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k-best, vl);
        vNdRes = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmpRec, vl);
        vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k, vl);
        vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp2, vNdTmpRec, vl);
        for (idx = 1; idx < ceilDivisionRiscv64Rvv(best, 2); ++idx) {
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
            vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k-best+idx, vl);
            vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
            vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k-idx, vl);
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
            vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k-best+idx, vl);
            vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
        }

        return vNdRes;
    }

    /* Recursively compute the Reliability */
    vNdTmp1 = __riscv_vle64_v_f64m1(&data->reliabilities[(--n * data->numTimes) + time], vl);
    vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k-1, vl);
    vNdRes = __riscv_vfmul_vv_f64m1(vNdTmp1, vNdTmpRec, vl);
    vNdTmp1 = __riscv_vfrsub_vf_f64m1(vNdTmp1, 1.0, vl);
    vNdTmpRec = rbdKooNGenericShannonStepVNdRvv(data, time, n, k, vl);
    vNdRes = __riscv_vfmacc_vv_f64m1(vNdRes, vNdTmp1, vNdTmpRec, vl);
    return vNdRes;
}

/**
 * rbdKooNBddRvv
 *
 * Recursively compute the Reliability curve of a BDD Node with RISC-V 64bit RVV instruction set
 *
 * Input:
 *      struct rbdKooNBddData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This recursive function computes the Reliability curve of the provided BDD Node
 *  using RISC-V 64bit RVV instruction set
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
static FUNCTION_TARGET("arch=+v") double *rbdKooNBddRvv(struct rbdKooNBddData *data, int nodeIdx, unsigned int timeStart, unsigned int numSteps)
{
    double *nodeValues;
    unsigned char *computedNodes;
    double *high;
    double *low;
    double *rel;
    struct bddnode *node;
    unsigned int tIdx;
    unsigned long int vl;

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
    high = rbdKooNBddRvv(data, node->high, timeStart, numSteps);
    if (high == NULL) {
        return NULL;
    }
    /* Recursively evaluate the reliability of the low part of the current BDD Node */
    low  = rbdKooNBddRvv(data, node->low, timeStart, numSteps);
    if (low == NULL) {
        return NULL;
    }

    /* Retrieve the reliability curve associated with the variable */
    rel = &data->bddmgr->vars[node->var].reliability[timeStart];

    /* For each time instant to be evaluated (blocks of N time instants)... */
    tIdx = 0;
    while (tIdx < numSteps) {
        vl = __riscv_vsetvl_e64m1(numSteps - tIdx);
        /* Compute the (cached) reliability curve associated with the current BDD Node */
        rbdKooNBddStepVNdRvv(&rel[tIdx], &high[tIdx], &low[tIdx], &nodeValues[tIdx], vl);
        /* Increment current time instant */
        tIdx += vl;
    }

    /* Set the BDD Node as already evaluated */
    computedNodes[nodeIdx] = 1;

    return nodeValues;
}


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */
