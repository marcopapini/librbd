/*
 *  Component: rbd_internal.h
 *  Internal APIs used by RBD library
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020 by Marco Papini <papini.m@gmail.com>
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


/*< If CPU_SMP flag has not been provided, enable SMP */
#ifndef CPU_SMP
#define CPU_SMP                     1
#endif

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
#define MIN_BATCH_SIZE              (10000)     /* Minimum batch size in SMP RBD resolution */
#endif  /* CPU_SMP */



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
static inline int min(int a, int b) {
    return (a <= b) ? a : b;
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
static inline int floorDivision(int dividend, int divisor) {
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
static inline int ceilDivision(int dividend, int divisor) {
    return floorDivision((dividend + divisor - 1), divisor);
}


/**
 * postProcessReliability
 *
 * Post-process output Reliability, i.e. ensure that it is non-increasing
 *
 * Input:
 *      double *reliability
 *      unsigned int numTimes
 *
 * Output:
 *      double *reliability
 *
 * Description:
 *  This function ensures that the output Reliability is non-increasing
 *
 * Parameters:
 *      reliability: pointer to output Reliability
 *      numTimes: number of time instants over which output Reliability has been sampled
 *
 * Return:
 *  None
 */
void postProcessReliability(double *reliability, unsigned int numTimes);


#if CPU_SMP != 0                                /* Under SMP conditional compiling */
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
#endif  /* CPU_SMP */


#endif  /* RBD_INTERNAL_H_ */
