/*
 *  Component: rbd_internal_generic.h
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

#ifndef RBD_INTERNAL_GENERIC_H_
#define RBD_INTERNAL_GENERIC_H_


#include <math.h>
#include <stdlib.h>

#include "../compiler/compiler.h"
#include "architecture.h"


#define S1D                         (1)         /* Scalar of 1 double (64 bit)          */
#define V2D                         (2)         /* Vector of 2 doubles (128bit)         */
#define V4D                         (4)         /* Vector of 4 doubles (256bit)         */
#define V8D                         (8)         /* Vector of 8 doubles (512bit)         */


/*< If CPU_SMP flag has not been provided, disable SMP */
#ifndef CPU_SMP
#define CPU_SMP                         0
#endif /* CPU_SMP */

/*< If CPU_ENABLE_SIMD flag has not been provided, disable SIMD */
#ifndef CPU_ENABLE_SIMD
#define CPU_ENABLE_SIMD                 0
#endif /* CPU_ENABLE_SIMD */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#define MIN_BATCH_SIZE              (10000)     /* Minimum batch size in SMP RBD resolution */
#endif /* CPU_SMP */


typedef void *(*fpWorker)(void *);


/**
 * min
 *
 * Compute minimum between two numbers
 *
 * Input:
 *      int a
 *      int b
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the minimum between two numbers
 *
 * Parameters:
 *      a: first value for minimum computation
 *      b: second value for minimum computation
 *
 * Return (int):
 *  Minimum value
 */
static inline ALWAYS_INLINE int min(int a, int b) {
    return (a <= b) ? a : b;
}

/**
 * max
 *
 * Compute maximum between two numbers
 *
 * Input:
 *      int a
 *      int b
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the maximum between two numbers
 *
 * Parameters:
 *      a: first value for maximum computation
 *      b: second value for maximum computation
 *
 * Return (int):
 *  maximum value
 */
static inline ALWAYS_INLINE int max(int a, int b) {
    return (a >= b) ? a : b;
}

/**
 * floorDivision
 *
 * Compute floor value of division
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the floor value of the requested division
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Floor value of division
 */
static inline ALWAYS_INLINE int floorDivision(int dividend, int divisor) {
    return (dividend / divisor);
}

/**
 * ceilDivision
 *
 * Compute ceil value of division
 *
 * Input:
 *      int dividend
 *      int divisor
 *
 * Output:
 *      None
 *
 * Description:
 *  Computes the ceil value of the requested division
 *
 * Parameters:
 *      dividend: dividend of division
 *      divisor: divisor of division
 *
 * Return (int):
 *  Ceil value of division
 */
static inline ALWAYS_INLINE int ceilDivision(int dividend, int divisor) {
    return floorDivision(dividend + divisor - 1, divisor);
}


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
double capReliabilityS1d(double s1dR);

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
void prefetchRead(double *reliability, unsigned char numComponents, unsigned int numTimes, unsigned int time);

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
void prefetchWrite(double *reliability, unsigned char numComponents, unsigned int numTimes, unsigned int time);

/**
 * getCpuInfo
 *
 * Get CPU-specific information
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves CPU-specific information from the OS
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
void getCpuInfo(void);

/**
 * getNumberOfCores
 *
 * Number of cores retrieval in an SMP system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the number of cores in an SMP system
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  Number of cores in SMP system
 */
unsigned int getNumberOfCores(void);

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
int computeNumCores(int numTimes);
#endif /* CPU_SMP */


#endif /* RBD_INTERNAL_GENERIC_H_ */
