// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsSolver.hpp"
#ifdef HAVE_CONFIG_H
#include "GAMSlinksConfig.h"
#endif

#include <cstdio>
#include <cassert>
#include <cstdlib>

#include "gmomcc.h"
#include "gevmcc.h"

#ifdef GAMS_BUILD
#include "palmcc.h"
#endif

#include "GamsCompatibility.h"

#ifdef HAVE_GOTO_SETNUMTHREADS
extern "C" void goto_set_num_threads(int);
#endif

#ifdef HAVE_OMP_SETNUMTHREADS
extern "C" void omp_set_num_threads(int);
#endif

void GamsSolver::setNumThreads(
   struct gevRec*      gev,                /**< GAMS environment */
      int              nthreads            /**< number of threads for OpenMP/GotoBlas */
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

int GamsSolver::getGmoReady()
{
   if( !gmoLibraryLoaded() )
   {
      char msg[256];

      if( !gmoGetReady(msg, sizeof(msg)) )
      {
         fprintf(stderr, "Error loading GMO library: %s\n",msg);
         return 1;
      }
   }

   return 0;
}

int GamsSolver::getGevReady()
{
   if( !gevLibraryLoaded() )
   {
      char msg[256];

      if( !gevGetReady(msg, sizeof(msg)) )
      {
         fprintf(stderr, "Error loading GEV library: %s\n",msg);
         return 1;
      }
   }

   return 0;
}
