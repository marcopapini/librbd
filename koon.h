/*
 *  Component: koon.h
 *  KooN (K-out-of-N) RBD management
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

#ifndef KOON_H_
#define KOON_H_


#include "rbd.h"
#include "combinations.h"

#include <limits.h>


struct combinationsKooN
{
    unsigned char numKooNcombinations;              /* Number of combinations of combinations used for computation of KooN */
    struct combinations *combinations[UCHAR_MAX];   /* Combinations of combinations used for computation of KooN */
};

struct rbdKooNFillData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int numCores;                          /* Number of threads in SMP system */
    double *output;                                 /* Array of output Reliability filled with fixed value */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    double value;                                   /* Fixed value used to fill output Reliability array */
};

struct rbdKooNGenericData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int numCores;                          /* Number of threads in SMP system */
    double *reliabilities;                          /* Matrix of reliabilities of KooN RBD system */
    double *output;                                 /* Array of computed reliabilities */
    unsigned char numComponents;                    /* Number of components of KooN RBD system N */
    unsigned char minComponents;                    /* Minimum number of components in the KooN system (K) */
    unsigned char bRecursive;                       /* Flag for KooN resolution through usage of recursion */
    unsigned char bComputeUnreliability;            /* Flag for KooN resolution through usage of Unreliability */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    struct combinationsKooN *combs;                 /* Possible combinations of combinations of KooN components */
};

struct rbdKooNIdenticalData
{
    unsigned char batchIdx;                         /* Index of work batch */
    unsigned int numCores;                          /* Number of threads in SMP system */
    double *reliabilities;                          /* Array of reliabilities of KooN RBD system */
    double *output;                                 /* Array of computed reliabilities */
    unsigned char numComponents;                    /* Number of components of KooN RBD system N */
    unsigned char minComponents;                    /* Minimum number of components in the KooN system (K) */
    unsigned char bComputeUnreliability;            /* Flag for KooN resolution through usage of Unreliability */
    unsigned int numTimes;                          /* Number of time instants to compute T */
    unsigned long long *nCi;                        /* Array of nCi values computed for n=N and i in [K, N] */
};

/* Platform-generic and platform-specific functions */
void *rbdKooNFillWorker(void *arg);
void *rbdKooNGenericWorker(void *arg);
void *rbdKooNIdenticalWorker(void *arg);

/* Platform-generic functions */
void rbdKooNGenericSuccessStepS1d(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNGenericFailStepS1d(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNRecursionS1d(struct rbdKooNGenericData *data, unsigned int time);
void rbdKooNIdenticalSuccessStepS1d(struct rbdKooNIdenticalData *data, unsigned int time);
void rbdKooNIdenticalFailStepS1d(struct rbdKooNIdenticalData *data, unsigned int time);


#endif /* KOON_H_ */
