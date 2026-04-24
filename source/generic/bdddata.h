/*
 *  Component: bdddata.h
 *  Binary Decision Diagram implementation
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

#ifndef BDDDATA_H
#define BDDDATA_H


#include <stdint.h>
#include <stdlib.h>


#define BDD_WINDOW_SIZE     512             /* Elements in reliability curve cache */
#define BDD_TERMINAL_0      0               /* Terminal node 0 - FALSE */
#define BDD_TERMINAL_1      1               /* Terminal node 1 - TRUE */
#define BDD_NUM_TERMINAL    2               /* A BDD has two terminal nodes */


struct bddnode
{
    unsigned int var;                       /* BDD Variable identifier */
    int high;                               /* Index of child BDD Node when current Variable is HIGH */
    int low;                                /* Index of child BDD Node when current Variable is LOW */
    int next;                               /* Index of next BDD Node in Hash Table */
};

struct bddvar
{
    unsigned int index;                     /* Index of current BDD Variable */
    unsigned int varId;                     /* Variable identifier */
    double *reliability;                    /* Reliability array of current BDD Variable */
};

struct bddcacheentry
{
    int i;                                  /* Index of IF BDD Node */
    int t;                                  /* Index of THEN BDD Node */
    int e;                                  /* Index of ELSE BDD Node */
    int res;                                /* Index of result BDD Node */
};

struct bdd
{
    int root;                               /* Index of BDD Node acting as root */

    struct bddnode *nodes;                  /* Array of BDD Nodes */
    unsigned int numNodes;                  /* Current number of BDD Nodes */
    unsigned int maxNodes;                  /* Current capacity of BDD Nodes */
    unsigned int stepNodes;                 /* Step to resize BDD Nodes array if needed */
    int *uniqueTable;                       /* Unique Hash Table */
    unsigned int uniqueTableSize;           /* Size of Unique Hash Table */

    struct bddvar *vars;                    /* Array of BDD Variables */
    unsigned int numVars;                   /* Current number of BDD Variables */
    unsigned int maxVars;                   /* Current capacity of BDD Variables */
    int *varTable;                          /* Variables Hash Table */
    unsigned int varTableSize;              /* Size of Variables Hash Table */

    struct bddcacheentry *opCache;          /* BDD ITE cache */
    unsigned int cacheSize;                 /* Size of BDD ITE cache */

    unsigned char numThreads;               /* Number of concurrent Threads operating on the BDD Manager */
    double *valuePool;                      /* Pool of all values for BDD Evaluation */
    unsigned char *computedPool;            /* Pool of BDD Nodes already computed */
};


/**
 * bddGetValues
 *
 * Get values array from pool
 *
 * Input:
 *      const struct bdd *const mgr
 *      int nodeIdx
 *      unsigned int threadIdx
 *
 * Output:
 *      None
 *
 * Description:
 *  This function gets the values array associated with the provided BDD Node and Thread
 *
 * Parameters:
 *      mgr: the BDD Manager
 *      nodeIdx: the index of the BDD Node
 *      threadIdx: the index of the Thread running the BDD Manager
 *
 * Return (double *):
 *  the values array if successful, NULL otherwise
 */
static inline double* bddGetValues(const struct bdd *const mgr, int nodeIdx, unsigned int threadIdx)
{
    return &(mgr->valuePool[(threadIdx * mgr->numNodes + nodeIdx) * BDD_WINDOW_SIZE]);
}

/**
 * bddGetComputed
 *
 * Get computed array from pool
 *
 * Input:
 *      const struct bdd *const mgr
 *      unsigned char threadIdx
 *
 * Output:
 *      None
 *
 * Description:
 *  This function gets the computed array associated with the provided Thread
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      threadIdx: The index of the Thread running the BDD Manager
 *
 * Return (unsigned char *):
 *  The computed array if successful, NULL otherwise
 */
static inline unsigned char* bddGetComputed(const struct bdd *const mgr, unsigned char threadIdx)
{
    return &(mgr->computedPool[threadIdx * mgr->numNodes]);
}


struct bdd *bddInit(unsigned char numThreads);
void bddFree(struct bdd *mgr);
int bddAllocPool(struct bdd *mgr);
int bddAddVar(struct bdd *mgr, unsigned int varId, double *reliability);
int bddIte(struct bdd *mgr, int i, int t, int e);

#endif
