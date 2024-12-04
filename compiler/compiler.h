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
#else
#error "Unknown/unsupported compiler"
#endif


#if defined(COMPILER_NOT_DETECTED)
#undef COMPILER_NOT_DETECTED
#endif


#if     defined(COMPILER_CLANG)
#include "clang.h"
#elif   defined(COMPILER_GCC)
#include "gcc.h"
#endif



#endif /* COMPILER_H_ */
