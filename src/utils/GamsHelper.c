/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsHelper.c
 * @author Stefan Vigerske
 */

#include "GamsHelper.h"
#ifdef HAVE_CONFIG_H
#include "GamsLinksConfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "gevmcc.h"

#ifdef HAVE_GOTO_SETNUMTHREADS
void goto_set_num_threads(int);
#endif

#ifdef HAVE_OMP_SETNUMTHREADS
void omp_set_num_threads(int);
#endif

void GAMSsetNumThreads(
   struct gevRec*      gev,                /**< GAMS environment */
   int                 nthreads            /**< number of threads for OpenMP/GotoBlas */
)
{
#ifdef HAVE_GOTO_SETNUMTHREADS
   if( gev != NULL && nthreads > 1 )
   {
      char msg[100];
      sprintf(msg, "Number of GotoBlas threads: %d.\n", nthreads);
      gevLogPChar(gev, msg);
   }
   goto_set_num_threads(nthreads);
#endif

#ifdef HAVE_OMP_SETNUMTHREADS
   if( gev != NULL && nthreads > 1 )
   {
      char msg[100];
      sprintf(msg, "Number of OpenMP threads: %d.\n", nthreads);
      gevLogPChar(gev, msg);
   }
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
