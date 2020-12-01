/*
 *  Component: rbd_internal.c
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


#include "rbd_internal.h"


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
void postProcessReliability(double *reliability, unsigned int numTimes)
{
    unsigned int idx1, idx2;

    /* For each consecutive couple of time instants */
    for (idx1 = 0, idx2 = 1; idx2 < numTimes; idx1++, idx2++) {
        /* Is Reliability of second instant greater than Reliability of first one? */
        if (reliability[idx2] > reliability[idx1]) {
            /* Limit Reliability of second instant to the Reliability of the first one */
            reliability[idx2] = reliability[idx1];
        }
    }
}
