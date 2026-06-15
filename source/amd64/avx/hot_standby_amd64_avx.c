/*
 *  Component: hot_standby_amd64_avx.c
 *  Hot Stand-by RBD management - Optimized using amd64 AVX instruction set
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
#include "../hot_standby_amd64.h"
#include "../../x86/hot_standby_x86.h"


/**
 * rbdHotStandbyWorkerAvx
 *
 * Hot Stand-by RBD Worker function with amd64 AVX instruction set
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD Worker exploiting amd64 AVX instruction set.
 *  It is responsible to compute the reliabilities over a given batch of a Hot Stand-by RBD system
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *
 * Return (void *):
 *  NULL
 */
HIDDEN void *rbdHotStandbyWorkerAvx(struct rbdHotStandbyData *data)
{
    unsigned int time;

    /* Retrieve first time instant to be processed by worker */
    time = data->batchIdx * V4D;

    /* For each time instant to be processed (blocks of 4 time instants)... */
    while ((time + V4D) <= data->numTimes) {
        /* Prefetch for next iteration */
        prefetchRead(data->primaryReliability, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchRead(data->standbyReliability, 1, data->numTimes, time + (data->numCores * V4D));
        prefetchWrite(data->output, 1, data->numTimes, time + (data->numCores * V4D));
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV4dAvx(data, time);
        /* Increment current time instant */
        time += (data->numCores * V4D);
    }
    /* Are (at least) 2 time instants remaining? */
    if ((time + V2D) <= data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV2dSse2(data, time);
        /* Increment current time instant */
        time += V2D;
    }
    /* Is 1 time instant remaining? */
    if (time < data->numTimes) {
        /* Compute reliability of Hot Stand-by RBD at current time instant */
        rbdHotStandbyStepV1dSse2(data, time);
    }

    return NULL;
}

/**
 * rbdHotStandbyStepV4dAvx
 *
 * Hot Stand-by RBD step function with amd64 AVX 256bit
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function implements the Hot Stand-by RBD step exploiting amd64 AVX 256bit.
 *  It is responsible to compute the reliability of a Hot Stand-by block
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 */
HIDDEN FUNCTION_TARGET("avx") void rbdHotStandbyStepV4dAvx(struct rbdHotStandbyData *data, unsigned int time)
{
    __m256d v4dTmp;
    __m256d v4dPri;
    __m256d v4dRes;

    /* Compute reliability of Hot Stand-by RBD at current time instant */
    v4dPri = _mm256_loadu_pd(&data->primaryReliability[time]);
    v4dRes = _mm256_set1_pd(data->pSwitch);
    v4dTmp = _mm256_loadu_pd(&data->standbyReliability[time]);
    v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    v4dTmp = _mm256_sub_pd(v4dOnes, v4dPri);
    v4dRes = _mm256_mul_pd(v4dRes, v4dTmp);
    v4dRes = _mm256_add_pd(v4dRes, v4dPri);

    /* Cap the computed reliability and set it into output array */
    _mm256_storeu_pd(&data->output[time], capReliabilityV4dAvx(v4dRes));
}


#endif /* defined(ARCH_AMD64) && (CPU_ENABLE_SIMD != 0) */
