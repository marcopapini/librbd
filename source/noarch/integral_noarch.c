/*
 *  Component: integral_noarch.c
 *  Compute integral for RBD management - Platform-independent implementation
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


#include "../generic/rbd_internal_generic.h"

#include "../integral.h"


/**
 * rbdIntegralColdStandbyS1d
 *
 * Compute integral for Cold Stand-by function
 *
 * Input:
 *      struct rbdColdStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the integral for a Cold Stand-by RBD step.
 *  It is responsible to compute $\int_0^t{f_P(\tau) R_S(t-\tau) d\tau}$,
 *  where f_P is the failure density of the primary component and R_S is
 *  the reliability of the stand-by component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Cold Stand-by RBD data structure
 *      time: current time instant over which Cold Stand-by RBD shall be computed
 *
 * Return (double):
 *  The result of the integral for Cold Stand-by computation
 */
HIDDEN double rbdIntegralColdStandbyS1d(struct rbdColdStandbyData *data, unsigned int time)
{
    double s1dSum;
    double s1dC;
    double s1dY;
    double s1dTSum;
    double s1dTmp;
    unsigned int idx;
    unsigned int startRevIdx;

    s1dSum = 0.0;

    /* The integral if null (0.0) if the time domain is empty */
    if (time == 0) {
        return s1dSum;
    }

    s1dC = 0.0;

    /* Compute the first index to be used over the reversed stand-by reliability */
    startRevIdx = data->numTimes - 1 - time;

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        /**
         * Compute $f_P(\tau) \cdot R_S(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         */
        s1dTmp = data->primaryFailureDensity[idx] * data->standbyReliabilityRev[startRevIdx + idx];

        /* Add the current product to the result using the Kahan's method */
        s1dY = s1dTmp - s1dC;
        s1dTSum = s1dSum + s1dY;
        s1dC = (s1dTSum - s1dSum) - s1dY;
        s1dSum = s1dTSum;
    }

    /**
     * Compute $f_P(0) \cdot R_S(t)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->primaryFailureDensity[0] * data->standbyReliabilityRev[startRevIdx];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dC = (s1dTSum - s1dSum) - s1dY;
    s1dSum = s1dTSum;

    /**
     * Compute $f_P(t) \cdot R_S(0)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->primaryFailureDensity[time] * data->standbyReliabilityRev[startRevIdx + time];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dSum = s1dTSum;

    /* Multiply the partial result with the delta time */
    return s1dSum * data->deltaT;
}
