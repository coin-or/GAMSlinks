// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Common Public License.
//
// Author: Stefan Vigerske

#include "GamsSolver.hpp"
#include "GAMSlinksConfig.h"

#include <cstdio>
#include <cassert>

#include "gmomcc.h"
#include "gevmcc.h"

#ifdef GAMS_BUILD
#include "gmspal.h"
#endif

#include "GamsCompatibility.h"

#ifdef HAVE_GOTO_SETNUMTHREADS
extern "C" void goto_set_num_threads(int);
#endif

#ifdef HAVE_MKL_SETNUMTHREADS
extern "C" void MKL_Domain_Set_Num_Threads(int, int);
//extern "C" void MKL_Set_Num_Threads(int);
#endif

void GamsSolver::setNumThreadsBlas(
   struct gevRec*      gev,                /**< GAMS environment */
   int                 nthreads            /**< number of threads for BLAS routines */
)
{
#ifdef HAVE_GOTO_SETNUMTHREADS
   if( gev != NULL && nthreads > 1 )
   {
      char msg[100];
      sprintf(msg, "Limit number of threads in GotoBLAS to %d.\n", nthreads);
      gevLogPChar(gev, msg);
   }
   goto_set_num_threads(nthreads);
#endif

#ifdef HAVE_MKL_SETNUMTHREADS
   if( gev != NULL && nthreads > 1 )
   {
      char msg[100];
      sprintf(msg, "Limit number of threads in MKL BLAS to %d.\n", nthreads);
      gevLogPChar(gev, msg);
   }
   MKL_Domain_Set_Num_Threads(nthreads, 1); // 1 = Blas
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

GamsSolver::~GamsSolver()
{
   gmoLibraryUnload();
   gevLibraryUnload();
}

bool GamsSolver::checkLicense(
   struct gmoRec*     gmo                 /**< GAMS modeling object */
)
{
#ifdef GAMS_BUILD
   gevRec* gev = (gevRec*)gmoEnvironment(gmo);
#define GEVPTR gev
#include "cmagic2.h"
   if( licenseCheck(gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo)) )
   {
      char msg[256];
      gevLogStat(gev, "The license check failed:");
      while( licenseGetMessage(msg, sizeof(msg)) )
         gevLogStat(gev, msg);
      return false;
   }
   return true;
#undef GEVPTR
#else
   return true;
#endif
}

bool GamsSolver::checkAcademicLicense(
   struct gmoRec*     gmo                 /**< GAMS modeling object */
)
{
   assert(gmo != NULL);
#ifdef GAMS_BUILD
   gevRec* gev = (gevRec*)gmoEnvironment(gmo);

#define GEVPTR gev
/* bad bad bad */
#ifdef SUB_FR
#undef SUB_FR
#endif
#define SUB_SC
#include "cmagic2.h"
   if( licenseCheck(gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo)) )
   {
      // model larger than demo and no solver-specific license; check if we have an academic license
      int isAcademic = 0;
      licenseQueryOption("GAMS", "ACADEMIC", &isAcademic);
      if( !isAcademic )
      {
         char msg[256];
         while( licenseGetMessage(msg, sizeof(msg)) )
            gevLogStat(gev, msg);
         return false;
      }
   }
#undef SUB_OC
#undef GEVPTR
#endif

   return true;
}

bool GamsSolver::registerGamsCplexLicense(
   struct gmoRec*     gmo                 /**< GAMS modeling object */
)
{
   assert(gmo != NULL);
#if defined(COIN_HAS_OSICPX) && defined(GAMS_BUILD)
   gevRec* gev = (gevRec*)gmoEnvironment(gmo);

#define GEVPTR gev
   /* bad bad bad */
#ifdef SUB_FR
#undef SUB_FR
#endif
#define SUB_OC
#include "cmagic2.h"
   if( licenseCheck(gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo)) )
   {
      if( licenseCheckSubSys(2, "CPCL") )
      {
         gevLogStat(gev,"***");
         gevLogStat(gev,"*** LICENSE ERROR:");
         gevLogStat(gev,"*** See http://www.gams.com/osicplex/ for OsiCplex licensing information.");
         gevLogStat(gev,"***");
         return false;
      }
      else
      {
         int isLicMIP = 0;
         int isLicCP  = 1;
         licenseQueryOption("CPLEX", "MIP",     &isLicMIP);
         licenseQueryOption("CPLEX", "GMSLICE", &isLicCP);
         if( 0 == isLicMIP && 1 == isLicCP && gmoNDisc(gmo))
         {
            gevLogStat(gev,"*** MIP option not licensed. Can solve continuous models only.");
            return false;
         }
      }
   }
   else
   {
      int isLicMIP = 0;
      int isLicCP  = 1;
      licenseQueryOption("CPLEX", "MIP",     &isLicMIP);
      licenseQueryOption("CPLEX", "GMSLICE", &isLicCP);
      if( 0 == isLicMIP && 1 == isLicCP && gmoNDisc(gmo) )
      {
         gevLogStat(gev,"*** MIP option not licensed. Can solve continuous models only.");
         return false;
      }
   }
#undef SUB_OC
#undef GEVPTR
#endif

  return true;
}
