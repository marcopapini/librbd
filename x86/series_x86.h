/*
 *  Component: series_x86.h
 *  Series RBD management - x86 platform-specific implementation
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

#ifndef SERIES_X86_H_
#define SERIES_X86_H_


#include "../rbd_internal.h"
#include "../series.h"


#if CPU_X86_SSE2 != 0
/* Platform-specific functions for x86 SSE2 instruction set */
void rbdSeriesGenericStepV2dSse2(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepV2dSse2(struct rbdSeriesData *data, unsigned int time);
#endif /* CPU_X86_SSE2 */

#if CPU_X86_AVX != 0
/* Platform-specific functions for x86 AVX instruction set */
void rbdSeriesGenericStepV4dAvx(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepV4dAvx(struct rbdSeriesData *data, unsigned int time);
#endif /* CPU_X86_AVX */

#if CPU_X86_AVX512F != 0
/* Platform-specific functions for x86 AVX512F instruction set */
void rbdSeriesGenericStepV8dAvx512f(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepV8dAvx512f(struct rbdSeriesData *data, unsigned int time);
#endif /* CPU_X86_AVX512F */


#endif /* SERIES_X86_H_ */
