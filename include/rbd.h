/*
 *  Component: rbd.h
 *  RBD library APIs
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

#ifndef RBD_H_
#define RBD_H_


#ifdef  __cplusplus
extern "C" {
#endif


#define RBD_BRIDGE_COMPONENTS       5       /* Number of components in Bridge RBD block */
#define RBD_HOT_STANDBY_COMPONENTS  3       /* Number of components in Hot Stand-by RBD block */
#define RBD_COLD_STANDBY_COMPONENTS 3       /* Number of components in Cold Stand-by RBD block */


/* Declare extern symbols */
#if   defined(_MSC_VER)
#if   defined(COMPILE_DLL)
#define EXTERN          extern __declspec(dllexport)
#elif defined(COMPILE_LIB)      || defined(LINK_TO_LIB)
#define EXTERN          extern
#else
#define EXTERN          extern __declspec(dllimport)
#endif
#else
#define EXTERN          extern
#endif


/**
 * rbdSeriesGeneric
 *
 * Compute reliability of a generic Series RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic Series RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Series RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Series RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Series RBD system (N)
 *      numTimes: number of time instants over which Series RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdSeriesGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

/**
 * rbdSeriesIdentical
 *
 * Compute reliability of an identical Series RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an identical Series RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Series RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Series RBD system
 *      numTimes: number of time instants over which Series RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdSeriesIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

/**
 * rbdParallelGeneric
 *
 * Compute reliability of a generic Parallel RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic Parallel RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Parallel RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Parallel RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Parallel RBD system (N)
 *      numTimes: number of time instants over which Parallel RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdParallelGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

/**
 * rbdParallelIdentical
 *
 * Compute reliability of an identical Parallel RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an identical Parallel RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Parallel RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Parallel RBD system
 *      numTimes: number of time instants over which Parallel RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdParallelIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

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
EXTERN int rbdKooNGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes);

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
EXTERN int rbdKooNIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned char minComponents, unsigned int numTimes);

/**
 * rbdBridgeIdentical
 *
 * Compute reliability of an identical Bridge RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of an identical Bridge RBD system,
 *  i.e. a system for which the components are identical
 *
 * Parameters:
 *      reliabilities: this array contains the input reliabilities of all components
 *                      at the provided time instants
 *      output: this array contains the reliabilities of Bridge RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Bridge RBD system The number of
 *                      components in a Bridge RBD block must be equal to 5
 *      numTimes: number of time instants over which Bridge RBD shall be computed
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdBridgeIdentical(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

/**
 * rbdBridgeGeneric
 *
 * Compute reliability of a generic Bridge RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a generic Bridge RBD system,
 *  i.e. a system for which the components are not identical
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Bridge RBD
 *                      system and T is the number of time instants
 *      output: this array contains the reliabilities of Bridge RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Bridge RBD system (N) The number of
 *                      components in a Bridge RBD block must be equal to 5
 *      numTimes: number of time instants over which Bridge RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdBridgeGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);

/**
 * rbdHotStandby
 *
 * Compute reliability of a Hot Stand-by RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *      double deltaT
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a Hot Stand-by RBD system
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Hot Stand-by RBD
 *                      system and T is the number of time instants. The first component
 *                      identifies the primary, the second one is the reserve and the third
 *                      one is the switch unit
 *      output: this array contains the reliabilities of Hot Stand-by RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Hot Stand-by RBD system (N). The number of
 *                      components in a Hot Stand-by RBD block must be equal to 3
 *      numTimes: number of time instants over which Hot Stand-by RBD shall be computed (T)
 *      deltaT: time difference between two consecutive time instants
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdHotStandby(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, double deltaT);

/**
 * rbdColdStandby
 *
 * Compute reliability of a Cold Stand-by RBD system
 *
 * Input:
 *      double *reliabilities
 *      unsigned char numComponents
 *      unsigned int numTimes
 *      double deltaT
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the reliabilities over time of a Cold Stand-by RBD system
 *
 * Parameters:
 *      reliabilities: this matrix contains the input reliabilities of all components
 *                      at the provided time instants. The matrix shall be provided as
 *                      a NxT one, where N is the number of components of Cold Stand-by RBD
 *                      system and T is the number of time instants. The first component
 *                      identifies the primary, the second one is the reserve and the third
 *                      one is the switch unit
 *      output: this array contains the reliabilities of Cold Stand-by RBD system computed at
 *                      the provided time instants
 *      numComponents: number of components in Cold Stand-by RBD system (N). The number of
 *                      components in a Cold Stand-by RBD block must be equal to 3
 *      numTimes: number of time instants over which Cold Stand-by RBD shall be computed (T)
 *      deltaT: time difference between two consecutive time instants
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdColdStandby(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes, double deltaT);

/**
 * rbdUnreliability
 *
 * Compute unreliability given the RBD reliability curve
 *
 * Input:
 *      double *reliability
 *      unsigned int numTimes
 *
 * Output:
 *      double *unreliability
 *
 * Description:
 *  This function computes the unreliability curve over time given its corresponding
 *  reliability curve
 *
 * Parameters:
 *      reliability: this array contains the input reliability curve at the provided
 *                      time instants
 *      unreliability: this array is filled with the unreliability curve at the
 *                      provided time instants
 *      numTimes: number of time instants over which unreliability curve shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN double *rbdUnreliability(double *reliability, double *unreliability, unsigned int numTimes);

/**
 * rbdReliabilitySum
 *
 * Compute the sum of two reliability curves
 *
 * Input:
 *      double *r1
 *      double *r2
 *      unsigned int numTimes
 *
 * Output:
 *      double *output
 *
 * Description:
 *  This function computes the sum of the two provided reliability curves over time
 *
 * Parameters:
 *      r1: this array contains the first input reliability curve at the provided
 *                      time instants
 *      r2: this array contains the second input reliability curve at the provided
 *                      time instants
 *      output: this array is filled with the sum of the reliability curves at the
 *                      provided time instants
 *      numTimes: number of time instants over which unreliability curve shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN double *rbdReliabilitySum(double *r1, double *r2, double *output, unsigned int numTimes);


#ifdef  __cplusplus
}
#endif


#endif /* RBD_H_ */
