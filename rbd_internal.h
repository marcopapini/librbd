/*
 *  Component: rbd_internal.h
 *  Internal APIs used by RBD library
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

#ifndef RBD_INTERNAL_H_
#define RBD_INTERNAL_H_


#include <math.h>
#include <stdlib.h>

#include "compiler/compiler.h"


#define V2D_SIZE                    (2)         /* Size of vector of 2 doubles (128bit) */
#define V4D_SIZE                    (4)         /* Size of vector of 4 doubles (256bit) */
#define V8D_SIZE                    (8)         /* Size of vector of 8 doubles (512bit) */


/*< If CPU_SMP flag has not been provided, disable SMP */
#ifndef CPU_SMP
#define CPU_SMP                         0
#endif /* CPU_SMP */

/*< If CPU_X86_SSE2 flag has not been provided, disable x86 SSW2 */
#ifndef CPU_X86_SSE2
#define CPU_X86_SSE2                    0
#endif /* CPU_X86_SSE2 */

/*< If CPU_X86_AVX flag has not been provided, disable x86 AVX */
#ifndef CPU_X86_AVX
#define CPU_X86_AVX                     0
#endif /* CPU_X86_AVX */

/*< If CPU_X86_FMA flag has not been provided, disable x86 FMA */
#ifndef CPU_X86_FMA
#define CPU_X86_FMA                     0
#endif /* CPU_X86_FMA */

/*< If CPU_X86_AVX512F flag has not been provided, disable x86 AVX512F */
#ifndef CPU_X86_AVX512F
#define CPU_X86_AVX512F                 0
#endif /* CPU_X86_AVX512F */

/*< If CPU_AARCH64_NEON flag has not been provided, disable AArch64 NEON */
#ifndef CPU_AARCH64_NEON
#define CPU_AARCH64_NEON                0
#endif /* CPU_AARCH64_NEON */

#if CPU_X86_SSE2 == 0
#if CPU_X86_AVX != 0
#error "AVX instruction set cannot be enabled without SSE2 instruction set!"
#endif /* CPU_X86_AVX */
#endif /* CPU_X86_SSE2 */

#if CPU_X86_AVX == 0
#if CPU_X86_FMA != 0
#error "FMA instruction set cannot be enabled without AVX instruction set!"
#endif /* CPU_X86_FMA */
#if CPU_X86_AVX512F != 0
#error "AVX512F instruction set cannot be enabled without AVX instruction set!"
#endif /* CPU_X86_AVX512F */
#endif /* CPU_X86_AVX */

#if CPU_X86_FMA == 0
#if CPU_X86_AVX512F != 0
#error "AVX512F instruction set cannot be enabled without FMA instruction set!"
#endif /* CPU_X86_AVX512F */
#endif /* CPU_X86_FMA */

#if CPU_X86_SSE2 != 0
#define DISABLE_GENERIC_FUNCTIONS
#endif /* CPU_X86_AVX */
#if CPU_AARCH64_NEON != 0
#define DISABLE_GENERIC_FUNCTIONS
#endif /* CPU_AARCH64_NEON */

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

/**
 * getX86Sse2Supported
 *
 * SSE2 instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of SSE2 instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if SSE2 instruction set is available, 0 otherwise
 */
unsigned int x86Sse2Supported(void);

/**
 * getX86AvxSupported
 *
 * AVX instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of AVX instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if AVX instruction set is available, 0 otherwise
 */
unsigned int x86AvxSupported(void);

/**
 * getFmaSupported
 *
 * FMA instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of FMA instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if FMA instruction set is available, 0 otherwise
 */
unsigned int x86FmaSupported(void);

/**
 * x86Avx512fSupported
 *
 * x86 AVX512F instruction set supported by the system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of x86 AVX512F instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if x86 AVX512F instruction set is available, 0 otherwise
 */
unsigned int x86Avx512fSupported(void);

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


#endif /* RBD_INTERNAL_H_ */
