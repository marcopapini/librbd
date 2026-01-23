/*
 *  Component: rbd_internal_noarch.h
 *  Internal APIs used by RBD library - Platform-independent implementation
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


#include "rbd_internal_noarch.h"

#include "../generic/rbd_internal_generic.h"


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
HIDDEN double capReliabilityS1d(double s1dR) {
    /* Cap computed reliability to accepted bounds [0, 1] */
    if ((isnan(s1dR) != 0) || (s1dR < 0.0)) {
        return 0.0;
    }
    else if (s1dR > 1.0) {
        return 1.0;
    }
    return s1dR;
}
