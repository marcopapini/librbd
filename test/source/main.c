/*
 * main.c
 *
 *  Created on: Jul 14, 2018
 *      Author: maldcity
 */


#if defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <math.h>

#include "rbd.h"

#define NUM_RUNS                15U

#define PRINT_INPUT             0

#define TEST_SERIES
#define TEST_PARALLEL
#define TEST_KOON
#define TEST_KOON_COMPARE
#define TEST_BRIDGE

#if defined(TEST_KOON_COMPARE)
#define KOON_COMPARISON_N       20
#define KOON_COMPARISON_TESTS   (KOON_COMPARISON_N+2)
#endif


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


#if defined(TEST_SERIES) || defined(TEST_PARALLEL) || defined(TEST_KOON)
static const rbdDim rbdTests[] = {
        {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 7}, {5, 10},
        {2, 1000}, {2, 1607}, {2, 5000}, {2, 10000}, {2, 20000}, {2, 50000},
        {2, 50001}, {2, 50002}, {2, 50003}, {2, 100000}, {2, 200000},
        {3, 1000}, {3, 1607}, {3, 5000}, {3, 10000}, {3, 20000}, {3, 50000},
        {3, 50001}, {3, 50002}, {3, 50003}, {3, 100000}, {3, 200000},
        {4, 1000}, {4, 1607}, {4, 5000}, {4, 10000}, {4, 20000}, {4, 50000},
        {4, 50001}, {4, 50002}, {4, 50003}, {4, 100000}, {4, 200000},
        {5, 1000}, {5, 1607}, {5, 5000}, {5, 10000}, {5, 20000}, {5, 50000},
        {5, 50001}, {5, 50002}, {5, 50003}, {5, 100000}, {5, 200000},
        {7, 1000}, {7, 1607}, {7, 5000}, {7, 10000}, {7, 20000}, {7, 50000},
        {7, 50001}, {7, 50002}, {7, 50003}, {7, 100000}, {7, 200000},
        {10, 1000}, {10, 1607}, {10, 5000}, {10, 10000}, {10, 20000}, {10, 50000},
        {10, 50001}, {10, 50002}, {10, 50003}, {10, 100000}, {10, 200000},
        {12, 1000}, {12, 1607}, {12, 5000}, {12, 10000}, {12, 20000}, {12, 50000},
        {12, 50001}, {12, 50002}, {12, 50003}, {12, 100000}, {12, 200000},
        {15, 1000}, {15, 1607}, {15, 5000}, {15, 10000}, {15, 20000}, {15, 50000},
        {15, 50001}, {15, 50002}, {15, 50003}, {15, 100000}, {15, 200000},
        {20, 1000}, {20, 1607}, {20, 5000}, {20, 10000}, {20, 20000}, {20, 50000},
        {20, 50001}, {20, 50002}, {20, 50003}, {20, 100000}, {20, 200000},
        {50, 1000}, {50, 1607}, {50, 5000}, {50, 10000}, {50, 20000}, {50, 50000},
        {50, 50001}, {50, 50002}, {50, 50003}, {50, 100000}, {50, 200000},
        {100, 1000}, {100, 1607}, {100, 5000}, {100, 10000}, {100, 20000}, {100, 50000},
        {100, 50001}, {100, 50002}, {100, 50003}, {100, 100000}, {100, 200000},
        {200, 1000}, {200, 1607}, {200, 5000}, {200, 10000}, {200, 20000}, {200, 50000},
        {200, 50001}, {200, 50002}, {200, 50003}, {200, 100000}, {200, 200000}
};
#endif


#if defined(TEST_BRIDGE)
static const rbdDim rbdBridgeTests[] = {
        {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 7}, {5, 10},
        {5, 1000}, {5, 1607}, {5, 5000}, {5, 10000}, {5, 20000}, {5, 50000},
        {5, 50001}, {5, 50002}, {5, 50003}, {5, 100000}, {5, 200000},
};
#endif


#if defined(TEST_KOON_COMPARE)
static const rbdDim rbdKooNDimComparison = {KOON_COMPARISON_N, 200000};
#endif

static const double lambda[] = {
        0.0000074218, 0.0000031542, 0.0000089231, 0.0000014567, 0.0000056782,
        0.0000041239, 0.0000098710, 0.0000023456, 0.0000067891, 0.0000034567,
        0.0000010023, 0.0000087654, 0.0000054321, 0.0000022233, 0.0000077788,
        0.0000044455, 0.0000099912, 0.0000011122, 0.0000033344, 0.0000066677,
        0.0000088899, 0.0000022211, 0.0000055566, 0.0000012345, 0.0000090000,
        0.0000045678, 0.0000076543, 0.0000032109, 0.0000019876, 0.0000081234,
        0.0000059876, 0.0000021345, 0.0000071239, 0.0000049876, 0.0000091122,
        0.0000015566, 0.0000036677, 0.0000061122, 0.0000083344, 0.0000025566,
        0.0000057788, 0.0000092233, 0.0000018899, 0.0000042233, 0.0000073344,
        0.0000038899, 0.0000014455, 0.0000086677, 0.0000062233, 0.0000029911,
        0.0000051122, 0.0000094455, 0.0000017788, 0.0000048899, 0.0000079911,
        0.0000035566, 0.0000013344, 0.0000082233, 0.0000069911, 0.0000026677,
        0.0000052233, 0.0000096677, 0.0000016677, 0.0000043344, 0.0000072233,
        0.0000031122, 0.0000018877, 0.0000085566, 0.0000064455, 0.0000027788,
        0.0000058899, 0.0000091133, 0.0000012244, 0.0000046655, 0.0000078866,
        0.0000039977, 0.0000015588, 0.0000089911, 0.0000061133, 0.0000024455,
        0.0000053366, 0.0000097788, 0.0000014422, 0.0000041133, 0.0000075544,
        0.0000032255, 0.0000019966, 0.0000084477, 0.0000068888, 0.0000025599,
        0.0000054411, 0.0000092222, 0.0000013333, 0.0000047744, 0.0000071155,
        0.0000036666, 0.0000012277, 0.0000088888, 0.0000063399, 0.0000021100,
        0.0000059911, 0.0000095522, 0.0000011133, 0.0000048844, 0.0000074455,
        0.0000037766, 0.0000014477, 0.0000086688, 0.0000062299, 0.0000028800,
        0.0000051111, 0.0000093322, 0.0000015533, 0.0000049944, 0.0000072255,
        0.0000038866, 0.0000016677, 0.0000084488, 0.0000061199, 0.0000027700,
        0.0000053311, 0.0000091122, 0.0000018833, 0.0000042244, 0.0000075555,
        0.0000034466, 0.0000011177, 0.0000083388, 0.0000065599, 0.0000029900,
        0.0000052211, 0.0000098822, 0.0000017733, 0.0000043344, 0.0000076655,
        0.0000031166, 0.0000014477, 0.0000082288, 0.0000067799, 0.0000024400,
        0.0000056611, 0.0000094422, 0.0000012233, 0.0000045544, 0.0000078855,
        0.0000033366, 0.0000019977, 0.0000081188, 0.0000069999, 0.0000021100,
        0.0000054411, 0.0000097722, 0.0000015533, 0.0000048844, 0.0000072255,
        0.0000039966, 0.0000016677, 0.0000085588, 0.0000063399, 0.0000027700,
        0.0000058811, 0.0000092222, 0.0000014433, 0.0000041144, 0.0000074455,
        0.0000035566, 0.0000018877, 0.0000087788, 0.0000061199, 0.0000023300,
        0.0000051111, 0.0000099922, 0.0000013333, 0.0000046644, 0.0000079955,
        0.0000032266, 0.0000011177, 0.0000084488, 0.0000066699, 0.0000025500,
        0.0000057711, 0.0000093322, 0.0000016633, 0.0000044444, 0.0000077755,
        0.0000038866, 0.0000015577, 0.0000089988, 0.0000062299, 0.0000021100,
        0.0000055511, 0.0000096622, 0.0000012233, 0.0000049944, 0.0000073355,
        0.0000031166, 0.0000017777, 0.0000086688, 0.0000068899, 0.0000024400,
        0.0000059911, 0.0000091122, 0.0000014433, 0.0000042244, 0.0000075555,
        0.0000036666, 0.0000019977, 0.0000083388, 0.0000065599, 0.0000028800,
        0.0000053311, 0.0000098822, 0.0000011133, 0.0000047744, 0.0000071155,
        0.0000034466, 0.0000013377, 0.0000081188, 0.0000069999, 0.0000022200,
        0.0000056611, 0.0000094422, 0.0000018833, 0.0000045544, 0.0000078855,
        0.0000039966, 0.0000016677, 0.0000082288, 0.0000067799, 0.0000025500,
        0.0000051111, 0.0000097722, 0.0000013333, 0.0000041144, 0.0000074455,
        0.0000033366, 0.0000015577, 0.0000086688, 0.0000062299, 0.0000029900,
        0.0000058811, 0.0000092222, 0.0000014433, 0.0000046644, 0.0000079955,
        0.0000037766, 0.0000011177, 0.0000084488, 0.0000061199, 0.0000026600,
        0.0000054411, 0.0000099922, 0.0000012233, 0.0000043344, 0.0000076655
    };


#if defined(TEST_SERIES) || defined(TEST_PARALLEL) || defined(TEST_KOON)
#define NUM_EXPERIMENTS                 ((sizeof(rbdTests) / sizeof(rbdDim)))
#endif
#if defined(TEST_BRIDGE)
#define NUM_BRIDGE_EXPERIMENTS          ((sizeof(rbdBridgeTests) / sizeof(rbdDim)))
#endif

#if defined(TEST_SERIES)
static resultExperiment resultSeriesGeneric[NUM_EXPERIMENTS];
static resultExperiment resultSeriesIdentical[NUM_EXPERIMENTS];
#endif
#if defined(TEST_PARALLEL)
static resultExperiment resultParallelGeneric[NUM_EXPERIMENTS];
static resultExperiment resultParallelIdentical[NUM_EXPERIMENTS];
#endif
#if defined(TEST_KOON)
static resultExperiment resultKooNGeneric[NUM_EXPERIMENTS];
static resultExperiment resultKooNIdentical[NUM_EXPERIMENTS];
#endif
#if defined(TEST_KOON_COMPARE)
static resultExperiment resultKooNGenericComparison[KOON_COMPARISON_TESTS];
static resultExperiment resultKooNIdenticalComparison[KOON_COMPARISON_TESTS];
#endif
#if defined(TEST_BRIDGE)
static resultExperiment resultBridgeGeneric[NUM_BRIDGE_EXPERIMENTS];
static resultExperiment resultBridgeIdentical[NUM_BRIDGE_EXPERIMENTS];
#endif


#if defined(_MSC_VER)
#define CLOCK_MONOTONIC                 0
#define POW10_9                         1000000000

typedef int clockid_t;

static __inline int lc_set_errno(int result)
{
    if (result != 0) {
        errno = result;
        return -1;
    }
    return 0;
}

static int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    LARGE_INTEGER pf, pc;

    if (QueryPerformanceFrequency(&pf) == 0) {
        return lc_set_errno(EINVAL);
    }

    if (QueryPerformanceCounter(&pc) == 0) {
        return lc_set_errno(EINVAL);
    }

    tp->tv_sec = pc.QuadPart / pf.QuadPart;
    tp->tv_nsec = (int)(((pc.QuadPart % pf.QuadPart) * POW10_9 + (pf.QuadPart >> 1)) / pf.QuadPart);
    if (tp->tv_nsec >= POW10_9) {
        ++tp->tv_sec;
        tp->tv_nsec -= POW10_9;
    }

    return 0;
}
#endif



static inline void timeDiff(struct timespec *start, struct timespec *end, struct timespec *diff)
{
    if (end->tv_nsec > start->tv_nsec) {
        diff->tv_sec = end->tv_sec - start->tv_sec;
        diff->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    else if (end->tv_nsec < start->tv_nsec) {
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

    minIdx = 0;

    for (ii = 0; ii < numTimes; ++ii) {
        if (times->tv_sec < min.tv_sec) {
            min.tv_sec = times->tv_sec;
            min.tv_nsec = times->tv_nsec;
            minIdx = ii;
        }
        else if (times->tv_sec == min.tv_sec) {
            if (times->tv_nsec < min.tv_nsec) {
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

    for (ii = 0; ii < numTimes - 1; ++ii) {
        minIdx = getMinTimeIdx(&times[ii], (numTimes - ii));
        if (minIdx != 0) {
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

#if 1
    fprintf(pFile, " **************************************************************************** \n");
    fprintf(pFile, " *********  Outputs asked for the model: rbd ************** \n\n");

    for (ii = 0; ii < numTimes; ++ii) {
        fprintf(pFile, "t=%.6f\n", (double)ii);
        fprintf(pFile, "      Reliability(t):   %.8e\n", reliability[ii]);
    }
#else
    /* Write output file header. Reliability Curve header starts as follows:
     *
     *  From: <FROM_TIME>
     *  To: <TO_TIME>
     *  Step: <STEP>
     *  Values: "
     *
     * Where <FROM_TIME> and <TO_TIME> are respectively the start and end time of the
     * reliability curve and <STEP> is the time discretization.
     */
    fprintf(pFile, "From: %.4f\n", 0.0);
    fprintf(pFile, "To: %.4f\n", (double)(numTimes - 1));
    fprintf(pFile, "Step: %.4f\n", 1.0);
    fprintf(pFile, "Values: \n");

    /* For each point over reliability curve... */
    for (ii = 0; ii < numTimes; ++ii) {
        /* Write point over reliability curve to file */
        fprintf(pFile, "%.10e\n", reliability[ii]);
    }
#endif

    fclose(pFile);
}


int main(int argc, char **argv)
{
    struct timespec start;
    struct timespec now;
    struct timespec diff;
    double *relMat;
    double *relArr;
    double *output;
    int ii;
    unsigned int jj, kk;
    unsigned char minComponents;
    FILE *pFile;
    char filename[300];
#if PRINT_INPUT != 0
#if defined(TEST_SERIES) || defined(TEST_PARALLEL) || defined(TEST_KOON)
    unsigned char inputPrinted;
#endif
#endif

#if PRINT_INPUT != 0
#if defined(TEST_SERIES) || defined(TEST_PARALLEL) || defined(TEST_KOON)
    inputPrinted = 0;
#endif
#endif

#if defined(TEST_SERIES)
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

#if PRINT_INPUT != 0
        snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
        pFile = fopen(filename, "w");
        for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
            fprintf(pFile, "Lambda %2u    ", jj);
        }
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
            fprintf(pFile, "%.10f ", lambda[jj]);
        }
        fprintf(pFile, "\n");
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
            for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
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
        for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
            fprintf(pFile, "%.10f\n", relArr[jj]);
        }
        fclose(pFile);
#endif

        resultSeriesGeneric[ii].rbdDim = rbdTests[ii];
        resultSeriesIdentical[ii].rbdDim = rbdTests[ii];

        for (jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Series experiment %d (%d/%lu) - Components %d, times %d\n", jj+1, ii+1, (unsigned long)NUM_EXPERIMENTS, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
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

        free(relMat);
        free(relArr);
        free(output);
    }

#if PRINT_INPUT != 0
    inputPrinted = 1;
#endif
#endif

#if defined(TEST_PARALLEL)
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

#if PRINT_INPUT != 0
        if (inputPrinted == 0) {
            snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            pFile = fopen(filename, "w");
            for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
                fprintf(pFile, "Lambda %2u    ", jj);
            }
            fprintf(pFile, "\n");
            for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
                fprintf(pFile, "%.10f ", lambda[jj]);
            }
            fprintf(pFile, "\n");
            fprintf(pFile, "\n");
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
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
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                fprintf(pFile, "%.10f\n", relArr[jj]);
            }
            fclose(pFile);
        }
#endif

        resultParallelGeneric[ii].rbdDim = rbdTests[ii];
        resultParallelIdentical[ii].rbdDim = rbdTests[ii];

        for (jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Parallel experiment %d (%d/%lu) - Components %d, times %d\n", jj+1, ii+1, (unsigned long)NUM_EXPERIMENTS, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
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

        free(relMat);
        free(relArr);
        free(output);
    }

#if PRINT_INPUT != 0
    inputPrinted = 1;
#endif
#endif

#if defined(TEST_KOON)
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        relMat = (double *)malloc(sizeof(double) * rbdTests[ii].numComponents * rbdTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdTests[ii].numTimes);

        for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                relMat[jj + kk * rbdTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

#if PRINT_INPUT != 0
        if (inputPrinted == 0) {
            snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdTests[ii].numComponents, rbdTests[ii].numTimes);
            pFile = fopen(filename, "w");
            for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
                fprintf(pFile, "Lambda %2u    ", jj);
            }
            fprintf(pFile, "\n");
            for (jj = 0; jj < rbdTests[ii].numComponents; ++jj) {
                fprintf(pFile, "%.10f ", lambda[jj]);
            }
            fprintf(pFile, "\n");
            fprintf(pFile, "\n");
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                for (kk = 0; kk < rbdTests[ii].numComponents; ++kk) {
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
            for (jj = 0; jj < rbdTests[ii].numTimes; ++jj) {
                fprintf(pFile, "%.10f\n", relArr[jj]);
            }
            fclose(pFile);
        }
#endif

        resultKooNGeneric[ii].rbdDim = rbdTests[ii];
        resultKooNIdentical[ii].rbdDim = rbdTests[ii];

        for (jj = 0; jj < NUM_RUNS; ++jj) {
            minComponents = (rbdTests[ii].numComponents / 2) + (rbdTests[ii].numComponents & 1);
            printf("KooN experiment %d (%d/%lu) - %doo%d, times %d\n", jj+1, ii+1, (unsigned long)NUM_EXPERIMENTS, minComponents, rbdTests[ii].numComponents, rbdTests[ii].numTimes);
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

        free(relMat);
        free(relArr);
        free(output);
    }

#if PRINT_INPUT != 0
    inputPrinted = 1;
#endif
#endif

#if defined(TEST_BRIDGE)
    for (ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        relMat = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numComponents * rbdBridgeTests[ii].numTimes);
        relArr = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numTimes);
        output = (double *)malloc(sizeof(double) * rbdBridgeTests[ii].numTimes);

        for (kk = 0; kk < rbdBridgeTests[ii].numComponents; ++kk) {
            for (jj = 0; jj < rbdBridgeTests[ii].numTimes; ++jj) {
                relMat[jj + kk * rbdBridgeTests[ii].numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
            }
        }
        for (jj = 0; jj < rbdBridgeTests[ii].numTimes; ++jj) {
            relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
        }

#if PRINT_INPUT != 0
        snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
        pFile = fopen(filename, "w");
        for (jj = 0; jj < rbdBridgeTests[ii].numComponents; ++jj) {
            fprintf(pFile, "Lambda %2u    ", jj);
        }
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdBridgeTests[ii].numComponents; ++jj) {
            fprintf(pFile, "%.10f ", lambda[jj]);
        }
        fprintf(pFile, "\n");
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdBridgeTests[ii].numTimes; ++jj) {
            for (kk = 0; kk < rbdBridgeTests[ii].numComponents; ++kk) {
                fprintf(pFile, "%.10f ", relMat[jj + rbdBridgeTests[ii].numTimes * kk]);
            }
            fprintf(pFile, "\n");
        }
        fclose(pFile);

        snprintf(filename, sizeof(filename), "in_identical_%dx%d.txt", rbdBridgeTests[ii].numComponents, rbdBridgeTests[ii].numTimes);
        pFile = fopen(filename, "w");
        fprintf(pFile, "Lambda\n");
        fprintf(pFile, "%.10f ", lambda[0]);
        fprintf(pFile, "\n");
        fprintf(pFile, "\n");
        for (jj = 0; jj < rbdBridgeTests[ii].numTimes; ++jj) {
            fprintf(pFile, "%.10f\n", relArr[jj]);
        }
        fclose(pFile);
#endif

        resultBridgeGeneric[ii].rbdDim = rbdBridgeTests[ii];
        resultBridgeIdentical[ii].rbdDim = rbdBridgeTests[ii];

        for (jj = 0; jj < NUM_RUNS; ++jj) {
            printf("Bridge experiment %d (%d/%lu) - Times %d\n", jj+1, ii+1, (unsigned long)NUM_BRIDGE_EXPERIMENTS, rbdTests[ii].numTimes);
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

        free(relMat);
        free(relArr);
        free(output);
    }
#endif

#if defined(TEST_KOON_COMPARE)
    relMat = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numComponents * rbdKooNDimComparison.numTimes);
    relArr = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numTimes);
    output = (double *)malloc(sizeof(double) * rbdKooNDimComparison.numTimes);

    for (kk = 0; kk < rbdKooNDimComparison.numComponents; ++kk) {
        for (jj = 0; jj < rbdKooNDimComparison.numTimes; ++jj) {
            relMat[jj + kk * rbdKooNDimComparison.numTimes] = exp((0.0 - lambda[kk]) * (double)jj);
        }
    }
    for (jj = 0; jj < rbdKooNDimComparison.numTimes; ++jj) {
        relArr[jj] = exp((0.0 - lambda[0]) * (double)jj);
    }

#if PRINT_INPUT != 0
    snprintf(filename, sizeof(filename), "in_generic_%dx%d.txt", rbdKooNDimComparison.numComponents, rbdKooNDimComparison.numTimes);
    pFile = fopen(filename, "w");
    for (jj = 0; jj < rbdKooNDimComparison.numComponents; ++jj) {
        fprintf(pFile, "Lambda %2u    ", jj);
    }
    fprintf(pFile, "\n");
    for (jj = 0; jj < rbdKooNDimComparison.numComponents; ++jj) {
        fprintf(pFile, "%.10f ", lambda[jj]);
    }
    fprintf(pFile, "\n");
    fprintf(pFile, "\n");
    for (jj = 0; jj < rbdKooNDimComparison.numTimes; ++jj) {
        for (kk = 0; kk < rbdKooNDimComparison.numComponents; ++kk) {
            fprintf(pFile, "%.10f ", relMat[jj + rbdTests[ii].numTimes * kk]);
        }
        fprintf(pFile, "\n");
    }
    fclose(pFile);

    snprintf(filename, sizeof(filename), "in_identical_%dx%d.txt", rbdKooNDimComparison.numComponents, rbdKooNDimComparison.numTimes);
    pFile = fopen(filename, "w");
    fprintf(pFile, "Lambda\n");
    fprintf(pFile, "%.10f ", lambda[0]);
    fprintf(pFile, "\n");
    fprintf(pFile, "\n");
    for (jj = 0; jj < rbdKooNDimComparison.numTimes; ++jj) {
        fprintf(pFile, "%.10f\n", relArr[jj]);
    }
    fclose(pFile);
#endif

    for (ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        resultKooNGenericComparison[ii].rbdDim = rbdKooNDimComparison;
        resultKooNIdenticalComparison[ii].rbdDim = rbdKooNDimComparison;

        for (jj = 0; jj < NUM_RUNS; ++jj) {
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

    free(relMat);
    free(relArr);
    free(output);
#endif

#if defined(TEST_SERIES)
    pFile = fopen("log_series_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultSeriesGeneric[ii].rbdDim.numComponents, resultSeriesGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultSeriesGeneric[ii].minTime.tv_sec, (resultSeriesGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultSeriesGeneric[ii].maxTime.tv_sec, (resultSeriesGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultSeriesGeneric[ii].medianTime.tv_sec, (resultSeriesGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_series_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultSeriesIdentical[ii].rbdDim.numComponents, resultSeriesIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultSeriesIdentical[ii].minTime.tv_sec, (resultSeriesIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultSeriesIdentical[ii].maxTime.tv_sec, (resultSeriesIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultSeriesIdentical[ii].medianTime.tv_sec, (resultSeriesIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);
#endif

#if defined(TEST_PARALLEL)
    pFile = fopen("log_parallel_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultParallelGeneric[ii].rbdDim.numComponents, resultParallelGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultParallelGeneric[ii].minTime.tv_sec, (resultParallelGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultParallelGeneric[ii].maxTime.tv_sec, (resultParallelGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultParallelGeneric[ii].medianTime.tv_sec, (resultParallelGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_parallel_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultParallelIdentical[ii].rbdDim.numComponents, resultParallelIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultParallelIdentical[ii].minTime.tv_sec, (resultParallelIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultParallelIdentical[ii].maxTime.tv_sec, (resultParallelIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultParallelIdentical[ii].medianTime.tv_sec, (resultParallelIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);
#endif

#if defined(TEST_KOON)
    pFile = fopen("log_koon_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        minComponents = (resultKooNGeneric[ii].rbdDim.numComponents / 2) + (resultKooNGeneric[ii].rbdDim.numComponents & 1);
        fprintf(pFile, "%uoo%u, %u, ", minComponents, resultKooNGeneric[ii].rbdDim.numComponents, resultKooNGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNGeneric[ii].minTime.tv_sec, (resultKooNGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNGeneric[ii].maxTime.tv_sec, (resultKooNGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultKooNGeneric[ii].medianTime.tv_sec, (resultKooNGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_EXPERIMENTS; ++ii) {
        minComponents = (resultKooNIdentical[ii].rbdDim.numComponents / 2) + (resultKooNIdentical[ii].rbdDim.numComponents & 1);
        fprintf(pFile, "%uoo%u, %u, ", minComponents, resultKooNIdentical[ii].rbdDim.numComponents, resultKooNIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNIdentical[ii].minTime.tv_sec, (resultKooNIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNIdentical[ii].maxTime.tv_sec, (resultKooNIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultKooNIdentical[ii].medianTime.tv_sec, (resultKooNIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);
#endif

#if defined(TEST_KOON_COMPARE)
    pFile = fopen("log_koon_comparison_generic.txt", "w");
    fprintf(pFile, "Blocks, Min, Max, Median\n");
    for (ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        fprintf(pFile, "%uoo%u, ", ii, resultKooNGenericComparison[ii].rbdDim.numComponents);
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNGenericComparison[ii].minTime.tv_sec, (resultKooNGenericComparison[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNGenericComparison[ii].maxTime.tv_sec, (resultKooNGenericComparison[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultKooNGenericComparison[ii].medianTime.tv_sec, (resultKooNGenericComparison[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_koon_comparison_identical.txt", "w");
    fprintf(pFile, "Blocks, Min, Max, Median\n");
    for (ii = 0; ii < KOON_COMPARISON_TESTS; ++ii) {
        fprintf(pFile, "%uoo%u, ", ii, resultKooNIdenticalComparison[ii].rbdDim.numComponents);
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNIdenticalComparison[ii].minTime.tv_sec, (resultKooNIdenticalComparison[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultKooNIdenticalComparison[ii].maxTime.tv_sec, (resultKooNIdenticalComparison[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultKooNIdenticalComparison[ii].medianTime.tv_sec, (resultKooNIdenticalComparison[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);
#endif

#if defined(TEST_BRIDGE)
    pFile = fopen("log_bridge_generic.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultBridgeGeneric[ii].rbdDim.numComponents, resultBridgeGeneric[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultBridgeGeneric[ii].minTime.tv_sec, (resultBridgeGeneric[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultBridgeGeneric[ii].maxTime.tv_sec, (resultBridgeGeneric[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultBridgeGeneric[ii].medianTime.tv_sec, (resultBridgeGeneric[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);

    pFile = fopen("log_bridge_identical.txt", "w");
    fprintf(pFile, "Blocks, Times, Min, Max, Median\n");
    for (ii = 0; ii < NUM_BRIDGE_EXPERIMENTS; ++ii) {
        fprintf(pFile, "%u, %u, ", resultBridgeIdentical[ii].rbdDim.numComponents, resultBridgeIdentical[ii].rbdDim.numTimes);
        fprintf(pFile, "%ld.%06ld, ", (long)resultBridgeIdentical[ii].minTime.tv_sec, (resultBridgeIdentical[ii].minTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld, ", (long)resultBridgeIdentical[ii].maxTime.tv_sec, (resultBridgeIdentical[ii].maxTime.tv_nsec / 1000));
        fprintf(pFile, "%ld.%06ld\n", (long)resultBridgeIdentical[ii].medianTime.tv_sec, (resultBridgeIdentical[ii].medianTime.tv_nsec / 1000));
    }
    fclose(pFile);
#endif

    return 0;
}
