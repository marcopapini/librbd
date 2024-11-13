/*
 *  Component: series_aarch64.h
 *  Series RBD management - AArch64 platform-specific implementation
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

#ifndef SERIES_AARCH64_H_
#define SERIES_AARCH64_H_


#include "../generic/rbd_internal_generic.h"
#include "../series.h"


#if CPU_AARCH64_NEON != 0
/* Platform-specific functions for AArch64 NEON instruction set */
void rbdSeriesGenericStepV2dNeon(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepV2dNeon(struct rbdSeriesData *data, unsigned int time);
#endif /* CPU_AARCH64_NEON */


#endif /* SERIES_AARCH64_H_ */
