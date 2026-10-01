/*
 *  Component: reliability.h
 *  Reliability RBD management
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

#ifndef RELIABILITY_H_
#define RELIABILITY_H_


#include "../include/rbd.h"


/**
 * Data used during RBD Unreliability computation
 */
struct rbdUnreliabilityData
{
    double *reliability;                /* Reliability array to compute unreliability */
    double *unreliability;              /* Array of computed unreliability */
    unsigned int numTimes;              /* Number of time instants to compute T */
};

/**
 * Data used during RBD Reliability sum computation
 */
struct rbdReliabilityData
{
    double *r1;                         /* First Reliability array */
    double *r2;                         /* Second Reliability array */
    double *output;                     /* Array of computed reliability */
    unsigned int numTimes;              /* Number of time instants to compute T */
};


/* Platform-generic and platform-specific functions */
void *rbdUnreliabilityWorker(void *arg);
void *rbdReliabilitySumWorker(void *arg);

/* Platform-generic functions */
void *rbdUnreliabilityWorkerNoarch(struct rbdUnreliabilityData *data);
void *rbdReliabilitySumWorkerNoarch(struct rbdReliabilityData *data);


#endif /* RELIABILITY_H_ */
