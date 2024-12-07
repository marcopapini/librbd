/*
 *  Component: gcc.h
 *  Compiler management - GCC compiler
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

#ifndef GCC_H_
#define GCC_H_


#include "compiler.h"

#if defined(COMPILER_GCC)


/* Update architecture target for functions and variables */
#define FUNCTION_TARGET(X)      __attribute__((__target__(X)))
#define VARIABLE_TARGET(X)

#define ALWAYS_INLINE           __attribute__((always_inline))

/* Declare hidden symbols */
#define HIDDEN                  __attribute__((visibility ("hidden")))

/**
 * compilerPrefetchRead
 *
 * Prefetch memory for read access
 *
 * Input:
 *      void *address
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetches memory for read access using GCC
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
static inline ALWAYS_INLINE void compilerPrefetchRead(void *address)
{
    __builtin_prefetch(address, 0, 3);
}

/**
 * compilerPrefetchWrite
 *
 * Prefetch memory for write access
 *
 * Input:
 *      void *address
 *
 * Output:
 *      None
 *
 * Description:
 *  This function prefetches memory for write access using GCC
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
static inline ALWAYS_INLINE void compilerPrefetchWrite(void *address)
{
    __builtin_prefetch(address, 1, 3);
}

#endif /* defined(COMPILER_GCC) */


#endif /* GCC_H_ */
