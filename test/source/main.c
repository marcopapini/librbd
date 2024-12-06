/*
 * main.c
 *
 *  Created on: Jul 14, 2018
 *      Author: maldcity
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <math.h>

#include "rbd.h"

#define NUM_RUNS                15

#define KOON_COMPARISON_N       20
#define KOON_COMPARISON_TESTS   (KOON_COMPARISON_N+2)


typedef struct rdbDimension
{
    unsigned char numComponents;
    unsigned int numTimes;
} rbdDim;


typedef struct resultExperiment
{
    rbdDim rbdDim;
    struct timespec time[NUM_RUNS];
    struct timespec minTime;
    struct timespec maxTime;
    struct timespec medianTime;
} resultExperiment;


static const rbdDim rbdTests[] = {
        {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 7}, {5, 10},
        {2, 1000}, {2, 1537}, {2, 5000}, {2, 10000}, {2, 20000}, {2, 50000},
        {2, 50001}, {2, 50002}, {2, 50003}, {2, 100000}, {2, 200000},
        {3, 1000}, {3, 1537}, {3, 5000}, {3, 10000}, {3, 20000}, {3, 50000},
        {3, 50001}, {3, 50002}, {3, 50003}, {3, 100000}, {3, 200000},
        {4, 1000}, {4, 1537}, {4, 5000}, {4, 10000}, {4, 20000}, {4, 50000},
        {4, 50001}, {4, 50002}, {4, 50003}, {4, 100000}, {4, 200000},
        {5, 1000}, {5, 1537}, {5, 5000}, {5, 10000}, {5, 20000}, {5, 50000},
        {5, 50001}, {5, 50002}, {5, 50003}, {5, 100000}, {5, 200000},
        {7, 1000}, {7, 1537}, {7, 5000}, {7, 10000}, {7, 20000}, {7, 50000},
        {7, 50001}, {7, 50002}, {7, 50003}, {7, 100000}, {7, 200000},
        {10, 1000}, {10, 1537}, {10, 5000}, {10, 10000}, {10, 20000}, {10, 50000},
        {10, 50001}, {10, 50002}, {10, 50003}, {10, 100000}, {10, 200000},
        {12, 1000}, {12, 1537}, {12, 5000}, {12, 10000}, {12, 20000}, {12, 50000},
        {12, 50001}, {12, 50002}, {12, 50003}, {12, 100000}, {12, 200000},
        {15, 1000}, {15, 1537}, {15, 5000}, {15, 10000}, {15, 20000}, {15, 50000},
        {15, 50001}, {15, 50002}, {15, 50003}, {15, 100000}, {15, 200000},
        {20, 1000}, {20, 1537}, {20, 5000}, {20, 10000}, {20, 20000}, {20, 50000},
        {20, 50001}, {20, 50002}, {20, 50003}, {20, 100000}, {20, 200000}
};


static const rbdDim rbdBridgeTests[] = {
        {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 7}, {5, 10},
        {5, 1000}, {5, 1537}, {5, 5000}, {5, 10000}, {5, 20000}, {5, 50000},
        {5, 50001}, {5, 50002}, {5, 50003}, {5, 100000}, {5, 200000},
};


static const rbdDim rbdKooNDimComparison = {KOON_COMPARISON_N, 200000};


#define NUM_EXPERIMENTS                 ((sizeof(rbdTests) / sizeof(rbdDim)))
#define NUM_BRIDGE_EXPERIMENTS          ((sizeof(rbdBridgeTests) / sizeof(rbdDim)))


static resultExperiment resultSeriesGeneric[NUM_EXPERIMENTS];
static resultExperiment resultSeriesIdentical[NUM_EXPERIMENTS];
static resultExperiment resultParallelGeneric[NUM_EXPERIMENTS];
static resultExperiment resultParallelIdentical[NUM_EXPERIMENTS];
static resultExperiment resultKooNGeneric[NUM_EXPERIMENTS];
static resultExperiment resultKooNIdentical[NUM_EXPERIMENTS];
static resultExperiment resultKooNGenericComparison[KOON_COMPARISON_TESTS];
static resultExperiment resultKooNIdenticalComparison[KOON_COMPARISON_TESTS];
static resultExperiment resultBridgeGeneric[NUM_BRIDGE_EXPERIMENTS];
static resultExperiment resultBridgeIdentical[NUM_BRIDGE_EXPERIMENTS];



static inline void timeDiff(struct timespec *start, struct timespec *end, struct timespec *diff)
{
    if(end->tv_nsec > start->tv_nsec) {
        diff->tv_sec = end->tv_sec - start->tv_sec;
        diff->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    else if(end->tv_nsec < start->tv_nsec) {
        diff->tv_sec = end->tv_sec - start->tv_sec - 1;
        diff->tv_nsec = 1000000000 - start->tv_nsec + end->tv_nsec;
    }
    else {
        diff->tv_sec = end->tv_sec - start->tv_sec + 1;
        diff->tv_nsec = 0;
    }
}

static inline int getMinTimeIdx(struct timespec *times, int numTimes)
{
    struct timespec min = {LONG_MAX, LONG_MAX};
    int ii;
    int minIdx;

    for(ii = 0; ii < numTimes; ++ii) {
        if(times->tv_sec < min.tv_sec) {
            min.tv_sec = times->tv_sec;
            min.tv_nsec = times->tv_nsec;
            minIdx = ii;
        }
        else if(times->tv_sec == min.tv_sec) {
            if(times->tv_nsec < min.tv_nsec) {
                min.tv_sec = times->tv_sec;
                min.tv_nsec = times->tv_nsec;
                minIdx = ii;
            }
        }
        ++times;
    }

    return minIdx;
}

static inline void sortTimes(struct timespec *times, int numTimes)
{
    int ii;
    int minIdx;
    struct timespec temp;

    for(ii = 0; ii < numTimes - 1; ++ii) {
        minIdx = getMinTimeIdx(&times[ii], (numTimes - ii));
        if(minIdx != 0) {
            temp = times[ii];
            times[ii] = times[minIdx + ii];
            times[minIdx + ii] = temp;
        }
    }
}

static inline void getMedianTime(struct timespec *times, int numTimes, struct timespec *medianTime)
{
    *medianTime = times[(numTimes / 2)];
}


static void printLog(char * fname, double * reliability, unsigned int numTimes)
{
    FILE *pFile;
    unsigned int ii;

    pFile = fopen(fname, "w");

    fprintf(pFile, " **************************************************************************** \n");
    fprintf(pFile, " *********  Outputs asked for the model: rbd ************** \n\n");

    for (ii = 0; ii < numTimes; ii++) {
        fprintf(pFile, "t=%u\n", ii);
        fprintf(pFile, "      Reliability(t):   %.8e\n", reliability[ii]);
    }

    fclose(pFile);
}


int main(int argc, char **argv)
{
    struct timespec start;
    struct timespec now;
    struct timespec diff;
    double *relMat;
    double *relArr;
    double *lambda;
    double *output;
    int ii, jj, kk;
    unsigned char minComponents;
    FILE *pFile;
    char filename[300];

    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        lambda = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents);
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        srand(0);
        for (jj = 0; jj < rbdTests[ii].numComponents; jj++) {
            lambda[jj] = ((double)rand() / (double)RAND_MAX) / 100000.0;
        }
        for (kk = 0; kk < rbdTests[ii].numComponents; kk++) {
            for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

        snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
        pFile = fopen(filename, "w");
        for (jj = 0; jj < rbdTests[ii].numComponents; jj++) {
            fprintf(pFile, "Lambda %2u    ", jj);
        }
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdTests[ii].numComponents; jj++) {
            fprintf(pFile, "%.10f ", lambda[jj]);
        }
        fprintf(pFile, "\n");
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
            for (kk = 0; kk < rbdTests[ii].numComponents; kk++) {
                fprintf(pFile, "%.10f ", relMat[jj + rbdTests[ii].numTimes * kk]);
            }
            fprintf(pFile, "\n");
        }
        fclose(pFile);

        snprintf(filename, sizeof(filename), "in_identical_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
        pFile = fopen(filename, "w");
        fprintf(pFile, "Lambda\n");
        fprintf(pFile, "%.10f ", lambda[0]);
        fprintf(pFile, "\n");
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
            fprintf(pFile, "%.10f\n", relArr[jj]);
        }
        fclose(pFile);

        resultSeriesGeneric[ii].rbdDim = rbdTests[ii];
        resultSeriesIdentical[ii].rbdDim = rbdTests[ii];

        for(jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Series experiment %d (%d/%lu) - Components %d, times %d\n", jj+1, ii+1, NUM_EXPERIMENTS, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdSeriesGeneric(relMat, output, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_series_gen_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultSeriesGeneric[ii].time[jj] = diff;

            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdSeriesIdentical(relArr, output, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_series_id_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultSeriesIdentical[ii].time[jj] = diff;
        }

        sortTimes(&resultSeriesGeneric[ii].time[0], NUM_RUNS);
        getMedianTime(&resultSeriesGeneric[ii].time[0], NUM_RUNS, &resultSeriesGeneric[ii].medianTime);
        resultSeriesGeneric[ii].minTime = resultSeriesGeneric[ii].time[0];
        resultSeriesGeneric[ii].maxTime = resultSeriesGeneric[ii].time[NUM_RUNS - 1];

        sortTimes(&resultSeriesIdentical[ii].time[0], NUM_RUNS);
        getMedianTime(&resultSeriesIdentical[ii].time[0], NUM_RUNS, &resultSeriesIdentical[ii].medianTime);
        resultSeriesIdentical[ii].minTime = resultSeriesIdentical[ii].time[0];
        resultSeriesIdentical[ii].maxTime = resultSeriesIdentical[ii].time[NUM_RUNS - 1];

        free(lambda);
        free(relMat);
        free(relArr);
        free(output);
    }

    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        lambda = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents);
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        srand(0);
        for (jj = 0; jj < rbdTests[ii].numComponents; jj++) {
            lambda[jj] = ((double)rand() / (double)RAND_MAX) / 100000.0;
        }
        for (kk = 0; kk < rbdTests[ii].numComponents; kk++) {
            for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

        resultParallelGeneric[ii].rbdDim = rbdTests[ii];
        resultParallelIdentical[ii].rbdDim = rbdTests[ii];

        for(jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Parallel experiment %d (%d/%lu) - Components %d, times %d\n", jj+1, ii+1, NUM_EXPERIMENTS, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdParallelGeneric(relMat, output, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_parallel_gen_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultParallelGeneric[ii].time[jj] = diff;

            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdParallelIdentical(relArr, output, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_parallel_id_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultParallelIdentical[ii].time[jj] = diff;
        }

        sortTimes(&resultParallelGeneric[ii].time[0], NUM_RUNS);
        getMedianTime(&resultParallelGeneric[ii].time[0], NUM_RUNS, &resultParallelGeneric[ii].medianTime);
        resultParallelGeneric[ii].minTime = resultParallelGeneric[ii].time[0];
        resultParallelGeneric[ii].maxTime = resultParallelGeneric[ii].time[NUM_RUNS - 1];

        sortTimes(&resultParallelIdentical[ii].time[0], NUM_RUNS);
        getMedianTime(&resultParallelIdentical[ii].time[0], NUM_RUNS, &resultParallelIdentical[ii].medianTime);
        resultParallelIdentical[ii].minTime = resultParallelIdentical[ii].time[0];
        resultParallelIdentical[ii].maxTime = resultParallelIdentical[ii].time[NUM_RUNS - 1];

        free(lambda);
        free(relMat);
        free(relArr);
        free(output);
    }

    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        lambda = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents);
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        srand(0);
        for (jj = 0; jj < rbdTests[ii].numComponents; jj++) {
            lambda[jj] = ((double)rand() / (double)RAND_MAX) / 100000.0;
        }
        for (kk = 0; kk < rbdTests[ii].numComponents; kk++) {
            for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; jj++) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

        resultKooNGeneric[ii].rbdDim = rbdTests[ii];
        resultKooNIdentical[ii].rbdDim = rbdTests[ii];

        for(jj = 0; jj < NUM_RUNS; ++jj) {
            minComponents = (rbdTests[ii].numComponents / 2) + (rbdTests[ii].numComponents & 1);
            printf("KooN experiment %d (%d/%lu) - %doo%d, times %d\n", jj+1, ii+1, NUM_EXPERIMENTS, minComponents, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdKooNGeneric(relMat, output, rbdTests[ii].numComponents, minComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_koon_gen_%doo%dx%d.txt", minComponents, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultKooNGeneric[ii].time[jj] = diff;

            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdKooNIdentical(relArr, output, rbdTests[ii].numComponents, minComponents, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_koon_id_%doo%dx%d.txt", minComponents, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
                printLog(filename, output, rbdTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultKooNIdentical[ii].time[jj] = diff;
        }

        sortTimes(&resultKooNGeneric[ii].time[0], NUM_RUNS);
        getMedianTime(&resultKooNGeneric[ii].time[0], NUM_RUNS, &resultKooNGeneric[ii].medianTime);
        resultKooNGeneric[ii].minTime = resultKooNGeneric[ii].time[0];
        resultKooNGeneric[ii].maxTime = resultKooNGeneric[ii].time[NUM_RUNS - 1];

        sortTimes(&resultKooNIdentical[ii].time[0], NUM_RUNS);
        getMedianTime(&resultKooNIdentical[ii].time[0], NUM_RUNS, &resultKooNIdentical[ii].medianTime);
        resultKooNIdentical[ii].minTime = resultKooNIdentical[ii].time[0];
        resultKooNIdentical[ii].maxTime = resultKooNIdentical[ii].time[NUM_RUNS - 1];

        free(lambda);
        free(relMat);
        free(relArr);
        free(output);
    }

    for(ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        lambda = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numComponents);
        relMat = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numComponents * rbdBridgeTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numTimes);

        srand(0);
        for (jj = 0; jj < rbdBridgeTests[ii].numComponents; jj++) {
            lambda[jj] = ((double)rand() / (double)RAND_MAX) / 100000.0;
        }
        for (kk = 0; kk < rbdBridgeTests[ii].numComponents; kk++) {
            for (jj = 0; jj < rbdBridgeTests[ii].numTimes; jj++) {
                relMat[jj + kk * rbdBridgeTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdBridgeTests[ii].numTimes; jj++) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

        resultBridgeGeneric[ii].rbdDim = rbdBridgeTests[ii];
        resultBridgeIdentical[ii].rbdDim = rbdBridgeTests[ii];

        for(jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Bridge experiment %d (%d/%lu) - Times %d\n", jj+1, ii+1, NUM_BRIDGE_EXPERIMENTS, rbdTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdBridgeGeneric(relMat, output, rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_bridge_gen_%dx%d.txt", rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
                printLog(filename, output, rbdBridgeTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultBridgeGeneric[ii].time[jj] = diff;

            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdBridgeIdentical(relArr, output, rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_bridge_id_%dx%d.txt", rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
                printLog(filename, output, rbdBridgeTests[ii].numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultBridgeIdentical[ii].time[jj] = diff;
        }

        sortTimes(&resultBridgeGeneric[ii].time[0], NUM_RUNS);
        getMedianTime(&resultBridgeGeneric[ii].time[0], NUM_RUNS, &resultBridgeGeneric[ii].medianTime);
        resultBridgeGeneric[ii].minTime = resultBridgeGeneric[ii].time[0];
        resultBridgeGeneric[ii].maxTime = resultBridgeGeneric[ii].time[NUM_RUNS - 1];

        sortTimes(&resultBridgeIdentical[ii].time[0], NUM_RUNS);
        getMedianTime(&resultBridgeIdentical[ii].time[0], NUM_RUNS, &resultBridgeIdentical[ii].medianTime);
        resultBridgeIdentical[ii].minTime = resultBridgeIdentical[ii].time[0];
        resultBridgeIdentical[ii].maxTime = resultBridgeIdentical[ii].time[NUM_RUNS - 1];

        free(lambda);
        free(relMat);
        free(relArr);
        free(output);
    }

    lambda = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numComponents);
    relMat = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numComponents * rbdKooNDimComparison.numTimes);
    relArr = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numTimes);
    output = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numTimes);

    srand(0);
    for (jj = 0; jj < rbdKooNDimComparison.numComponents; jj++) {
        lambda[jj] = ((double)rand() / (double)RAND_MAX) / 100000.0;
    }
    for (kk = 0; kk < rbdKooNDimComparison.numComponents; kk++) {
        for (jj = 0; jj < rbdKooNDimComparison.numTimes; jj++) {
            relMat[jj + kk * rbdKooNDimComparison.numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
        }
    }
    for (jj = 0; jj < rbdKooNDimComparison.numTimes; jj++) {
        relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
    }

    for(ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        resultKooNGenericComparison[ii].rbdDim = rbdKooNDimComparison;
        resultKooNIdenticalComparison[ii].rbdDim = rbdKooNDimComparison;

        for(jj = 0; jj < NUM_RUNS; ++jj) {
            printf("KooN Comparison experiment %d (%d/%d) - %doo%d, times %d\n", jj+1, ii+1, KOON_COMPARISON_TESTS, ii, rbdKooNDimComparison.numComponents, rbdKooNDimComparison.numTimes);
            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdKooNGeneric(relMat, output, rbdKooNDimComparison.numComponents, ii, rbdKooNDimComparison.numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_koon_gen_%doo%dx%d.txt", ii, rbdKooNDimComparison.numComponents, rbdKooNDimComparison.numTimes);
                printLog(filename, output, rbdKooNDimComparison.numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultKooNGenericComparison[ii].time[jj] = diff;

            clock_gettime(CLOCK_MONOTONIC, &start);
            rbdKooNIdentical(relArr, output, rbdKooNDimComparison.numComponents, ii, rbdKooNDimComparison.numTimes);
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (jj == 0) {
                snprintf(filename, sizeof(filename), "out_koon_id_%doo%dx%d.txt", ii, rbdKooNDimComparison.numComponents, rbdKooNDimComparison.numTimes);
                printLog(filename, output, rbdKooNDimComparison.numTimes);
            }
            timeDiff(&start, &now, &diff);
            resultKooNIdenticalComparison[ii].time[jj] = diff;
        }

        sortTimes(&resultKooNGenericComparison[ii].time[0], NUM_RUNS);
        getMedianTime(&resultKooNGenericComparison[ii].time[0], NUM_RUNS, &resultKooNGenericComparison[ii].medianTime);
        resultKooNGenericComparison[ii].minTime = resultKooNGenericComparison[ii].time[0];
        resultKooNGenericComparison[ii].maxTime = resultKooNGenericComparison[ii].time[NUM_RUNS - 1];

        sortTimes(&resultKooNIdenticalComparison[ii].time[0], NUM_RUNS);
        getMedianTime(&resultKooNIdenticalComparison[ii].time[0], NUM_RUNS, &resultKooNIdenticalComparison[ii].medianTime);
        resultKooNIdenticalComparison[ii].minTime = resultKooNIdenticalComparison[ii].time[0];
        resultKooNIdenticalComparison[ii].maxTime = resultKooNIdenticalComparison[ii].time[NUM_RUNS - 1];
    }

    free(lambda);
    free(relMat);
    free(relArr);
    free(output);

    pFile = fopen("log_series_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultSeriesGeneric[ii].rbdDim.numComponents, resultSeriesGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultSeriesGeneric[ii].minTime.tv_sec, (resultSeriesGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultSeriesGeneric[ii].maxTime.tv_sec, (resultSeriesGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultSeriesGeneric[ii].medianTime.tv_sec, (resultSeriesGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_series_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultSeriesIdentical[ii].rbdDim.numComponents, resultSeriesIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultSeriesIdentical[ii].minTime.tv_sec, (resultSeriesIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultSeriesIdentical[ii].maxTime.tv_sec, (resultSeriesIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultSeriesIdentical[ii].medianTime.tv_sec, (resultSeriesIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_parallel_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultParallelGeneric[ii].rbdDim.numComponents, resultParallelGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultParallelGeneric[ii].minTime.tv_sec, (resultParallelGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultParallelGeneric[ii].maxTime.tv_sec, (resultParallelGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultParallelGeneric[ii].medianTime.tv_sec, (resultParallelGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_parallel_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultParallelIdentical[ii].rbdDim.numComponents, resultParallelIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultParallelIdentical[ii].minTime.tv_sec, (resultParallelIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultParallelIdentical[ii].maxTime.tv_sec, (resultParallelIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultParallelIdentical[ii].medianTime.tv_sec, (resultParallelIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        minComponents = (resultKooNGeneric[ii].rbdDim.numComponents / 2) + (resultKooNGeneric[ii].rbdDim.numComponents & 1);
        fprintf(pFile, "%uoo%u, %u, ", minComponents, resultKooNGeneric[ii].rbdDim.numComponents, resultKooNGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultKooNGeneric[ii].minTime.tv_sec, (resultKooNGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultKooNGeneric[ii].maxTime.tv_sec, (resultKooNGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultKooNGeneric[ii].medianTime.tv_sec, (resultKooNGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        minComponents = (resultKooNIdentical[ii].rbdDim.numComponents / 2) + (resultKooNIdentical[ii].rbdDim.numComponents & 1);
        fprintf(pFile, "%uoo%u, %u, ", minComponents, resultKooNIdentical[ii].rbdDim.numComponents, resultKooNIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultKooNIdentical[ii].minTime.tv_sec, (resultKooNIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultKooNIdentical[ii].maxTime.tv_sec, (resultKooNIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultKooNIdentical[ii].medianTime.tv_sec, (resultKooNIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_comparison_generic.txt", "w");
    fprintf(pFile, "Blocks, Min, Max, Median\n");
    for(ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        fprintf(pFile, "%uoo%u, ", ii, resultKooNGenericComparison[ii].rbdDim.numComponents);
        fprintf(pFile, "%ld.%06ld, ", resultKooNGenericComparison[ii].minTime.tv_sec, (resultKooNGenericComparison[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultKooNGenericComparison[ii].maxTime.tv_sec, (resultKooNGenericComparison[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultKooNGenericComparison[ii].medianTime.tv_sec, (resultKooNGenericComparison[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_comparison_identical.txt", "w");
    fprintf(pFile, "Blocks, Min, Max, Median\n");
    for(ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        fprintf(pFile, "%uoo%u, ", ii, resultKooNIdenticalComparison[ii].rbdDim.numComponents);
        fprintf(pFile, "%ld.%06ld, ", resultKooNIdenticalComparison[ii].minTime.tv_sec, (resultKooNIdenticalComparison[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultKooNIdenticalComparison[ii].maxTime.tv_sec, (resultKooNIdenticalComparison[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultKooNIdenticalComparison[ii].medianTime.tv_sec, (resultKooNIdenticalComparison[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_bridge_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultBridgeGeneric[ii].rbdDim.numComponents, resultBridgeGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultBridgeGeneric[ii].minTime.tv_sec, (resultBridgeGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultBridgeGeneric[ii].maxTime.tv_sec, (resultBridgeGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultBridgeGeneric[ii].medianTime.tv_sec, (resultBridgeGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_bridge_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for(ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultBridgeIdentical[ii].rbdDim.numComponents, resultBridgeIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", resultBridgeIdentical[ii].minTime.tv_sec, (resultBridgeIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", resultBridgeIdentical[ii].maxTime.tv_sec, (resultBridgeIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", resultBridgeIdentical[ii].medianTime.tv_sec, (resultBridgeIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    return 0;
}
