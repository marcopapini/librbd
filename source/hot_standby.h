/*
 *  Component: hot_standby.h
 *  Hot Stand-by RBD management
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

#ifndef HOT_STANDBY_H_
#define HOT_STANDBY_H_


#include "../include/rbd.h"


/**
 * Data used during RBD Hot Stand-by computation
 */
struct rbdHotStandbyData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int numCores;              /* Number of threads in SMP system */
    double pSwitch;                     /* Probability that the switch is correctly executed */
    double *primaryReliability;         /* Reliability of Primary RBD system (array) */
    double *standbyReliability;         /* Reliability of Stand-by RBD system (array) */
    double *output;                     /* Array of computed reliabilities */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


/* Platform-generic and platform-specific functions */
void *rbdHotStandbyWorker(void *arg);

/* Platform-generic functions */
void *rbdHotStandbyWorkerNoarch(struct rbdHotStandbyData *data);

void rbdHotStandbyStepS1d(struct rbdHotStandbyData *data, unsigned int time);


#endif /* HOT_STANDBY_H_ */
