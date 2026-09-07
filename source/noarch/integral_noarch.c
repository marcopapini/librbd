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
 *  It is responsible to compute $\int_0^t{f_{pri}(\tau) R_{swi}(\tau) R_{sec}(t-\tau) d\tau}$,
 *  where f_{pri} is the failure density of the primary component, R_{swi} is the
 *  reliability of the switch component and R_{sec} is the reliability of
 *  the stand-by component.
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
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau) \cdot R_{sec}(t-\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         */
        s1dTmp = data->switchReliability[idx] *
                 data->primaryFailureDensity[idx] *
                 data->standbyReliabilityRev[startRevIdx + idx];

        /* Add the current product to the result using the Kahan's method */
        s1dY = s1dTmp - s1dC;
        s1dTSum = s1dSum + s1dY;
        s1dC = (s1dTSum - s1dSum) - s1dY;
        s1dSum = s1dTSum;
    }

    /**
     * Compute $R_{swi}(0) \cdot f_{pri}(0) \cdot R_{sec}(t)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->switchReliability[0] *
             data->primaryFailureDensity[0] *
             data->standbyReliabilityRev[startRevIdx];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dC = (s1dTSum - s1dSum) - s1dY;
    s1dSum = s1dTSum;

    /**
     * Compute $R_{swi}(t) \cdot f_{pri}(t) \cdot R_{sec}(0)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->switchReliability[time] *
             data->primaryFailureDensity[time] *
             data->standbyReliabilityRev[startRevIdx + time];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dC = (s1dTSum - s1dSum) - s1dY;

    /* Apply Kahan compensation to clean the accumulated sums */
    s1dSum = s1dTSum - s1dC;

    /* Multiply the partial result with the delta time */
    return s1dSum * data->deltaT;
}

/**
 * rbdIntegralHotStandbyS1d
 *
 * Compute integral for Hot Stand-by function
 *
 * Input:
 *      struct rbdHotStandbyData *data
 *      unsigned int time
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the integral for a Hot Stand-by RBD step.
 *  It is responsible to compute $\int_0^t{f_{pri}(\tau) R_{swi}(\tau)$,
 *  where f_{pri} is the failure density of the primary component and R_{swi}
 *  is the reliability of the switch component.
 *  To minimize the numerical error, this function uses the Kahan's method.
 *
 * Parameters:
 *      data: Hot Stand-by RBD data structure
 *      time: current time instant over which Hot Stand-by RBD shall be computed
 *
 * Return (double):
 *  The result of the integral for Hot Stand-by computation
 */
HIDDEN double rbdIntegralHotStandbyS1d(struct rbdHotStandbyData *data, unsigned int time)
{
    double s1dSum;
    double s1dC;
    double s1dY;
    double s1dTSum;
    double s1dTmp;
    unsigned int idx;

    s1dSum = 0.0;

    /* The integral if null (0.0) if the time domain is empty */
    if (time == 0) {
        return s1dSum;
    }

    s1dC = 0.0;

    /* For each internal time instant... */
    for (idx = 1; idx < time; ++idx) {
        /**
         * Compute $R_{swi}(\tau) \cdot f_{pri}(\tau)$
         * The weight is 1.0 (trapezoidal rule for internal time instants)
         */
        s1dTmp = data->switchReliability[idx] *
                 data->primaryFailureDensity[idx];

        /* Add the current product to the result using the Kahan's method */
        s1dY = s1dTmp - s1dC;
        s1dTSum = s1dSum + s1dY;
        s1dC = (s1dTSum - s1dSum) - s1dY;
        s1dSum = s1dTSum;
    }

    /**
     * Compute $R_{swi}(0) \cdot f_{pri}(0)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->switchReliability[0] *
             data->primaryFailureDensity[0];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dC = (s1dTSum - s1dSum) - s1dY;
    s1dSum = s1dTSum;

    /**
     * Compute $R_{swi}(t) \cdot f_{pri}(t)$
     * The weight is 0.5 (trapezoidal rule for external time instants)
     */
    s1dTmp = 0.5 * data->switchReliability[time] *
             data->primaryFailureDensity[time];

    /* Add the current product to the result using the Kahan's method */
    s1dY = s1dTmp - s1dC;
    s1dTSum = s1dSum + s1dY;
    s1dC = (s1dTSum - s1dSum) - s1dY;

    /* Apply Kahan compensation to clean the accumulated sums */
    s1dSum = s1dTSum - s1dC;

    /* Multiply the partial result with the delta time */
    return s1dSum * data->deltaT;
}
