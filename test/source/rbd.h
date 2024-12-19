/*
 *  Component: rbd.h
 *  RBD library APIs
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

#ifndef RBD_H_
#define RBD_H_


#ifdef  __cplusplus
extern "C" {
#endif


#define RBD_BRIDGE_COMPONENTS       5       /* Number of components in Bridge RBD block */


/* Declare extern symbols */
#if   defined(_MSC_VER)
#if   defined(COMPILE_DLL)
#define EXTERN          extern __declspec(dllexport)
#elif defined(COMPILE_LIB)
#define EXTERN          extern
#elif defined(LINK_TO_LIB)
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
 *      numComponents: number of components in Bridge RBD system
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
 *      numComponents: number of components in Bridge RBD system (N)
 *      numTimes: number of time instants over which Bridge RBD shall be computed (T)
 *
 * Return (int):
 *  0 in case of successful computation, < 0 otherwise
 */
EXTERN int rbdBridgeGeneric(double *reliabilities, double *output, unsigned char numComponents, unsigned int numTimes);


#ifdef  __cplusplus
}
#endif


#endif /* RBD_H_ */
