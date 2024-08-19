/*
 *  Component: rbd_internal_genericch
 *  Internal APIs used by RBD library - Generic implementation
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


#include "../rbd_internal.h"


/**
 * capReliabilityS1d
 *
 * Cap reliability to accepted bounds [0.0, 1.0]
 *
 * Input:
 *      double s1dR
 *
 * Output:
 *      None
 *
 * Description:
 *  This function caps the provided reliability (scalar value, double-precision FP)
 *      to the accepted bounds
 *
 * Parameters:
 *      s1dR: Reliability
 *
 * Return (double):
 *  Reliability within accepted bounds
 */
__attribute__((visibility ("hidden"))) double capReliabilityS1d(double s1dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    if ((isnan(s1dR) != 0) || (s1dR < 0.0)) {
        return 0.0;
    }
    else if (s1dR > 1.0) {
        return 1.0;
    }
    return s1dR;
}

/**
 * prefetchRead
 *
 * Prefetch reliability for read
 *
 * Input:
 *      double *reliability
 *      unsigned char numComponents
 *      unsigned int numTimes
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetch reliability for read operations
 *
 * Parameters:
 *      reliability: reliability to be fetched
 *      numComponents: number of components in reliability matrix
 *      numTimes: number of times in reliability matrix
 *      time: current time to prefetch
 */
__attribute__((visibility ("hidden"))) void prefetchRead(double *reliability, unsigned char numComponents, unsigned int numTimes, unsigned int time)
{
    int component = (int)numComponents;

    while (--component >= 0) {
        __builtin_prefetch(&reliability[(component * numTimes) + time], 0, 3);
    }
}

/**
 * prefetchWrite
 *
 * Prefetch reliability for write
 *
 * Input:
 *      double *reliability
 *      unsigned char numComponents
 *      unsigned int numTimes
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetch reliability for write operations
 *
 * Parameters:
 *      reliability: reliability to be fetched
 *      numComponents: number of components in reliability matrix
 *      numTimes: number of times in reliability matrix
 *      time: current time to prefetch
 */
__attribute__((visibility ("hidden"))) void prefetchWrite(double *reliability, unsigned char numComponents, unsigned int numTimes, unsigned int time)
{
    int component = (int)numComponents;

    while (--component >= 0) {
        __builtin_prefetch(&reliability[(component * numTimes) + time], 1, 3);
    }
}

#if CPU_SMP != 0
/**
 * computeNumCores
 *
 * Compute the number of cores in SMP system used to analyze RBD block
 *
 * Input:
 *      int numTimes
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the number of cores when SMP is used
 *
 * Parameters:
 *      numTimes: total number of time instants
 *
 * Return (int):
 *  Batch size
 */
__attribute__((visibility ("hidden"))) int computeNumCores(int numTimes) {
    int numCores, batchSize;

    /* Retrieve number of cores available in SMP system */
    numCores = getNumberOfCores();
    /* Compute batch size */
    batchSize = max(ceilDivision(numTimes, numCores), MIN_BATCH_SIZE);
    /* Compute number of threads required */
    numCores = ceilDivision(numTimes, batchSize);
    /* Return number of threads required */
    return numCores;
}
#endif /* CPU_SMP */
