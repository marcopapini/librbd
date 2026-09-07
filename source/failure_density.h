/*
 *  Component: failure_density.h
 *  Failure Density computation for RBD management
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

#ifndef FAILURE_DENSITY_H_
#define FAILURE_DENSITY_H_


/* Platform-generic and platform-specific functions */
void rbdFailureDensityWorker(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT);


/* Platform-generic functions */
void rbdFailureDensityWorkerNoarch(double *reliability, double *failureDensity, unsigned int numTimes, double deltaT);


#endif /* FAILURE_DENSITY_H_ */
