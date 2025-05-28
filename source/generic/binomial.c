/*
 *  Component: binomial.c
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


#include "binomial.h"

#include "../compiler/compiler.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>


static unsigned char computeGcd(unsigned long long a, unsigned char b);
static void divideBy(unsigned long long *res, unsigned char *divisors, unsigned char numDivisors);


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
HIDDEN unsigned long long binomialCoefficient(unsigned char n, unsigned char k)
{
    unsigned long long res = 1;
    int idx;
    int i;
    unsigned char divisors[SCHAR_MAX];

    /* In case k is greater than n, nCk computation is not possible. Return 0 */
    if (k > n) {
        return 0ULL;
    }

    /* 0C0 is equal to 1. */
    if (n == 0) {
        return 1ULL;
    }

    /* nCk is equal to nC(n-k). Resolve for k=min(k, n-k) */
    if ((n - k) < k) {
        k = n - k;
    }

    /* nC0 is equal to 1. */
    if (k == 0) {
        return 1ULL;
    }

    /* Initialize array values from 1 to k */
    for (idx = k - 1; idx >= 0; --idx) {
        divisors[idx] = idx + 1;
    }

    i = k - 1;
    res = 1ULL;

    /* For i=0, ..., k-1 */
    do {
        if (res > (18446744073709551615ULL / (n - i))) {
            /* Overflow condition detected, return 0. */
            return 0ULL;
        }
        /* Multiply partial result by (n-i) */
        res *= (n - i);
        /* Divide partial result using all divisors */
        divideBy(&res, divisors, k);
    }
    while (i-- > 0);

    /* Divide partial result by remaining divisors */
    for (idx = k - 1; idx >= 0; --idx) {
        res /= divisors[idx];
    }

    /* Return nCk */
    return res;
}

/**
 * computeGcd
 *
 * Computation of Greatest Common Divisor
 *
 * Input:
 *      unsigned long long a
 *      unsigned char b
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the Greatest Common Divisor GCD.
 *
 * Parameters:
 *      a: first parameter for GCD computation
 *      b: second parameter for GCD computation
 *
 * Return (unsigned char):
 *  The computed GCD.
 */
static unsigned char computeGcd(unsigned long long a, unsigned char b)
{
    unsigned int temp;

    /* Compute GCD(a,b) using Euclid's algorithm */
    while (b != 0) {
        temp = b;
        b = a % b;
        a = temp;
    }

    return (unsigned char)a;
}

/**
 * divideBy
 *
 * Divide partial result by array of divisors avoiding numerical errors
 *
 * Input:
 *      unsigned char *divisors
 *      unsigned char numDivisors
 *
 * Output:
 *      unsigned long long *res
 *
 * Description:
 *  This function divides partial result by array of divisors avoiding numerical errors
 *
 * Parameters:
 *      res: partial result
 *      divisors: array of divisors
 *      numDivisors: number of divisors
 *
 * Return: None
 */
static void divideBy(unsigned long long *res, unsigned char *divisors, unsigned char numDivisors)
{
    int idx;
    unsigned char gcd;

    /* For each divisor... */
    for (idx = numDivisors - 1; idx >= 0; --idx) {
        /* Compute GCD between partial result and current divisor */
        gcd = computeGcd(*res, divisors[idx]);
        /* Divide partial result by GCD */
        *res /= gcd;
        /* Update current divisor by performing division by GCD */
        divisors[idx] /= gcd;
    }
}
