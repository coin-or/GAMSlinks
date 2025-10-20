/* Copyright (C) GAMS Development and others
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsHelper.c
 * @author Stefan Vigerske
 */

#include "GamsHelper.h"
#include "GamsLinksConfig.h"

#include <stdio.h>
#include <stdlib.h>

#include "gevmcc.h"

#if defined _OPENMP
extern
void omp_set_num_threads(int);
#endif

#ifdef HAVE_GOTO_SETNUMTHREADS
void goto_set_num_threads(int);
#endif

void GAMSsetNumThreads(
   struct gevRec*      gev,                /**< GAMS environment */
   int                 nthreads            /**< number of threads for OpenMP/GotoBlas */
)
{
#ifdef HAVE_GOTO_SETNUMTHREADS
   //if( gev != NULL && nthreads > 1 )
   //{
   //   char msg[100];
   //   sprintf(msg, "Number of GotoBlas threads: %d.\n", nthreads);
   //   gevLogPChar(gev, msg);
   //}
   goto_set_num_threads(nthreads);
#endif

#ifdef _OPENMP
   //if( gev != NULL && nthreads > 1 )
   //{
   //   char msg[100];
   //   sprintf(msg, "Number of OpenMP threads: %d.\n", nthreads);
   //   gevLogPChar(gev, msg);
   //}
   omp_set_num_threads(nthreads);
#endif

#ifdef __APPLE__
   {
      char buf[10];
      snprintf(buf, 10, "%d", nthreads);
      setenv("VECLIB_MAXIMUM_THREADS", buf, 1);
   }
#endif
}
