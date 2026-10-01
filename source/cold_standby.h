/*
 *  Component: cold_standby.h
 *  Cold Stand-by RBD management
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

#ifndef COLD_STANDBY_H_
#define COLD_STANDBY_H_


#include "../include/rbd.h"


/**
 * Data used during RBD Cold Stand-by computation
 */
struct rbdColdStandbyData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int numCores;              /* Number of threads in SMP system */
    double *primaryReliability;         /* Reliability of Primary RBD system (array) */
    double *primaryFailureDensity;      /* Failure Density of Primary RBD system (array) */
    double *standbyReliabilityRev;      /* Reliability of Stand-by RBD system (reversed array) */
    double *switchReliability;          /* Reliability of Switch RBD system (array) */
    double *output;                     /* Array of computed reliabilities */
    unsigned int numTimes;              /* Number of time instants to compute T */
    double deltaT;                      /* Time difference between two consecutive time instants */
};


/* Platform-generic and platform-specific functions */
void *rbdColdStandbyWorker(void *arg);

/* Platform-generic functions */
void *rbdColdStandbyWorkerNoarch(struct rbdColdStandbyData *data);


#endif /* COLD_STANDBY_H_ */
