/*
 *  Component: compiler.h
 *  Compiler management
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

#ifndef COMPILER_H_
#define COMPILER_H_


#if     defined(__clang__)
/* clang compiler */
#define COMPILER_CLANG
#elif   defined(__GNUC__)
/* GCC compiler */
#define COMPILER_GCC
#elif   defined(_MSC_VER)
/* Visual Studio compiler */
#define COMPILER_VS
#elif   defined(__ibmxl__)
/* IBM XL C compiler */
#define COMPILER_XLC
#else
#error "Unknown/unsupported compiler"
#endif

#if     defined(COMPILER_CLANG)
#include "clang.h"
#elif   defined(COMPILER_GCC)
#include "gcc.h"
#elif   defined(COMPILER_VS)
#include "vs.h"
#elif   defined(COMPILER_XLC)
#include "xlc.h"
#endif

typedef void* (*fpWorker)(void*);

#if CPU_SMP != 0
void *allocateThreadHandles(unsigned int numThreads);
int createThread(void *threadHandles, unsigned int threadIdx, fpWorker fpWorker, void *args);
void waitThread(void *threadHandles, unsigned int threadIdx);
#endif /* CPU_SMP != 0 */


#endif /* COMPILER_H_ */
