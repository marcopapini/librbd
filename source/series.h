/*
 *  Component: series.h
 *  Series RBD management
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

#ifndef SERIES_H_
#define SERIES_H_


#include "../include/rbd.h"


/**
 * Data used during RBD Series computation
 */
struct rbdSeriesData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int numCores;              /* Number of threads in SMP system */
    double *reliabilities;              /* Reliabilities of Series RBD system (matrix for generic Series, array for identical Series) */
    double *output;                     /* Array of computed reliabilities */
    unsigned char numComponents;        /* Number of components of Series RBD system N */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


/* Platform-generic and platform-specific functions */
void *rbdSeriesGenericWorker(void *arg);
void *rbdSeriesIdenticalWorker(void *arg);

/* Platform-generic functions */
void *rbdSeriesGenericWorkerNoarch(struct rbdSeriesData *data);
void *rbdSeriesIdenticalWorkerNoarch(struct rbdSeriesData *data);

void rbdSeriesGenericStepS1d(struct rbdSeriesData *data, unsigned int time);
void rbdSeriesIdenticalStepS1d(struct rbdSeriesData *data, unsigned int time);


#endif /* SERIES_H_ */
