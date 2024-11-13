/*
 *  Component: combinations.h
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

#ifndef COMBINATIONS_H_
#define COMBINATIONS_H_


struct combinations
{
    unsigned long long numCombinations;     /* Number of combinations of k elements out of n (nCk) */
    unsigned char n;                        /* Dimension of set n */
    unsigned char k;                        /* Dimension of subsets k */
    unsigned char buff[];                   /* Array of combinations: each combination is an array of k elements */
} __attribute__((packed));


/**
 * computeCombinations
 *
 * Computation of combinations of k elements out of n
 *
 * Input:
 *      unsigned char n
 *      unsigned char k
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the combinations of k elements out of n.
 *  In case the nCk computation encounters an error, this function returns 0.
 *  This code is based on Rosetta Code Combinations: C code for Lexicographic ordered generation.
 *  https://rosettacode.org/wiki/Combinations#Lexicographic_ordered_generation
 *
 * Parameters:
 *      n: n parameter of nCk
 *      k: k parameter of nCk. RANGE: (0 <= k <= n)
 *
 * Return (struct combinations *):
 *  The structure containing computed nCk combinations of k out of n elements, NULL if an error
 *  occurred.
 */
struct combinations *computeCombinations(unsigned char n, unsigned char k);

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
void firstCombination(unsigned char k, unsigned char *combination);

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
int nextCombination(unsigned char n, unsigned char k, unsigned char *combination);


#endif /* COMBINATIONS_H_ */
