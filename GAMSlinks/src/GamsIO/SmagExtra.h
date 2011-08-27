/* Copyright (C) 2008
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id: SmagExtra.h 510 2008-08-16 19:31:27Z stefan $

 Author: Stefan Vigerske
*/

#ifndef SMAGEXTRA_H_
#define SMAGEXTRA_H_

#include "GAMSlinksConfig.h"

#include "smag.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Gives the structure of the Hessian of the objective and each constraint separately.
 * The user provides space to store the row/col indices and values of hessian entries.
 * The Hessians of all constraints and the objective are stored in one contiguous array.
 * Only the values from the lower-left part of the Hessian are returned.
 * rowStart indicates where the entries for which constraint of the Hessian start. The entries for the objective are found at index rowstart[smagRowCount()].
 * That is, hesRowIdx[rowStart[connr]..rowStart[connr]-1] are the row indices of the elements in the Hessian of constraint connr if connr<smagRowCount() or the objective if connr==smagRowCount().
 * Similar for hesColIdx.
 * The Hessian values are computed in the initial level values of the variables and are stored in hesValue. Giving NULL for hesValue switches off the computation of hessian entry values.
 * hesSize should be large enough to store the indices and columns of all Hessians. GAMS uses the estimate 10*prob->gms.nnz*prob->gms.workfactor for the size of the Hessian of the Lagrangian.      
 * @param prob Pointer to smag structure.
 * @param hesRowIdx Storage for row indices for each Hessian of size at least hesSize.
 * @param hesColIdx Storage for column indices for each Hessian of size at least hesSize.
 * @param hesValue Storage for values each Hessian of size at least hesSize or NULL.
 * @param hesSize The size of the first three arrays.
 * @param rowStart Storage for indices where the Hessian of objective and each constraint starts of size at least smagRowCount()+2.
 * @return 0 on success, -1 if the size hesSize was too small, >0 if there was an error evaluating a Hessian. 
 */ 
int smagSingleHessians(smagHandle_t prob, int* hesRowIdx, int* hesColIdx, double* hesValue, int hesSize, int* rowStart);

/** EXPERIMENTAL FUNCTION: Computes the directional derivative of the objective function.
 * Given a vector x and a direction d, computes the function value f(x) and the product <grad f(x), d>.
 * @param prob Pointer to smag structure.
 * @param x Array of length smagColCount(prob) containing the point where f(x) is evaluated.
 * @param d Array of length smagColCount(prob) containing the direction in which the derivative is computed.
 * @param f Storage for function value.
 * @param df_d Storage for directional derivative.
 * @return 0 on success, nonzero on failure.
 */
int smagEvalObjGradProd(smagHandle_t prob, double* x, double* d, double* f, double* df_d);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /*SMAGEXTRA_H_*/
