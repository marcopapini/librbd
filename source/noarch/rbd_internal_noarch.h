/*
 *  Component: rbd_internal_noarch.c
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

#ifndef RBD_INTERNAL_NOARCH_H_
#define RBD_INTERNAL_NOARCH_H_


#define S1D                         (1)         /* Scalar of 1 double (64 bit)          */
#define V2D                         (2)         /* Vector of 2 doubles (128bit)         */
#define V4D                         (4)         /* Vector of 4 doubles (256bit)         */
#define V8D                         (8)         /* Vector of 8 doubles (512bit)         */


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


#endif /* RBD_INTERNAL_NOARCH_H_ */
