/*
 *  Component: binomial.h
 *  Binomial Coefficient computation
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

#ifndef BINOMIAL_H_
#define BINOMIAL_H_


/**
 * binomialCoefficient
 *
 * Computation of Binomial Coefficient
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the Binomial Coefficient nCk.
 *  In case the nCk computation encounters an error, this function returns 0.
 *
 * Parameters:
 *      n: n parameter of nCk
 *      k: k parameter of nCk. RANGE: (0 <= k <= n)
 *
 * Return (unsigned long long):
 *  The computed nCk if no error is encountered, 0 otherwise.
 */
unsigned long long binomialCoefficient(unsigned char n, unsigned char k);


#endif /* BINOMIAL_H_ */
