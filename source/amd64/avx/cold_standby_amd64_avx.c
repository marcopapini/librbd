/*
 *  Component: cold_standby_amd64_avx.c
 *  Cold Stand-by RBD management - Optimized using amd64 AVX instruction set
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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
#include "../../x86/cold_standby_x86.h"


/**
 * rbdColdStandbyWorkerAvx
 *
 * Cold Stand-by RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Cold Stand-by RBD system
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdColdStandbyWorkerAvx(struct rbdColdStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchRead(data->primaryFailureDensity, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchRead(data->standbyReliabilityRev, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Cold Stand-by RBD at current time instant */
        rbdColdStandbyStepV1dAvx(data, time);
    }

    return NULL;
}

/**
 * rbdColdStandbyStepV4dAvx
 *
 * Cold Stand-by RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Cold Stand-by block
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdColdStandbyStepV4dAvx(struct rbdColdStandbyData *data, unsigned int time)
{
    __m256d v4dTmp;
    __m256d v4dPri;
    __m256d v4dRes;

    /* Compute reliability of Cold Stand-by RBD at current time instant */
    v4dPri = _mm256_loadu_pd(&data->primaryReliability[time]);
    v4dTmp = rbdIntegralColdStandbyV4dAvx(data, time);
    v4dRes = _mm256_add_pd(v4dTmp, v4dPri);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}

/**
 * rbdColdStandbyStepV1dAvx
 *
 * Cold Stand-by RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Cold Stand-by RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Cold Stand-by block
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdColdStandbyStepV1dAvx(struct rbdColdStandbyData *data, unsigned int time)
{
    __m128d v2dTmp;
    __m128d v2dPri;
    __m128d v2dRes;

    /* Compute reliability of Cold Stand-by RBD at current time instant */
    v2dPri = _mm_load_sd(&data->primaryReliability[time]);
    v2dTmp = _mm256_castpd256_pd128(rbdIntegralColdStandbyV1dAvx(data, time));
    v2dRes = _mm_add_sd(v2dTmp, v2dPri);

    /* Cap the computed reliability and set it into output array */
    _mm_store_sd(&data->output[time], capReliabilityV2dSse2(v2dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
