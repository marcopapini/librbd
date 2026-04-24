/*
 *  Component: bdddata.c
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

#include "bdddata.h"

#include "rbd_internal_generic.h"

#include <string.h>


#define BDD_NODES_MIN_NODES     1024        /* Minimum size for the BDD Nodes */
#define BDD_CACHE_MIN_SIZE      4096        /* Minimum size for the BDD Manager cache */
#define BDD_VAR_SIZE            256         /* Size for the BDD Variables */


static int bddGetNode(struct bdd *mgr, unsigned int var, int high, int low);
static int bddNodeRehash(struct bdd *mgr);
static unsigned int bddNodeHash(unsigned int var, int high, int low, unsigned int size);
static unsigned int bddIteHash(int i, int t, int e, unsigned int size);
static int bddCacheLookup(struct bdd *mgr, int i, int t, int e);
static void bddCacheInsert(struct bdd *mgr, int i, int t, int e, int res);


/**
 * bddFindTopVar
 *
 * Find the top variable of ITE
 *
 * Input:
 *      struct bdd *mgr
 *      int i
 *      int t
 *      int e
 *
 * Output:
 *      None
 *
 * Description:
 *  This function finds the top-most variable, i.e., closest to the root, of
 *  the provided IF-THEN-ELSE (ITE)
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      i: Index of the IF condition
 *      t: Index of the THEN condition
 *      e: Index of the ELSE condition
 *
 * Return (unsigned int):
 *  The top-most variable
 */
static inline unsigned int bddFindTopVar(struct bdd *mgr, int i, int t, int e)
{
    unsigned int top;

    /* Find the top variable, i.e., the one closest to the root */
    top = u32min(mgr->nodes[i].var, mgr->nodes[t].var);
    top = u32min(top, mgr->nodes[e].var);
    return top;
}

/**
 * bddGetHigh
 *
 * Get high variable
 *
 * Input:
 *      struct bdd *mgr
 *      int node
 *      unsigned int top
 *
 * Output:
 *      None
 *
 * Description:
 *  This function returns the high variable index of the current node if it is
 *  equal to the top variable, it returns the current node index otherwise
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      node: Index of the BDD Node
 *      top: Variable associated with the top BDD Node
 *
 * Return (int):
 *  The high variable index if the current is the top variable, the current node otherwise
 */
static inline int bddGetHigh(struct bdd *mgr, int node, unsigned int top)
{
    return (mgr->nodes[node].var == top) ? mgr->nodes[node].high : node;
}

/**
 * bddGetLow
 *
 * Get low variable
 *
 * Input:
 *      struct bdd *mgr
 *      int node
 *      unsigned int top
 *
 * Output:
 *      None
 *
 * Description:
 *  This function returns the low variable index of the current node if it is
 *  equal to the top variable, it returns the current node index otherwise
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      node: Index of the BDD Node
 *      top: Variable associated with the top BDD Node
 *
 * Return (int):
 *  The low variable index if the current is the top variable, the current node otherwise
 */
static inline int bddGetLow(struct bdd *mgr, int node, unsigned int top)
{
    return (mgr->nodes[node].var == top) ? mgr->nodes[node].low : node;
}


/**
 * bddInit
 *
 * Initialize the BDD Manager
 *
 * Description:
 *  This function initializes the BDD Manager
 *
 * Input:
 *      unsigned char numThreads
 *
 * Output:
 *      None
 *
 * Parameters:
 *      numThreads: Number of concurrent Threads exploiting this BDD
 *
 * Return (struct bdd *):
 *  The initialized BDD Manager if successful, NULL otherwise
 */
struct bdd *bddInit(unsigned char numThreads)
{
    struct bdd *mgr;
    unsigned int idx;

    /* Allocate memory for BDD Manager */
    mgr = (struct bdd *)calloc(1, sizeof(struct bdd));
    if (mgr == NULL) {
        return NULL;
    }

    /* Initialize root */
    mgr->root = -1;

    /* Initialize and allocate BDD Nodes */
    mgr->maxNodes = BDD_NODES_MIN_NODES;
    mgr->stepNodes = BDD_NODES_MIN_NODES;
    mgr->nodes = (struct bddnode *)malloc(mgr->maxNodes * sizeof(struct bddnode));

    /* Initialize and allocate BDD Nodes hash table */
    mgr->uniqueTableSize = nextPow2(BDD_NODES_MIN_NODES) << 1;
    mgr->uniqueTable = (int *)malloc(mgr->uniqueTableSize * sizeof(int));

    /* Initialize and allocate BDD Variables */
    mgr->maxVars = BDD_VAR_SIZE;
    mgr->vars = (struct bddvar *)malloc(mgr->maxVars * sizeof(struct bddvar));

    /* Initialize and allocate BDD Variables hash table */
    mgr->varTableSize = BDD_VAR_SIZE;
    mgr->varTable = (int *)malloc(BDD_VAR_SIZE * sizeof(int));

    /* Initialize and allocate BDD cache */
    mgr->cacheSize = nextPow2(BDD_CACHE_MIN_SIZE);
    mgr->opCache = (struct bddcacheentry *)malloc(mgr->cacheSize * sizeof(struct bddcacheentry));

    if ((mgr->nodes == NULL) || (mgr->uniqueTable == NULL) ||
        (mgr->vars == NULL) || (mgr->varTable == NULL) ||
        (mgr->opCache == NULL)) {
        bddFree(mgr);
        return NULL;
    }

    /* Initialize content of nodes hash table, variables hash table and cache */
    for (idx = 0; idx < mgr->uniqueTableSize; ++idx) {
        mgr->uniqueTable[idx] = -1;
    }
    for (idx = 0; idx < mgr->varTableSize; ++idx) {
        mgr->varTable[idx] = -1;
    }
    for (idx = 0; idx < mgr->cacheSize; ++idx) {
        mgr->opCache[idx].i = -1;
    }

    /* Set the number of concurrent Threads exploiting this BDD */
    mgr->numThreads = numThreads;

    /* Create terminal node 0 - FALSE */
    mgr->nodes[BDD_TERMINAL_0].var = 0xFFFFFFFF;
    mgr->nodes[BDD_TERMINAL_0].high    = -1;
    mgr->nodes[BDD_TERMINAL_0].low     = -1;
    mgr->nodes[BDD_TERMINAL_0].next    = -1;
    /* Create terminal node 1 - TRUE */
    mgr->nodes[BDD_TERMINAL_1].var = 0xFFFFFFFF;
    mgr->nodes[BDD_TERMINAL_1].high    = -1;
    mgr->nodes[BDD_TERMINAL_1].low     = -1;
    mgr->nodes[BDD_TERMINAL_1].next    = -1;
    /* Set the number of nodes in the BDD to the number of terminal nodes */
    mgr->numNodes = BDD_NUM_TERMINAL;

    return mgr;
}

/**
 * bddFree
 *
 * Free the BDD Manager
 *
 * Input:
 *      struct bdd *mgr
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function frees all the memory used by the provided BDD Manager
 *
 * Parameters:
 *      mgr: The BDD Manager to be freed
 *      absoluteMax: Maximum admissible number of BDD Nodes
 *      cacheSize: Number of elements in BDD cache
 */
void bddFree(struct bdd *mgr)
{
    /* Free all allocated memory of the BDD Manager */
    if (mgr->nodes != NULL) {
        free(mgr->nodes);
    }
    if (mgr->uniqueTable != NULL) {
        free(mgr->uniqueTable);
    }
    if (mgr->vars != NULL) {
        free(mgr->vars);
    }
    if (mgr->varTable != NULL) {
        free(mgr->varTable);
    }
    if (mgr->opCache != NULL) {
        free(mgr->opCache);
    }
    if (mgr->valuePool != NULL) {
        free(mgr->valuePool);
    }
    if (mgr->computedPool != NULL) {
        free(mgr->computedPool);
    }
    free(mgr);
}

/**
 * bddAllocPool
 *
 * Allocate the pool of values
 *
 * Input:
 *      struct bdd *mgr
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function allocates the pool of temporary values for the provided BDD Manager
 *
 * Parameters:
 *      mgr: The BDD Manager
 *
 * Return (int):
 *  0 if the pool of values has been successfully allocated, -1 otherwise
 */
int bddAllocPool(struct bdd *mgr)
{
    unsigned int idx;
    unsigned int threadIdx;
    unsigned int numElements;
    double *pool0;
    double *pool1;
    unsigned char *comp;

    /* Free the pool of values if it has been already allocated */
    if (mgr->valuePool) {
        free(mgr->valuePool);
    }
    if (mgr->computedPool) {
        free(mgr->computedPool);
    }

    /* Try to allocate the pool of values for all BDD Nodes of the BDD */
    numElements = mgr->numNodes * mgr->numThreads;
    mgr->valuePool = (double *)malloc(numElements * BDD_WINDOW_SIZE * sizeof(double));
    if (mgr->valuePool == NULL) {
        return -1;
    }
    mgr->computedPool = (unsigned char *)calloc(numElements, sizeof(unsigned char));
    if (mgr->computedPool == NULL) {
        return -1;
    }

    /* For each Thread... */
    for (threadIdx = 0; threadIdx < mgr->numThreads; ++threadIdx) {
        /* For current Thread, retrieve pools for Terminal 0 (FALSE) and Terminal 1 (TRUE) BDD nodes */
        pool0 = bddGetValues(mgr, BDD_TERMINAL_0, threadIdx);
        pool1 = bddGetValues(mgr, BDD_TERMINAL_1, threadIdx);
        /* For current Thread, retrieve the computed array */
        comp = bddGetComputed(mgr, threadIdx);
        /* Initialize pools for Terminal 0 (FALSE) and Terminal 1 (TRUE) BDD nodes */
        for (idx = 0; idx < BDD_WINDOW_SIZE; ++idx) {
            pool0[idx] = 0.0;
            pool1[idx] = 1.0;
        }
        comp[BDD_TERMINAL_0] = 1;
        comp[BDD_TERMINAL_1] = 1;
    }

    return 0;
}

/**
 * bddAddVar
 *
 * Add a BDD Variable to the BDD Manager
 *
 * Input:
 *      struct bdd *mgr
 *      unsigned int varId
 *      double *reliability
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function inserts, if not existing, the requested BDD Variable
 *  into the provided BDD Manager, then returns the BDD Node associated
 *  with the BDD Variable
 *
 * Parameters:
 *      mgr: the BDD Manager
 *      varId: the variable identifier
 *      reliability: reliability curve associated with the variable
 *
 * Return (int):
 *  The non-negative index of the BDD Node associated with the BDD Variable
 *  if successful, -1 otherwise
 */
int bddAddVar(struct bdd *mgr, unsigned int varId, double *reliability)
{
    int newIdx;

    /* Search for the provided BDD Variable inside the current hash bucket */
    if (mgr->varTable[varId] != -1) {
        /* Return the BDD Node associated with the requested BDD Variable */
        return bddGetNode(mgr, mgr->vars[mgr->varTable[varId]].index, BDD_TERMINAL_1, BDD_TERMINAL_0);
    }

    /* The requested BDD Variable is a new variable */

    /* Is the array of BDD Variable already full? */
    if (mgr->numVars >= mgr->maxVars) {
        return -1;
    }

    /* Get the index of the current BDD Variable */
    newIdx = mgr->numVars++;
    /* Populate the BDD Variable */
    mgr->vars[newIdx].varId = varId;
    mgr->vars[newIdx].index = newIdx;
    mgr->vars[newIdx].reliability = reliability;
    /* Add the current BDD Variable inside the variables table */
    mgr->varTable[varId] = (int)newIdx;

    /* Return the BDD Node associated with the requested BDD Variable */
    return bddGetNode(mgr, newIdx, BDD_TERMINAL_1, BDD_TERMINAL_0);
}

/**
 * bddIte
 *
 * Evaluate the IF-THEN-ELSE operator exploiting the BDD Manager
 *
 * Input:
 *      struct bdd *mgr
 *      int i
 *      int t
 *      int e
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function evaluates the IF-THEN-ELSE operation
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      i: Index of the IF condition
 *      t: Index of the THEN condition
 *      e: Index of the ELSE condition
 *
 * Return (int):
 *  The non-negative index of the BDD Node associated with the IF-THEN-ELSE
 *  if successful, -1 otherwise
 */
int bddIte(struct bdd *mgr, int i, int t, int e)
{
    int res, h, l;
    unsigned int top;

    /* If IF is TRUE; return THEN */
    if (i == BDD_TERMINAL_1) {
        return t;
    }
    /* If IF is FALSE; return ELSE */
    if (i == BDD_TERMINAL_0) {
        return e;
    }
    /* If THEN and ELSE are equal, return THEN */
    if (t == e) {
        return t;
    }
    /* If THEN is TRUE and ELSE is FALSE; return IF */
    if ((t == BDD_TERMINAL_1) && (e == BDD_TERMINAL_0)) {
        return i;
    }

    /* Search for the IF-THEN-ELSE in the BDD cache */
    res = bddCacheLookup(mgr, i, t, e);
    if (res != -1) {
        return res;
    }

    /* Shannon decomposition, search for top variable */
    top = bddFindTopVar(mgr, i, t, e);
    /* Recursively evaluate IF-THEN-ELSE on high branch */
    h = bddIte(mgr, bddGetHigh(mgr, i, top),
                    bddGetHigh(mgr, t, top),
                    bddGetHigh(mgr, e, top));
    /* Recursively evaluate IF-THEN-ELSE on low branch */
    l = bddIte(mgr, bddGetLow(mgr, i, top),
                    bddGetLow(mgr, t, top),
                    bddGetLow(mgr, e, top));

    /* Retrieve the BDD Node associated with the IF-THEN-ELSE */
    res = bddGetNode(mgr, top, h, l);
    /* Add the IF-THEN-ELSE into the BDD cache */
    bddCacheInsert(mgr, i, t, e, res);

    return res;
}


/**
 * bddGetNode
 *
 * Get the requested BDD Node
 *
 * Input:
 *      struct bdd *mgr
 *      unsigned int var
 *      int high
 *      int low
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function gets the requested BDD Node from the provided BDD Manager
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      var: Variable associated with the BDD Node
 *      high: High index of the BDD Node
 *      low: Low index of the BDD Node
 *
 * Return (int):
 *  The non-negative index of the requested BDD Node if successful, -1 otherwise
 */
static int bddGetNode(struct bdd *mgr, unsigned int var, int high, int low)
{
    unsigned int newMax;
    unsigned int hash;
    unsigned int threshold;
    int curr;
    int newIdx;
    struct bddnode *newNodes;

    /* Reduction Rule: if the two children are equal, immediately return one of them */
    if (high == low) {
        return high;
    }

    /* Compute hash of the BDD Node and retrieve the first BDD Node from the hash table */
    hash = bddNodeHash(var, high, low, mgr->uniqueTableSize);
    curr = mgr->uniqueTable[hash];

    /* Search for the provided BDD Node inside the current hash bucket */
    while (curr != -1) {
        if ((mgr->nodes[curr].var == var) &&
            (mgr->nodes[curr].high == high) &&
            (mgr->nodes[curr].low == low)) {
            return curr;
        }
        curr = mgr->nodes[curr].next;
    }

    /* The requested BDD Node is a new node */

    /* Is the array of BDD Nodes already full? */
    if (mgr->numNodes >= mgr->maxNodes) {
        /* Compute new BDD Nodes array size */
        newMax = mgr->maxNodes + mgr->stepNodes;

        /* Reallocate BDD Nodes array */
        newNodes = realloc(mgr->nodes, newMax * sizeof(struct bddnode));
        if (newNodes == NULL) {
            return -1;
        }
        mgr->nodes = newNodes;
        mgr->maxNodes = newMax;

        /* Compute the threshold as 75% of the hash table size */
        threshold = (mgr->uniqueTableSize >> 1) + (mgr->uniqueTableSize >> 2);
        /* Is the new number of nodes above the threshold? */
        if (mgr->numNodes > threshold) {
            /* Update the whole hash table */
            if (bddNodeRehash(mgr) != 0) {
                return -1;
            }
            hash = bddNodeHash(var, high, low, mgr->uniqueTableSize);
        }
    }

    /* Get the index of the current BDD Node */
    newIdx = (int)mgr->numNodes++;
    /* Populate the BDD Node */
    mgr->nodes[newIdx].var = var;
    mgr->nodes[newIdx].high = high;
    mgr->nodes[newIdx].low = low;
    /* Chain the current BDD Node inside the hash bucket */
    mgr->nodes[newIdx].next = mgr->uniqueTable[hash];
    mgr->uniqueTable[hash] = newIdx;

    return newIdx;
}

/**
 * bddNodeRehash
 *
 * Recompute the hash table for BDD Nodes
 *
 * Input:
 *      struct bdd *mgr
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function recomputes the overall hash table due to a resize operation
 *
 * Parameters:
 *      mgr: BDD Manager
 *
 * Return (int):
 *  0 if the re-hash operation is successful, -1 otherwise
 */
static int bddNodeRehash(struct bdd *mgr)
{
    unsigned int newHashSize;
    unsigned int idx;
    unsigned int hash;
    int *newTable;

    /* Compute new hash table size */
    newHashSize = mgr->uniqueTableSize << 1;
    /* Allocate new hash table */
    newTable = (int *)malloc(newHashSize * sizeof(int));
    if (newTable == NULL) {
        return -1;
    }

    /* Initialize hash table */
    for (idx = 0; idx < newHashSize; ++idx) {
        newTable[idx] = -1;
    }

    /* Re-insert all existing nodes, excluding the terminal nodes, into the new hash table */
    for (idx = 2; idx < mgr->numNodes; ++idx) {
        hash = bddNodeHash(mgr->nodes[idx].var, mgr->nodes[idx].high, mgr->nodes[idx].low, newHashSize);
        mgr->nodes[idx].next = newTable[hash];
        newTable[hash] = (int)idx;
    }

    /* Substitute old hash table with the new one */
    free(mgr->uniqueTable);
    mgr->uniqueTable = newTable;
    mgr->uniqueTableSize = newHashSize;

    return 0;
}

/**
 * bddNodeHash
 *
 * Compute the hash function for the BDD Node
 *
 * Input:
 *      unsigned int var
 *      int high
 *      int low
 *      unsigned int size
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the hash function for the provided BDD Node.
 *  The hash function used if FNV-1a (Fowler-Noll-Vo)
 *
 * Parameters:
 *      var: Variable associated with the BDD Node
 *      high: High index of the BDD Node
 *      low: Low index of the BDD Node
 *      size: Size of the hash table
 *
 * Return (unsigned int):
 *  The result of the FNV-1a hash function
 */
static unsigned int bddNodeHash(unsigned int var, int high, int low, unsigned int size)
{
    unsigned int hash;

    hash = 0x811c9dc5;
    hash ^= var;
    hash *= 0x01000193;
    hash ^= (unsigned int)high;
    hash *= 0x01000193;
    hash ^= (unsigned int)low;
    hash *= 0x01000193;
    return hash & (size - 1);
}

/**
 * bddIteHash
 *
 * Compute the hash function for the BDD IF-THEN-ELSE function
 *
 * Input:
 *      int i
 *      int t
 *      int e
 *      unsigned int size
 *
 * Output:
 *      None
 *
 * Description:
 *  This function computes the hash function for the IF-THEN-ELSE (ITE) function
 *
 * Parameters:
 *      i: Index of the IF condition
 *      t: Index of the THEN condition
 *      e: Index of the ELSE condition
 *      size: Size of the hash table
 *
 * Return (unsigned int):
 *  The result of the hash function
 */
static unsigned int bddIteHash(int i, int t, int e, unsigned int size)
{
    unsigned int hash;

    hash = (unsigned int)i;
    hash ^= (unsigned int)t + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (unsigned int)e + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash & (size - 1);
}

/**
 * bddCacheLookup
 *
 * Check if the requested IF-THEN-ELSE (ITE) is in the BDD cache
 *
 * Input:
 *      struct bdd *mgr
 *      int i
 *      int t
 *      int e
 *
 * Output:
 *      None
 *
 * Description:
 *  This function checks if the BDD cache already contains the provided IF-THEN-ELSE (ITE)
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      i: The IF BDD Node
 *      t: The THEN BDD Node
 *      e: The ELSE BDD Node
 *
 * Return (int):
 *  The non-negative index of the resulting BDD Node if it is contained in the BDD cache, -1 otherwise
 */
static int bddCacheLookup(struct bdd *mgr, int i, int t, int e)
{
    unsigned int hash;
    struct bddcacheentry *entry;

    /* Compute hash value for the BDD structure */
    hash = bddIteHash(i, t, e, mgr->cacheSize);
    /* Check if the BDD cache memory contains the BDD structure */
    entry = &mgr->opCache[hash];
    if ((entry->i == i) && (entry->t == t) && (entry->e == e)) {
        return entry->res;
    }
    return -1;
}

/**
 * bddCacheInsert
 *
 * Insert the provided IF-THEN-ELSE (ITE) into the BDD cache
 *
 * Input:
 *      struct bdd *mgr
 *      int i
 *      int t
 *      int e
 *      int res
 *
 * Output:
 *      struct bdd *mgr
 *
 * Description:
 *  This function inserts provided IF-THEN-ELSE (ITE) into the BDD cache
 *
 * Parameters:
 *      mgr: The BDD Manager
 *      i: The IF BDD Node
 *      t: The THEN BDD Node
 *      e: The ELSE BDD Node
 *      res: The Result BDD Node
 */
static void bddCacheInsert(struct bdd *mgr, int i, int t, int e, int res)
{
    unsigned int hash;

    /* Compute hash value for the BDD structure */
    hash = bddIteHash(i, t, e, mgr->cacheSize);
    /* Insert the BDD structure into the BDD cache memory */
    mgr->opCache[hash].i = i;
    mgr->opCache[hash].t = t;
    mgr->opCache[hash].e = e;
    mgr->opCache[hash].res = res;
}
