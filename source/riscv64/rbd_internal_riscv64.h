/*
 *  Component: rbd_internal_riscv64.h
 *  Internal APIs used by RBD library - Optimized using RISC-V 64bit platform-specific instruction sets
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

#ifndef RBD_INTERNAL_RISCV_H_
#define RBD_INTERNAL_RISCV_H_


#if defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0)


#include <riscv_vector.h>
#include <limits.h>
#include <string.h>


#define RVV_MAX_VECTOR_SIZE     8192        /* RISC-V 64bit RVV supports vectors of 65536 bits (8192 bytes) */


struct rbdKooNRecursionData
{
    unsigned char comb[SCHAR_MAX + 1];      /* Array for the computation of KooN combinations */
    unsigned char buff[(UCHAR_MAX + 1) * RVV_MAX_VECTOR_SIZE];  /* Temporary buffer */
    double      *s1dR;                      /* Pointer to array of reliabilities - Scalar 1 double */
};


vfloat64m1_t capReliabilityVNdRvv(vfloat64m1_t vR, unsigned long int vl);


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
    alignAddr = ((unsigned long long)(&data->buff) + sizeof(double) - 1) & ~(sizeof(double) - 1);
    data->s1dR = (double *)alignAddr;
}


/**
 * riscv64RvvSupported
 *
 * RVV instruction set supported by the RISC-V 64bit system
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the availability of RVV instruction set
 *
 * Parameters:
 *      None
 *
 * Return (unsigned int):
 *  1 if RVV instruction set is available, 0 otherwise
 */
unsigned int riscv64RvvSupported(void);

/**
 * retrieveRiscv64CpuInfo
 *
 * Retrieve RISC-V 64bit-specific CPU info (supported instruction sets)
 *
 * Input:
 *      None
 *
 * Output:
 *      None
 *
 * Description:
 *  This function retrieves the following information:
 *  - If RVV instruction set is supported
 *
 * Parameters:
 *      None
 *
 * Return:
 *      None
 */
void retrieveRiscv64CpuInfo(void);


#endif /* defined(ARCH_RISCV64) && (CPU_ENABLE_SIMD != 0) */


#endif /* RBD_INTERNAL_RISCV_H_ */
