/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsHelper.h
 * @author Stefan Vigerske
 */

#ifndef GAMSHELPER_H_
#define GAMSHELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gevRec* gevHandle_t;

/** sets number of OpenMP/GotoBlas/Apple threads */
void GAMSsetNumThreads(
   struct gevRec*      gev,                /**< GAMS environment */
   int                 nthreads            /**< number of threads for OpenMP/GotoBlas */
);

#ifdef __cplusplus
}
#endif

#endif /* GAMSLINKS_SRC_GAMSUTILS_GAMSHELPER_H_ */
