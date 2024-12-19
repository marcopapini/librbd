/*
 *  Component: vs.h
 *  Compiler management - Visual Studio compiler
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

#ifndef VS_H_
#define VS_H_


#include "compiler.h"

#if defined(COMPILER_VS)

#include <windows.h>


/* Update architecture target for functions and variables */
#define FUNCTION_TARGET(X)
#define VARIABLE_TARGET(X)

#define ALWAYS_INLINE

/* Declare hidden symbols */
#define HIDDEN

/* Prefetch data for read/write into cache */
void compilerPrefetchRead(void *address);
void compilerPrefetchWrite(void *address);

#endif /* defined(COMPILER_VS) */


#endif /* VS_H_ */
