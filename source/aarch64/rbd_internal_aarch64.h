/*
 *  Component: rbd_internal_aarch64.h
 *  Internal APIs used by RBD library - Optimized using AArch64 platform-specific instruction sets
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

#ifndef RBD_INTERNAL_AARCH64_H_
#define RBD_INTERNAL_AARCH64_H_


#if defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0)


#include <arm_neon.h>
#include <arm_sve.h>
#include <limits.h>
#include <string.h>


#define SVE_MAX_VECTOR_SIZE     256         /* AArch64 SVE supports vectors of 2048 bits (256 bytes) */


struct rbdKooNRecursionData
{
    unsigned char comb[SCHAR_MAX + 1];      /* Array for the computation of KooN combinations */
    unsigned char buff[(UCHAR_MAX + 1) * SVE_MAX_VECTOR_SIZE];  /* Temporary buffer */
    double      *s1dR;                      /* Pointer to array of reliabilities - Scalar 1 double */
    float64x2_t *v2dR;                      /* Pointer to array of reliabilities - Vector 2 double */
};


VARIABLE_TARGET("+simd") extern const float64x2_t v2dZeros;
VARIABLE_TARGET("+simd") extern const float64x2_t v2dOnes;
VARIABLE_TARGET("+simd") extern const float64x2_t v2dTwos;
VARIABLE_TARGET("+simd") extern const float64x2_t v2dMinusTwos;


float64x2_t capReliabilityV2dNeon(float64x2_t v2dR);
svfloat64_t capReliabilityVNdSve(svbool_t pg, svfloat64_t vNdR);


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
    alignAddr = ((unsigned long long)(&data->buff) + sizeof(float64x2_t) - 1) & ~(sizeof(float64x2_t) - 1);
    data->s1dR = (double *)alignAddr;
    data->v2dR = (float64x2_t *)alignAddr;
}


/**
 * aarch64SveSupported
 *
 * SVE instruction set supported by the AArch64 system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of SVE instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if SVE instruction set is available, 0 otherwise
 */
unsigned int aarch64SveSupported(void);

/**
 * retrieveAarch64CpuInfo
 *
 * Retrieve AArch64-specific CPU info (supported instruction sets)
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - If SVE instruction set is supported
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
void retrieveAarch64CpuInfo(void);


#endif /* defined(ARCH_AARCH64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_AARCH64_H_ */
