/*
 *  Component: xlc.h
 *  Compiler management - XL C compiler
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

#ifndef XLC_H_
#define XLC_H_


#include "compiler.h"

#if defined(COMPILER_XLC)


/* Update architecture target for functions and variables */
#define FUNCTION_TARGET(X)
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
 *  This function prefetches memory for read access using XL C
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
static inline ALWAYS_INLINE void compilerPrefetchRead(void *address)
{
    __dcbtt(address);
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
 *  This function prefetches memory for write access using XL C
 *
 * Parameters:
 *      address: memory address to prefetch
 *
 * Return:
 *  None
 */
static inline ALWAYS_INLINE void compilerPrefetchWrite(void *address)
{
    __dcbtstt(address);
}

#endif /* defined(COMPILER_XLC) */


#endif /* XLC_H_ */
