/*
 *  Component: koon.c
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


#include "generic/rbd_internal_generic.h"

#include "generic/bdddata.h"
#include "generic/binomial.h"
#include "koon.h"


/**
 * Look-up table for the selection of the RBD KooN Generic algorithm,
 * Shannon Decomposition or BDD Evaluation.
 * For a given N, BDD is used if:
 * K >= rbdKoonGenericBddRange[N][0] && K <= rbdKoonGenericBddRange[N][1]
 */
static const unsigned char rbdKoonGenericBddRange[256][2] = {
    /* N */ /* {min_K, max_K} */
    /*   0 */ { 255,   0 },
    /*   1 */ { 255,   0 },
    /*   2 */ { 255,   0 },
    /*   3 */ { 255,   0 },
    /*   4 */ { 255,   0 },
    /*   5 */ { 255,   0 },
    /*   6 */ { 255,   0 },
    /*   7 */ { 255,   0 },
    /*   8 */ { 255,   0 },
    /*   9 */ { 255,   0 },
    /*  10 */ { 255,   0 },
    /*  11 */ { 255,   0 },
    /*  12 */ { 255,   0 },
    /*  13 */ { 255,   0 },
    /*  14 */ { 255,   0 },
    /*  15 */ { 255,   0 },
    /*  16 */ {   3,  14 },
    /*  17 */ {   3,  15 },
    /*  18 */ {   3,  16 },
    /*  19 */ {   3,  17 },
    /*  20 */ {   3,  18 },
    /*  21 */ {   3,  19 },
    /*  22 */ {   3,  20 },
    /*  23 */ {   3,  21 },
    /*  24 */ {   3,  22 },
    /*  25 */ {   3,  23 },
    /*  26 */ {   3,  24 },
    /*  27 */ {   3,  25 },
    /*  28 */ {   3,  26 },
    /*  29 */ {   3,  27 },
    /*  30 */ {   3,  28 },
    /*  31 */ {   3,  29 },
    /*  32 */ {   3,  30 },
    /*  33 */ {   3,  31 },
    /*  34 */ {   3,  32 },
    /*  35 */ {   3,  33 },
    /*  36 */ {   3,  34 },
    /*  37 */ {   3,  35 },
    /*  38 */ {   3,  36 },
    /*  39 */ {   3,  37 },
    /*  40 */ {   3,  38 },
    /*  41 */ {   3,  39 },
    /*  42 */ {   3,  40 },
    /*  43 */ {   3,  41 },
    /*  44 */ {   3,  42 },
    /*  45 */ {   3,  43 },
    /*  46 */ {   2,  45 },
    /*  47 */ {   2,  46 },
    /*  48 */ {   2,  47 },
    /*  49 */ {   2,  48 },
    /*  50 */ {   2,  49 },
    /*  51 */ {   2,  50 },
    /*  52 */ {   2,  51 },
    /*  53 */ {   2,  52 },
    /*  54 */ {   2,  53 },
    /*  55 */ {   2,  54 },
    /*  56 */ {   2,  55 },
    /*  57 */ {   2,  56 },
    /*  58 */ {   2,  57 },
    /*  59 */ {   2,  58 },
    /*  60 */ {   2,  59 },
    /*  61 */ {   2,  60 },
    /*  62 */ {   2,  61 },
    /*  63 */ {   2,  62 },
    /*  64 */ {   2,  63 },
    /*  65 */ {   2,  64 },
    /*  66 */ {   2,  65 },
    /*  67 */ {   2,  66 },
    /*  68 */ {   2,  67 },
    /*  69 */ {   2,  68 },
    /*  70 */ {   2,  69 },
    /*  71 */ {   2,  70 },
    /*  72 */ {   2,  71 },
    /*  73 */ {   2,  72 },
    /*  74 */ {   2,  73 },
    /*  75 */ {   2,  74 },
    /*  76 */ {   2,  75 },
    /*  77 */ {   2,  76 },
    /*  78 */ {   2,  77 },
    /*  79 */ {   2,  78 },
    /*  80 */ {   2,  79 },
    /*  81 */ {   2,  80 },
    /*  82 */ {   2,  81 },
    /*  83 */ {   2,  82 },
    /*  84 */ {   2,  83 },
    /*  85 */ {   2,  84 },
    /*  86 */ {   2,  85 },
    /*  87 */ {   2,  86 },
    /*  88 */ {   2,  87 },
    /*  89 */ {   2,  88 },
    /*  90 */ {   2,  89 },
    /*  91 */ {   2,  90 },
    /*  92 */ {   2,  91 },
    /*  93 */ {   2,  92 },
    /*  94 */ {   2,  93 },
    /*  95 */ {   2,  94 },
    /*  96 */ {   2,  95 },
    /*  97 */ {   2,  96 },
    /*  98 */ {   2,  97 },
    /*  99 */ {   2,  98 },
    /* 100 */ {   2,  99 },
    /* 101 */ {   2, 100 },
    /* 102 */ {   2, 101 },
    /* 103 */ {   2, 102 },
    /* 104 */ {   2, 103 },
    /* 105 */ {   2, 104 },
    /* 106 */ {   2, 105 },
    /* 107 */ {   2, 106 },
    /* 108 */ {   2, 107 },
    /* 109 */ {   2, 108 },
    /* 110 */ {   2, 109 },
    /* 111 */ {   2, 110 },
    /* 112 */ {   2, 111 },
    /* 113 */ {   2, 112 },
    /* 114 */ {   2, 113 },
    /* 115 */ {   2, 114 },
    /* 116 */ {   2, 115 },
    /* 117 */ {   2, 116 },
    /* 118 */ {   2, 117 },
    /* 119 */ {   2, 118 },
    /* 120 */ {   2, 119 },
    /* 121 */ {   2, 120 },
    /* 122 */ {   2, 121 },
    /* 123 */ {   2, 122 },
    /* 124 */ {   2, 123 },
    /* 125 */ {   2, 124 },
    /* 126 */ {   2, 125 },
    /* 127 */ {   2, 126 },
    /* 128 */ {   2, 127 },
    /* 129 */ {   2, 128 },
    /* 130 */ {   2, 129 },
    /* 131 */ {   2, 130 },
    /* 132 */ {   2, 131 },
    /* 133 */ {   2, 132 },
    /* 134 */ {   2, 133 },
    /* 135 */ {   2, 134 },
    /* 136 */ {   2, 135 },
    /* 137 */ {   2, 136 },
    /* 138 */ {   2, 137 },
    /* 139 */ {   2, 138 },
    /* 140 */ {   2, 139 },
    /* 141 */ {   2, 140 },
    /* 142 */ {   2, 141 },
    /* 143 */ {   2, 142 },
    /* 144 */ {   2, 143 },
    /* 145 */ {   2, 144 },
    /* 146 */ {   2, 145 },
    /* 147 */ {   2, 146 },
    /* 148 */ {   2, 147 },
    /* 149 */ {   2, 148 },
    /* 150 */ {   2, 149 },
    /* 151 */ {   2, 150 },
    /* 152 */ {   2, 151 },
    /* 153 */ {   2, 152 },
    /* 154 */ {   2, 153 },
    /* 155 */ {   2, 154 },
    /* 156 */ {   2, 155 },
    /* 157 */ {   2, 156 },
    /* 158 */ {   2, 157 },
    /* 159 */ {   2, 158 },
    /* 160 */ {   2, 159 },
    /* 161 */ {   2, 160 },
    /* 162 */ {   2, 161 },
    /* 163 */ {   2, 162 },
    /* 164 */ {   2, 163 },
    /* 165 */ {   2, 164 },
    /* 166 */ {   2, 165 },
    /* 167 */ {   2, 166 },
    /* 168 */ {   2, 167 },
    /* 169 */ {   2, 168 },
    /* 170 */ {   2, 169 },
    /* 171 */ {   2, 170 },
    /* 172 */ {   2, 171 },
    /* 173 */ {   2, 172 },
    /* 174 */ {   2, 173 },
    /* 175 */ {   2, 174 },
    /* 176 */ {   2, 175 },
    /* 177 */ {   2, 176 },
    /* 178 */ {   2, 177 },
    /* 179 */ {   2, 178 },
    /* 180 */ {   2, 179 },
    /* 181 */ {   2, 180 },
    /* 182 */ {   2, 181 },
    /* 183 */ {   2, 182 },
    /* 184 */ {   2, 183 },
    /* 185 */ {   2, 184 },
    /* 186 */ {   2, 185 },
    /* 187 */ {   2, 186 },
    /* 188 */ {   2, 187 },
    /* 189 */ {   2, 188 },
    /* 190 */ {   2, 189 },
    /* 191 */ {   2, 190 },
    /* 192 */ {   2, 191 },
    /* 193 */ {   2, 192 },
    /* 194 */ {   2, 193 },
    /* 195 */ {   2, 194 },
    /* 196 */ {   2, 195 },
    /* 197 */ {   2, 196 },
    /* 198 */ {   2, 197 },
    /* 199 */ {   2, 198 },
    /* 200 */ {   2, 199 },
    /* 201 */ {   2, 200 },
    /* 202 */ {   2, 201 },
    /* 203 */ {   2, 202 },
    /* 204 */ {   2, 203 },
    /* 205 */ {   2, 204 },
    /* 206 */ {   2, 205 },
    /* 207 */ {   2, 206 },
    /* 208 */ {   2, 207 },
    /* 209 */ {   2, 208 },
    /* 210 */ {   2, 209 },
    /* 211 */ {   2, 210 },
    /* 212 */ {   2, 211 },
    /* 213 */ {   2, 212 },
    /* 214 */ {   2, 213 },
    /* 215 */ {   2, 214 },
    /* 216 */ {   2, 215 },
    /* 217 */ {   2, 216 },
    /* 218 */ {   2, 217 },
    /* 219 */ {   2, 218 },
    /* 220 */ {   2, 219 },
    /* 221 */ {   2, 220 },
    /* 222 */ {   2, 221 },
    /* 223 */ {   2, 222 },
    /* 224 */ {   2, 223 },
    /* 225 */ {   2, 224 },
    /* 226 */ {   2, 225 },
    /* 227 */ {   2, 226 },
    /* 228 */ {   2, 227 },
    /* 229 */ {   2, 228 },
    /* 230 */ {   2, 229 },
    /* 231 */ {   2, 230 },
    /* 232 */ {   2, 231 },
    /* 233 */ {   2, 232 },
    /* 234 */ {   2, 233 },
    /* 235 */ {   2, 234 },
    /* 236 */ {   2, 235 },
    /* 237 */ {   2, 236 },
    /* 238 */ {   2, 237 },
    /* 239 */ {   2, 238 },
    /* 240 */ {   2, 239 },
    /* 241 */ {   2, 240 },
    /* 242 */ {   2, 241 },
    /* 243 */ {   2, 242 },
    /* 244 */ {   2, 243 },
    /* 245 */ {   2, 244 },
    /* 246 */ {   2, 245 },
    /* 247 */ {   2, 246 },
    /* 248 */ {   2, 247 },
    /* 249 */ {   2, 248 },
    /* 250 */ {   2, 249 },
    /* 251 */ {   2, 250 },
    /* 252 */ {   2, 251 },
    /* 253 */ {   2, 252 },
    /* 254 */ {   2, 253 },
    /* 255 */ {   2, 254 }
};


static int rbdKooNGenericShannon(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes);
static int rbdKooNGenericBdd(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes);
static int buildBddKooNGeneric(struct rbdKooNBddData *data, int *buildCache, double *reliabilities, unsigned char k, unsigned char n);

static int rbdKooNIdenticalBinomial(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes, unsigned char bComputeUnreliability, unsigned long long *nCi);
static int rbdKooNIdenticalBdd(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes);
static int buildBddKooNIdentical(struct rbdKooNBddData *data, int *buildCache, double *reliabilities, unsigned char k, unsigned char n);

/**
 * rbdKooNGeneric
 *
 * Compute reliability of a generic KooN (K-out-of-N) RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of KooN RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdKooNGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    struct rbdKooNFillData fillData;

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if (minComponents > numComponents) {
        /* Prepare fill Output data structure */
        fillData.output = output;
        fillData.numTimes = numTimes;
        fillData.value = 0.0;

        (void)rbdKooNFillWorker(&fillData);

        return 1;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if (minComponents == 0) {
        /* Prepare fill Output data structure */
        fillData.output = output;
        fillData.numTimes = numTimes;
        fillData.value = 1.0;

        (void)rbdKooNFillWorker(&fillData);

        return 1;
    }

    /* If K is 1 then it is a Parallel block */
    if (minComponents == 1) {
        return rbdParallelGeneric(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Series block */
    if (minComponents == numComponents) {
        return rbdSeriesGeneric(reliabilities, output, numComponents, numTimes);
    }

    /* Check if the RBD KooN has to be evaluated with Shannon Decomposition or with BDD Evaluation */
    if ((minComponents >= rbdKoonGenericBddRange[numComponents][0]) &&
        (minComponents <= rbdKoonGenericBddRange[numComponents][1])) {
        return rbdKooNGenericBdd(reliabilities, output, numComponents, minComponents, numTimes);
    }
    else {
        return rbdKooNGenericShannon(reliabilities, output, numComponents, minComponents, numTimes);
    }
}

/**
 * rbdKooNIdentical
 *
 * Compute reliability of an identical KooN (K-out-of-N) RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdKooNIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    unsigned char bUseBdd;
    unsigned char ii;
    unsigned int idx;
    unsigned long long nCi[SCHAR_MAX];
    unsigned char minFaultyComponents;
    unsigned char bComputeUnreliability;
    struct rbdKooNFillData fillData;

    /* If K is 1 then it is a Parallel block */
    if (minComponents == 1) {
        return rbdParallelIdentical(reliabilities, output, numComponents, numTimes);
    }

    /* If K is N then it is a Series block */
    if (minComponents == numComponents) {
        return rbdSeriesIdentical(reliabilities, output, numComponents, numTimes);
    }

    /* If K is greater than N fill output array with all zeroes and return 0 */
    if (minComponents > numComponents) {
        /* Prepare fill Output data structure */
        fillData.output = output;
        fillData.numTimes = numTimes;
        fillData.value = 0.0;

        (void)rbdKooNFillWorker(&fillData);

        return 0;
    }

    /* If K is 0 fill output array with all ones and return 0 */
    if (minComponents == 0) {
        /* Prepare fill Output data structure */
        fillData.output = output;
        fillData.numTimes = numTimes;
        fillData.value = 1.0;

        (void)rbdKooNFillWorker(&fillData);

        return 0;
    }

    bComputeUnreliability = 0;

    /* Compute minimum number of faulty components for having an unreliable block */
    minFaultyComponents = numComponents - minComponents + 1;
    /* Is minimum number of faulty components greater than minimum number of components? */
    if (minFaultyComponents > minComponents) {
        /* Assign minimum number of faulty components to minimum number of components */
        minComponents = minFaultyComponents;
        /* Set KooN computation through Unreliability flag */
        bComputeUnreliability = 1;
    }

    /* Compute all binomial coefficients nCi for i in [k, n] */
    bUseBdd = 0;
    ii = minComponents;
    idx = 0;
    do {
        nCi[idx] = binomialCoefficient(numComponents, ii++);
        if (nCi[idx++] == 0) {
            bUseBdd = 1;
        }
    }
    while ((ii <= numComponents) && (bUseBdd == 0));

    /* Check if the RBD KooN has to be evaluated with Binomial Coefficients or with BDD Evaluation */
    if (bUseBdd == 0) {
        return rbdKooNIdenticalBinomial(reliabilities, output, numComponents, minComponents, numTimes, bComputeUnreliability, nCi);
    }
    else {
        return rbdKooNIdenticalBdd(reliabilities, output, numComponents, minComponents, numTimes);
    }
}


/**
 * rbdKooNGenericShannon
 *
 * Compute reliability of a generic KooN (K-out-of-N) RBD system using Shannon Decomposition
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are not identical, through the application of
 *  Shannon Decomposition
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of KooN RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdKooNGenericShannon(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    int res;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNGenericShannonData *koonData;
    void *threadHandles;
    unsigned int idx;
    unsigned int numCores;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNGenericShannonData koonData[1];
#endif /* CPU_SMP */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);
#endif /* CPU_SMP */

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Allocate generic KooN RBD data array for Shannon Decomposition, return -1 in case of allocation failure */
    koonData = (struct rbdKooNGenericShannonData *)malloc(sizeof(struct rbdKooNGenericShannonData) * numCores);
    if (koonData == NULL) {
        return -1;
    }

    /* Is number of used cores greater than 1? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Prepare generic KooN RBD koonData structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].numCores = numCores;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].numTimes = numTimes;
            initKooNRecursionData(&koonData[idx].recur);

            /* Create the generic KooN RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdKooNGenericShannonWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare generic KooN RBD koonData structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = numCores;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].numTimes = numTimes;
        initKooNRecursionData(&koonData[0].recur);

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNGenericShannonWorker(&koonData[0]);

        /* Wait for created threads completion */
        for(idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare generic KooN RBD koonData structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = 1;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].numTimes = numTimes;
        initKooNRecursionData(&koonData[0].recur);

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNGenericShannonWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free generic KooN RBD koonData array */
    free(koonData);
#endif /* CPU_SMP */

    return res;
}

/**
 * rbdKooNGenericBdd
 *
 * Compute reliability of a generic KooN (K-out-of-N) RBD system using Binary Decision Diagram (BDD)
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic KooN (K-out-of-N) RBD system,
 *  i.e. a system for which the components are not identical, through the usage of the
 *  Binary Decision Diagram (BDD)
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of KooN RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdKooNGenericBdd(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    int res;
    struct bdd *bddmgr;
    unsigned int numCores;
    int root;
    unsigned int cacheSize;
    int *buildCache;
    unsigned int idx;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNBddData *koonData;
    void *threadHandles;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNBddData koonData[1];
#endif /* CPU_SMP */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);
#else
    numCores = 1;
#endif /* CPU_SMP */

    res = 0;

    /* Initialize the BDD Manager */
    bddmgr = bddInit(numCores);
    if (bddmgr == NULL) {
        return -1;
    }

    /* Allocate cache used to build the KooN BDD */
    cacheSize = (numComponents + 1) * (minComponents + 1);
    buildCache = (int *)malloc(cacheSize * sizeof(int));
    if (buildCache == NULL) {
        bddFree(bddmgr);
        return -1;
    }
    for (idx = 0; idx < cacheSize; ++idx) {
        buildCache[idx] = -1;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Allocate generic KooN RBD data array for BDD Evaluation, return -1 in case of allocation failure */
    koonData = (struct rbdKooNBddData *)malloc(sizeof(struct rbdKooNBddData) * numCores);
    if (koonData == NULL) {
        bddFree(bddmgr);
        free(buildCache);
        return -1;
    }

    /* For each available core... */
    for (idx = 0; idx < numCores; ++idx) {
        /* Prepare generic KooN RBD koonData structure */
        koonData[idx].batchIdx = idx;
        koonData[idx].numCores = numCores;
        koonData[idx].output = output;
        koonData[idx].numComponents = numComponents;
        koonData[idx].minComponents = minComponents;
        koonData[idx].numTimes = numTimes;
        koonData[idx].bddmgr = bddmgr;
    }
#else
    /* Prepare generic KooN RBD koonData structure */
    koonData[0].batchIdx = 0;
    koonData[0].numCores = 1;
    koonData[0].output = output;
    koonData[0].numComponents = numComponents;
    koonData[0].minComponents = minComponents;
    koonData[0].numTimes = numTimes;
    koonData[0].bddmgr = bddmgr;
#endif

    /* Build the BDD Manager associated with the give KooN RBD Block */
    root = buildBddKooNGeneric(&koonData[0], buildCache, reliabilities, minComponents, numComponents);
    if (root < 0) {
        bddFree(bddmgr);
        free(buildCache);
        return -1;
    }
    free(buildCache);
    bddmgr->root = root;

    /* Allocate the BDD Pool for BDD fast evaluation */
    if (bddAllocPool(bddmgr) < 0) {
        bddFree(bddmgr);
        return -1;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Is number of used cores greater than 1? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Create the generic KooN RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdKooNBddWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNBddWorker(&koonData[0]);

        /* Wait for created threads completion */
        for(idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNBddWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free generic KooN RBD koonData array */
    free(koonData);
#endif /* CPU_SMP */

    /* Release the BDD */
    bddFree(bddmgr);

    return res;
}

/**
 * buildBddKooNGeneric
 *
 * Recursively build the RBD BDD from the RBD DAG node of a KooN Generic block
 *
 * Input:
 *      struct rbdKooNBddData *data
 *      int *buildCache
 *      double *reliabilities
 *      unsigned char k
 *      unsigned char n
 *
 * Output:
 *      struct rbdKooNBddData *data
 *      int *buildCache
 *
 * Description:
 *  This recursive function builds the BDD representing the provided RBD KooN Generic block.
 *  KOON(A, B, C, K) = ITE(C, KOON(A, B, K-1), ITE(A, B, K))
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *      buildCache: temporary cache used to efficiently build the BDD
 *      reliabilities: matrix of reliabilities of KooN RBD system
 *      k: minimum number of components required by KooN RBD system (K)
 *      n: number of components in KooN RBD system (N)
 *
 * Return (int):
 *  the BDD Node identifier if the provided RBD DAG Node has been converted successfully, -1 otherwise
 */
static int buildBddKooNGeneric(struct rbdKooNBddData *data, int *buildCache, double *reliabilities, unsigned char k, unsigned char n)
{
    int high, low;
    int childId;
    int cacheIdx;
    int res;

    /* If K is 0, the KooN is working */
    if (k == 0) {
        return BDD_TERMINAL_1;
    }
    /* If K is greater than N, the KooN is failed */
    if (k > n) {
        return BDD_TERMINAL_0;
    }

    /* Check if the current BDD Node is already contained in the KooN BDD build cache */
    cacheIdx = n * (data->minComponents + 1) + k;
    if (buildCache[cacheIdx] != -1) {
        return buildCache[cacheIdx];
    }

    /* Decrement N */
    --n;

    /* Add the current component to the BDD Variables */
    childId = bddAddVar(data->bddmgr, n, &reliabilities[n * data->numTimes]);
    if (childId < 0) {
        return -1;
    }

    /* If the last child is working, recursively analyze a (K-1)oo(N-1) */
    high = buildBddKooNGeneric(data, buildCache, reliabilities, k -1, n);
    if (high < 0) {
        return -1;
    }
    /* If the last child is failed, recursively analyze a Koo(N-1) */
    low  = buildBddKooNGeneric(data, buildCache, reliabilities, k, n);
    if (low < 0) {
        return -1;
    }

    /* Return the KooN conditioned on the last child */
    res = bddIte(data->bddmgr, childId, high, low);
    buildCache[cacheIdx] = res;
    return res;
}






















/**
 * rbdKooNIdenticalBinomial
 *
 * Compute reliability of an identical KooN (K-out-of-N) RBD system using Binomial Coefficients
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *      unsigned char bComputeUnreliability
 *      unsigned long long *nCi
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an identical KooN (K-out-of-N) RBD system
 *  through the usage of Binomial Coefficients
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *      bComputeUnreliability: different from 0 to compute the unreliability of the KooN
 *                      and then perform its complement, 0 otherwise
 *      nCi: array of binomial coefficients
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdKooNIdenticalBinomial(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes, unsigned char bComputeUnreliability, unsigned long long *nCi)
{
    int res;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNIdenticalData *koonData;
    void *threadHandles;
    unsigned int numCores;
    unsigned int idx;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNIdenticalData koonData[1];
#endif /* CPU_SMP */

    res = 0;

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);

    /* Allocate generic KooN RBD data array, return -1 in case of allocation failure */
    koonData = (struct rbdKooNIdenticalData *)malloc(sizeof(struct rbdKooNIdenticalData) * numCores);
    if (koonData == NULL) {
        return -1;
    }

    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Prepare identical KooN RBD data structure */
            koonData[idx].batchIdx = idx;
            koonData[idx].numCores = numCores;
            koonData[idx].reliabilities = reliabilities;
            koonData[idx].output = output;
            koonData[idx].numComponents = numComponents;
            koonData[idx].minComponents = minComponents;
            koonData[idx].bComputeUnreliability = bComputeUnreliability;
            koonData[idx].numTimes = numTimes;
            koonData[idx].nCi = &nCi[0];

            /* Create the identical KooN RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdKooNIdenticalWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Prepare identical KooN RBD data structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = numCores;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].bComputeUnreliability = bComputeUnreliability;
        koonData[0].numTimes = numTimes;
        koonData[0].nCi = &nCi[0];

        /* Directly invoke the identical KooN RBD Worker */
        (void)rbdKooNIdenticalWorker(&koonData[0]);

        /* Wait for created threads completion */
        for (idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Prepare identical KooN RBD data structure */
        koonData[0].batchIdx = 0;
        koonData[0].numCores = 1;
        koonData[0].reliabilities = reliabilities;
        koonData[0].output = output;
        koonData[0].numComponents = numComponents;
        koonData[0].minComponents = minComponents;
        koonData[0].bComputeUnreliability = bComputeUnreliability;
        koonData[0].numTimes = numTimes;
        koonData[0].nCi = &nCi[0];

        /* Directly invoke the identical KooN RBD Worker */
        (void)rbdKooNIdenticalWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free identical KooN RBD data array */
    free(koonData);
#endif /* CPU_SMP */

    return res;
}

/**
 * rbdKooNIdenticalBdd
 *
 * Compute reliability of an identical KooN (K-out-of-N) RBD system using Binary Decision Diagram (BDD)
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned char minComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of  KooN (K-out-of-N) RBD system
 *  through the usage of the Binary Decision Diagram (BDD)
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of KooN RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in KooN RBD system (N)
 *      minComponents: minimum number of components required by KooN RBD system (K)
 *      numTimes: number of time instants over which KooN RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
static int rbdKooNIdenticalBdd(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes)
{
    int res;
    struct bdd *bddmgr;
    unsigned int numCores;
    int root;
    unsigned int cacheSize;
    int *buildCache;
    unsigned int idx;
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    struct rbdKooNBddData *koonData;
    void *threadHandles;
#else                                           /* Under single processor-single thread conditional compiling */
    struct rbdKooNBddData koonData[1];
#endif /* CPU_SMP */

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Compute the number of used cores given the number of times */
    numCores = computeNumCores(numTimes);
#else
    numCores = 1;
#endif /* CPU_SMP */

    res = 0;

    /* Initialize the BDD Manager */
    bddmgr = bddInit(numCores);
    if (bddmgr == NULL) {
        return -1;
    }

    /* Allocate cache used to build the KooN BDD */
    cacheSize = (numComponents + 1) * (minComponents + 1);
    buildCache = (int *)malloc(cacheSize * sizeof(int));
    if (buildCache == NULL) {
        bddFree(bddmgr);
        return -1;
    }
    for (idx = 0; idx < cacheSize; ++idx) {
        buildCache[idx] = -1;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Allocate generic KooN RBD data array for BDD Evaluation, return -1 in case of allocation failure */
    koonData = (struct rbdKooNBddData *)malloc(sizeof(struct rbdKooNBddData) * numCores);
    if (koonData == NULL) {
        bddFree(bddmgr);
        free(buildCache);
        return -1;
    }

    /* For each available core... */
    for (idx = 0; idx < numCores; ++idx) {
        /* Prepare generic KooN RBD koonData structure */
        koonData[idx].batchIdx = idx;
        koonData[idx].numCores = numCores;
        koonData[idx].output = output;
        koonData[idx].numComponents = numComponents;
        koonData[idx].minComponents = minComponents;
        koonData[idx].numTimes = numTimes;
        koonData[idx].bddmgr = bddmgr;
    }
#else
    /* Prepare generic KooN RBD koonData structure */
    koonData[0].batchIdx = 0;
    koonData[0].numCores = 1;
    koonData[0].output = output;
    koonData[0].numComponents = numComponents;
    koonData[0].minComponents = minComponents;
    koonData[0].numTimes = numTimes;
    koonData[0].bddmgr = bddmgr;
#endif

    /* Build the BDD Manager associated with the give KooN RBD Block */
    root = buildBddKooNIdentical(&koonData[0], buildCache, reliabilities, minComponents, numComponents);
    if (root < 0) {
        bddFree(bddmgr);
        free(buildCache);
        return -1;
    }
    free(buildCache);
    bddmgr->root = root;

    /* Allocate the BDD Pool for BDD fast evaluation */
    if (bddAllocPool(bddmgr) < 0) {
        bddFree(bddmgr);
        return -1;
    }

#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    /* Is number of used cores greater than 1? */
    if (numCores > 1) {
        /* Allocate Thread ID array, return -1 in case of allocation failure */
        threadHandles = allocateThreadHandles(numCores - 1);
        if (threadHandles == NULL) {
            free(koonData);
            return -1;
        }

        /* For each available core... */
        for (idx = 1; idx < numCores; ++idx) {
            /* Create the generic KooN RBD Worker thread */
            if (createThread(threadHandles, idx - 1, &rbdKooNBddWorker, &koonData[idx]) < 0) {
                res = -1;
            }
        }

        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNBddWorker(&koonData[0]);

        /* Wait for created threads completion */
        for(idx = 1; idx < numCores; ++idx) {
            waitThread(threadHandles, idx - 1);
        }
        /* Free Thread ID array */
        free(threadHandles);
    }
    else {
#endif /* CPU_SMP */
        /* Directly invoke the KooN RBD Worker */
        (void)rbdKooNBddWorker(&koonData[0]);
#if CPU_SMP != 0                                /* Under SMP conditional compiling */
    }

    /* Free generic KooN RBD koonData array */
    free(koonData);
#endif /* CPU_SMP */

    /* Release the BDD */
    bddFree(bddmgr);

    return res;
}

/**
 * buildBddKooNIdentical
 *
 * Recursively build the RBD BDD from the RBD DAG node of a KooN Identical block
 *
 * Input:
 *      struct rbdKooNBddData *data
 *      int *buildCache
 *      double *reliabilities
 *      unsigned char k
 *      unsigned char n
 *
 * Output:
 *      struct rbdKooNBddData *data
 *      int *buildCache
 *
 * Description:
 *  This recursive function builds the BDD representing the provided RBD KooN Identical block.
 *  KOON(A, B, C, K) = ITE(C, KOON(A, B, K-1), ITE(A, B, K))
 *
 * Parameters:
 *      data: Generic KooN for BDD Evaluation RBD data structure
 *      buildCache: temporary cache used to efficiently build the BDD
 *      reliabilities: array of reliabilities of KooN RBD system
 *      k: minimum number of components required by KooN RBD system (K)
 *      n: number of components in KooN RBD system (N)
 *
 * Return (int):
 *  the BDD Node identifier if the provided RBD DAG Node has been converted successfully, -1 otherwise
 */
static int buildBddKooNIdentical(struct rbdKooNBddData *data, int *buildCache, double *reliabilities, unsigned char k, unsigned char n)
{
    int high, low;
    int childId;
    int cacheIdx;
    int res;

    /* If K is 0, the KooN is working */
    if (k == 0) {
        return BDD_TERMINAL_1;
    }
    /* If K is greater than N, the KooN is failed */
    if (k > n) {
        return BDD_TERMINAL_0;
    }

    /* Check if the current BDD Node is already contained in the KooN BDD build cache */
    cacheIdx = n * (data->minComponents + 1) + k;
    if (buildCache[cacheIdx] != -1) {
        return buildCache[cacheIdx];
    }

    /* Decrement N */
    --n;

    /* Add the current component to the BDD Variables */
    childId = bddAddVar(data->bddmgr, n, reliabilities);
    if (childId < 0) {
        return -1;
    }

    /* If the last child is working, recursively analyze a (K-1)oo(N-1) */
    high = buildBddKooNIdentical(data, buildCache, reliabilities, k -1, n);
    if (high < 0) {
        return -1;
    }
    /* If the last child is failed, recursively analyze a Koo(N-1) */
    low  = buildBddKooNIdentical(data, buildCache, reliabilities, k, n);
    if (low < 0) {
        return -1;
    }

    /* Return the KooN conditioned on the last child */
    res = bddIte(data->bddmgr, childId, high, low);
    buildCache[cacheIdx] = res;
    return res;
}
