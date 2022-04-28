// Copyright (C) GAMS Development and others 2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "convert_nl.h"

#include "gmomcc.h"
#include "gevmcc.h"

#define AMPLINFTY    1.0e50

typedef struct
{
   gmoHandle_t gmo;
   gevHandle_t gev;

   char nlfilename[GMS_SSSIZE + 30];

} amplsolver;

RETURN writeNL(
   amplsolver* as
   )
{
   convertWriteNLopts writeopts;

   gevGetStrOpt(as->gev, gevNameScrDir, as->nlfilename);
   strcat(as->nlfilename, "prob.nl");

   writeopts.filename = as->nlfilename;
   writeopts.binary = 1;

   if( convertWriteNL(as->gmo, writeopts) == RETURN_ERROR )
   {
      gmoSolveStatSet(as->gmo, gmoSolveStat_Capability);
      gmoModelStatSet(as->gmo, gmoModelStat_ErrorNoSolution);
      return RETURN_OK;
   }

   return RETURN_OK;
}



#define GAMSSOLVER_ID amp
#include "GamsEntryPoints_tpl.c"

void ampInitialize(void)
{
   gmoInitMutexes();
   gevInitMutexes();
}

void ampFinalize(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
}

DllExport int STDCALL ampCreate(
   void**   Cptr,
   char*    msgBuf,
   int      msgBufLen
)
{
   assert(Cptr != NULL);
   assert(msgBufLen > 0);
   assert(msgBuf != NULL);

   *Cptr = calloc(1, sizeof(amplsolver));

   msgBuf[0] = 0;

   return 1;
}

DllExport void STDCALL ampFree(
   void** Cptr
)
{
   amplsolver* as;

   assert(Cptr != NULL);
   assert(*Cptr != NULL);

   as = (amplsolver*) *Cptr;
   free(as);

   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
}

DllExport int STDCALL ampReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
)
{
   amplsolver* as;

   assert(Cptr != NULL);
   assert(Gptr != NULL);

   char msg[256];
   if( !gmoGetReady(msg, sizeof(msg)) )
      return 1;
   if( !gevGetReady(msg, sizeof(msg)) )
      return 1;

   as = (amplsolver*) Cptr;
   as->gmo = Gptr;
   as->gev = (gevHandle_t) gmoEnvironment(as->gmo);

   /* get the problem into a normal form */
   gmoObjStyleSet(as->gmo, gmoObjType_Fun);
   gmoObjReformSet(as->gmo, 1);
   gmoIndexBaseSet(as->gmo, 0);
   gmoSetNRowPerm(as->gmo); /* hide =N= rows */
   gmoMinfSet(as->gmo, -AMPLINFTY);
   gmoPinfSet(as->gmo, AMPLINFTY);

   gmoModelStatSet(as->gmo, gmoModelStat_NoSolutionReturned);
   gmoSolveStatSet(as->gmo, gmoSolveStat_SystemErr);

   return 0;
}

DllExport int STDCALL ampCallSolver(
   void* Cptr
)
{
   amplsolver* as;

   as = (amplsolver*) Cptr;
   assert(as->gmo != NULL);
   assert(as->gev != NULL);

   /* get the problem into a normal form */
   gmoObjStyleSet(as->gmo, gmoObjType_Fun);
   gmoObjReformSet(as->gmo, 1);
   gmoIndexBaseSet(as->gmo, 0);
   // gmoSetNRowPerm(as->gmo); /* hide =N= rows */
   gmoMinfSet(as->gmo, -AMPLINFTY);
   gmoPinfSet(as->gmo, AMPLINFTY);

   writeNL(as);

   if( gmoSolveStat(as->gmo) == gmoSolveStat_Capability )
      return 0;

   gevTimeSetStart(as->gev);

   gmoSetHeadnTail(as->gmo, gmoHresused, gevTimeDiffStart(as->gev));

   if( !gevGetIntOpt(as->gev, gevKeep) )
   {
      int stublen;

      stublen = strlen(as->nlfilename)-2;
      if( remove(as->nlfilename) != 0 )
         fprintf(stderr, "Could not remove temporary file %s\n", as->nlfilename);

      strcpy(as->nlfilename + stublen, "col");
      if( remove(as->nlfilename) != 0 )
         fprintf(stderr, "Could not remove temporary file %s\n", as->nlfilename);

      strcpy(as->nlfilename + stublen, "row");
      if( remove(as->nlfilename) != 0 )
         fprintf(stderr, "Could not remove temporary file %s\n", as->nlfilename);
   }

   return 0;
}
