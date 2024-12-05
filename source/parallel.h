/*
 *  Component: parallel.h
 *  Parallel RBD management
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

#ifndef PARALLEL_H_
#define PARALLEL_H_


#include "rbd.h"


/**
 * Data used during RBD Parallel computation
 */
struct rbdParallelData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int numCores;              /* Number of threads in SMP system */
    double *reliabilities;              /* Reliabilities of Parallel RBD system (matrix for generic Parallel, array for identical Parallel) */
    double *output;                     /* Array of computed reliabilities */
    unsigned char numComponents;        /* Number of components of Parallel RBD system N */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


/* Platform-generic and platform-specific functions */
void *rbdParallelGenericWorker(void *arg);
void *rbdParallelIdenticalWorker(void *arg);

/* Platform-generic functions */
void rbdParallelGenericStepS1d(struct rbdParallelData *data, unsigned int time);
void rbdParallelIdenticalStepS1d(struct rbdParallelData *data, unsigned int time);


#endif /* PARALLEL_H_ */
