/*
 *  Component: cold_standby_amd64_avx512f.c
 *  Cold Stand-by RBD management - Optimized using amd64 AVX512F instruction set
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
#include "../cold_standby_amd64.h"
#include "../integral_amd64.h"


/**
 * rbdColdStandbyWorkerAvx512f
 *
 * Cold Stand-by RBD Worker function with amd64 AVX512F instruction set
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD Worker exploiting amd64 AVX512F instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Cold Stand-by RBD system
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN FUNCTION_TARGET("avx512f") void *rbdColdStandbyWorkerAvx512f(struct rbdColdStandbyData *data)
{
    unsigned int time;
    __mmask8 mask;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V8D;

    /* For each time instant to be processed (blocks of 8 time instants)... */
    while ((time + V8D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchRead(data->primaryFailureDensity, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchRead(data->standbyReliabilityRev, 1, data->numTimes, time + (data->numCores * V8D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V8D));
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepVNdAvx512f((__mmask8)0xFFU, data, time);
        /* Increment current time instant */
        time += (data->numCores * V8D);
    }
    /* Is (at least) 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute mask for the management of the tail */
        mask = (__mmask8)_cvtu32_mask16((1U << (data->numTimes - time)) - 1);
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepVNdAvx512f(mask, data, time);
    }

    return NULL;
}

/**
 * rbdColdStandbyStepVNdAvx512f
 *
 * Cold Stand-by RBD step function with amd64 AVX512F 512bit
 *
 * Input:
 *      __mmask8 mask
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD step exploiting amd64 AVX512F 512bit.
 *  It is responsible to compute the reliability of a Cold Stand-by block
 *
 * Parameters:
 *      mask: AVX512F 8-bit mask used during current step
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx512f") void rbdColdStandbyStepVNdAvx512f(__mmask8 mask, struct rbdColdStandbyData *data, unsigned int time)
{
    __m512d vNdTmp;
    __m512d vNdPri;
    __m512d vNdRes;

    /* Compute reliability of Cold Stand-by RBD at current time instant */
    vNdPri = _mm512_maskz_loadu_pd(mask, &data->primaryReliability[time]);
    vNdRes = _mm512_maskz_mov_pd(mask, _mm512_set1_pd(data->pSwitch));
    vNdTmp = rbdIntegralColdStandbyVNdAvx512f(mask, data, time);
    vNdRes = _mm512_maskz_fmadd_pd(mask, vNdRes, vNdTmp, vNdPri);

    /* Cap the computed reliability and set it into output array */
    _mm512_mask_storeu_pd(&data->output[time], mask, capReliabilityVNdAvx512f(mask, vNdRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
