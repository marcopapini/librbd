/*
 *  Component: integral.h
 *  Compute integral for RBD management
 *
 *  librbd - Reliability Block Diagrams evaluation library
 *  Copyright (C) 2020-2026 by Marco Papini <papini.m@gmail.com>
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

#ifndef INTEGRAL_H_
#define INTEGRAL_H_


#include "cold_standby.h"
#include "hot_standby.h"


/* Platform-generic functions */
double rbdIntegralColdStandbyS1d(struct rbdColdStandbyData *data, unsigned int time);
double rbdIntegralHotStandbyS1d(struct rbdHotStandbyData *data, unsigned int time);


#endif /* INTEGRAL_H_ */
