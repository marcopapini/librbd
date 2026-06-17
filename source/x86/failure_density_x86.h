/*
 *  Component: failure_density_x86.h
 *  Failure Density computation for RBD management - x86 platform-specific implementation
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

#ifndef FAILURE_DENSITY_X86_H_
#define FAILURE_DENSITY_X86_H_


#include "../generic/rbd_internal_generic.h"
#include "../failure_density.h"


#if (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for x86 SSE2 instruction set */
void rbdFailureDensityWorkerSse2(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT);
#endif /* (defined(ARCH_X86) || defined(ARCH_AMD64)) && (CPU_ENABLE_SIMD != 0) */


#endif /* FAILURE_DENSITY_X86_H_ */
