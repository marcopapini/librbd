/*
 *  Component: combinations.c
 *  Computation of combinations of k elements out of n
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


#include "combinations.h"

#include "../compiler/compiler.h"


/**
 * firstCombination
 *
 * Compute first combination of k elements out of n
 *
 * Input:
 *      unsigned char k
 *
 * Output:
 *      unsigned char *combination
 *
 * Description:
 *  This function computes the first combination of k elements out of n.
 *  This code is based on Rosetta Code Combinations: C code for Lexicographic ordered generation.
 *  https://rosettacode.org/wiki/Combinations#Lexicographic_ordered_generation
 *
 * Parameters:
 *      k: k parameter of nCk
 *      combination: buffer to be filled with first combination
 *
 * Return: None.
 */
HIDDEN void firstCombination(unsigned char k, unsigned char *combination)
{
    int i;

    /* Compute first combination */
    for (i = 0; i < k; i++) {
        combination[i] = i;
    }
}

/**
 * nextCombination
 *
 * Compute the next combination of k elements out of n starting from given combination
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      unsigned char *combination
 *
 * Description:
 *  This function computes the next combination of k elements out of n in lexicographic order.
 *  This code is based on Rosetta Code Combinations: C code for Lexicographic ordered generation.
 *  https://rosettacode.org/wiki/Combinations#Lexicographic_ordered_generation
 *
 * Parameters:
 *      n: n parameter of nCk
 *      k: k parameter of nCk
 *      combination: buffer to be filled with next combination in lexicographic order
 *
 * Return (int):
 *  0 in case the next combination in lexicographic order has been computed, -1 otherwise.
 */
HIDDEN int nextCombination(unsigned char n, unsigned char k, unsigned char *combination)
{
    int i;

    /* Set index of current element into temporary buffer to last element */
    i = k - 1;

    /**
     * Update combination: fast path
     * This check allows to speed up next combination computation in case last element
     * of combination buffer is not equal to n
     */
    if (++combination[i] < n) {
        return 0;
    }

    /* Search for first index into combinations buffer that can be updated */
    while (combination[i] >= n + i - k) {
        if (--i < 0) {
            /* All combinations have been computed, return -1 */
            return -1;
        }
    }
    /* Update combination: slow path */
    for (++combination[i]; i < k - 1; ++i) {
        combination[i + 1] = combination[i] + 1;
    }

    return 0;
}
