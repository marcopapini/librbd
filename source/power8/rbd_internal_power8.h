/*
 *  Component: rbd_internal_power8.h
 *  Internal APIs used by RBD library - Optimized using POWER8 platform-specific instruction sets
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

#ifndef RBD_INTERNAL_POWER8_H_
#define RBD_INTERNAL_POWER8_H_


#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)


#include <altivec.h>
#include <limits.h>
#include <string.h>


#ifdef COMPILER_GCC
typedef __vector double double64x2;
#else
typedef vector double double64x2;
#endif


struct rbdKooNRecursionData
{
    unsigned char comb[SCHAR_MAX + 1];      /* Array for the computation of KooN combinations */
    unsigned char buff[(UCHAR_MAX + 1) * sizeof(double64x2)];  /* Temporary buffer */
    double      *s1dR;                      /* Pointer to array of reliabilities - Scalar 1 double */
    double64x2  *v2dR;                      /* Pointer to array of reliabilities - Vector 2 double */
};


VARIABLE_TARGET("vsx") extern const double64x2 v2dZeros;
VARIABLE_TARGET("vsx") extern const double64x2 v2dOnes;
VARIABLE_TARGET("vsx") extern const double64x2 v2dTwos;


/**
 * initKooNRecursionData
 *
 * Initialize the provided RBD KooN Recursive Data
 *
 * Input:
 *      None
 *
 * Output:
 *      struct rbdKooNRecursionData *data
 *
 * Description:
 *  This function initializes the provided RBD KooN Recursive Data
 *
 * Parameters:
 *      data: RBD KooN Recursive Data to be initialized
 */
static inline ALWAYS_INLINE void initKooNRecursionData(struct rbdKooNRecursionData *data) {
    unsigned long long alignAddr;
    memset(data, 0, sizeof(struct rbdKooNRecursionData));
    alignAddr = ((unsigned long long)(&data->buff) + sizeof(double64x2) - 1) & ~(sizeof(double64x2) - 1);
    data->s1dR = (double *)alignAddr;
    data->v2dR = (double64x2 *)alignAddr;
}


/**
 * vectorLoad
 *
 * Load a vector of doubles with POWER8 VSX 128bit
 *
 * Input:
 *      double *addr
 *
 * Output:
 *      None
 *
 * Description:
 *  This function loads a vector of doubles from the requested address exploiting POWER8 VSX 128bit
 *
 * Parameters:
 *      addr: Effective address from which vector of doubles is loaded
 *
 * Return (double64x2):
 *  Loaded vector
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("vsx") double64x2 vectorLoad(double *addr) {
#ifdef COMPILER_GCC
    return vec_vsx_ld(0, addr);
#else
    return vec_xl(0, addr);
#endif
}


/**
 * vectorStore
 *
 * Store a vector of doubles with POWER8 VSX 128bit
 *
 * Input:
 *      double *addr
 *      double64x2 data
 *
 * Output:
 *      None
 *
 * Description:
 *  This function stores the provided vector of doubles to the requested address exploiting POWER8 VSX 128bit
 *
 * Parameters:
 *      addr: Effective address to which vector of doubles is stored
 *      data: Vector of doubles to be stored
 */
static inline ALWAYS_INLINE FUNCTION_TARGET("vsx") void vectorStore(double *addr, double64x2 data) {
#ifdef COMPILER_GCC
    return vec_vsx_st(data, 0, addr);
#else
    return vec_xst(data, 0, addr);
#endif
}


FUNCTION_TARGET("vsx") double64x2 capReliabilityV2dVsx(double64x2 v2dR);


#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_POWER8_H_ */
