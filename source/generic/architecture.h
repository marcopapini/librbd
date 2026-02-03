/*
 *  Component: architecture.h
 *  Retrieval of target architecture
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

#ifndef ARCHITECTURE_H_
#define ARCHITECTURE_H_


#if   defined(i386)        || defined(__i386)   || \
      defined(__i386__)    || defined(_X86_)    || \
      defined(_M_IX86)
/* Intel/AMD x86 architecture */
#define ARCH_X86
#elif defined(__amd64__)   || defined(__amd64)  || \
      defined(__x86_64__)  || defined(__x86_64) || \
      defined(_M_X64)      || defined(_M_AMD64)
/* Intel/AMD amd64 architecture */
#define ARCH_AMD64
#elif defined(__aarch64__) || defined(_M_ARM64)
/* ARM aarch64 architecture */
#define ARCH_AARCH64
#elif defined(_ARCH_PWR8)  || defined(__POWER8_VECTOR__)
/* IBM POWER8 architecture */
#define ARCH_POWER8
#elif defined(__riscv)     && (__riscv_xlen == 64)
/* RISC-V 64bit architecture */
#define ARCH_RISCV64
#else
/* Unknown architecture */
#define ARCH_UNKNOWN
#endif


#endif /* ARCHITECTURE_H_ */
