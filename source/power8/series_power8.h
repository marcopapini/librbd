/*
 *  Component: series_power8.h
 *  Series RBD management - POWER8 platform-specific implementation
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

#ifndef SERIES_POWER8_H_
#define SERIES_POWER8_H_


#include "../generic/rbd_internal_generic.h"
#include "../series.h"


#if defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0)
/* Platform-specific functions for POWER8 VSX instruction set */
void *rbdSeriesGenericWorkerVsx(struct rbdSeriesData *data);
void *rbdSeriesIdenticalWorkerVsx(struct rbdSeriesData *data);

void rbdSeriesGenericStepV2dVsx(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepV2dVsx(struct rbdSeriesData *data, unsigned int time);
#endif /* defined(ARCH_POWER8) && (CPU_ENABLE_SIMD != 0) */


#endif /* SERIES_POWER8_H_ */
