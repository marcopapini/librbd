/*
 *  Component: reliability.c
 *  Reliability RBD management
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


#include "generic/rbd_internal_generic.h"

#include "reliability.h"


/**
 * rbdUnreliability
 *
 * Compute unreliability given the RBD reliability curve
 *
 * Input:
 *      double *reliability
 *      unsigned int numTimes
 *
 * Output:
 *      double *unreliability
 *
 * Description:
 *  This function computes the unreliability curve over time given its corresponding
 *  reliability curve
 *
 * Parameters:
 *      reliability: this array contains the input reliability curve at the provided
 *                      time instants
 *      unreliability: this array is filled with the unreliability curve at the
 *                      provided time instants
 *      numTimes: number of time instants over which unreliability curve shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN double *rbdUnreliability(double *reliability, double *unreliability, unsigned int numTimes)
{
    struct rbdUnreliabilityData data;

    /* Prepare Unreliability RBD data structure */
    data.reliability = reliability;
    data.unreliability = unreliability;
    data.numTimes = numTimes;

    /* Invoke the Unreliability RBD Worker */
    (void)rbdUnreliabilityWorker(&data);

    return 0;
}

/**
 * rbdReliabilitySum
 *
 * Compute the sum of two reliability curves
 *
 * Input:
 *      double *r1
 *      double *r2
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the sum of the two provided reliability curves over time
 *
 * Parameters:
 *      r1: this array contains the first input reliability curve at the provided
 *                      time instants
 *      r2: this array contains the second input reliability curve at the provided
 *                      time instants
 *      output: this array is filled with the sum of the reliability curves at the
 *                      provided time instants
 *      numTimes: number of time instants over which unreliability curve shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN double *rbdReliabilitySum(double *r1, double *r2, double *output, unsigned int numTimes)
{
    struct rbdReliabilityData data;

    /* Prepare Reliability RBD data structure */
    data.r1 = r1;
    data.r2 = r2;
    data.output = output;
    data.numTimes = numTimes;

    /* Invoke the Reliability Sum RBD Worker */
    (void)rbdReliabilitySumWorker(&data);

    return 0;
}
