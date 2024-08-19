/*
 *  Component: bridge.h
 *  Bridge RBD management
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

#ifndef BRIDGE_H_
#define BRIDGE_H_


#include "rbd.h"


/**
 * Data used during RBD Bridge computation
 */
struct rbdBridgeData
{
    unsigned char batchIdx;             /* Index of work batch */
    unsigned int numCores;              /* Number of threads in SMP system */
    double *reliabilities;              /* Reliabilities of Bridge RBD system (matrix for generic Bridge, array for identical Bridge) */
    double *output;                     /* Array of computed reliabilities */
    unsigned char numComponents;        /* Number of components of Bridge RBD system N */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


/* Platform-generic and platform-specific functions */
void *rbdBridgeGenericWorker(void *arg);
void *rbdBridgeIdenticalWorker(void *arg);

/* Platform-generic functions */
void rbdBridgeGenericStepS1d(struct rbdBridgeData *data, unsigned int time);
void rbdBridgeIdenticalStepS1d(struct rbdBridgeData *data, unsigned int time);


#endif /* BRIDGE_H_ */
