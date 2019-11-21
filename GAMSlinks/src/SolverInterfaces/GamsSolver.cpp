// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsSolver.hpp"

#include <cstdio>
#include <cassert>
#include <cstdlib>

#include "gmomcc.h"
#include "gevmcc.h"

#include "GamsCompatibility.h"

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
