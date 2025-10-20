/** Copyright (C) GAMS Development and others
 *  All Rights Reserved.
 *  This code is published under the Eclipse Public License.
 *
 *  @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"

typedef struct optRec* optHandle_t;

/* check that three-letter id of solver is defined */
#ifndef GAMSSOLVER_ID
#error You need to define GAMSSOLVER_ID
#endif

/* we need to redirect the CONCAT macro to another one to ensure that macro arguments are substituted too */

#ifndef GAMSSOLVER_CONCAT_
#define GAMSSOLVER_CONCAT_(a, b) a ## b
#endif

#ifndef GAMSSOLVER_CONCAT
#define GAMSSOLVER_CONCAT(a, b) GAMSSOLVER_CONCAT_(a, b)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Needs to be implemented by solver interface */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void);

/* Needs to be implemented by solver interface */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void);

/* Needs to be implemented by solver interface */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Create)(void** Cptr, char* msgBuf, int msgBufLen);

/* Needs to be implemented by solver interface */
DllExport void  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Free)(void** Cptr);

/* Needs to be implemented by solver interface */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, struct gmoRec* Gptr);

/* Needs to be implemented by solver interface */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr);

#ifdef GAMSSOLVER_HAVEMODIFYPROBLEM
/* Needs to be implemented by solver interface if GAMSSOLVER_HAVEMODIFYPROBLEM is defined */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ModifyProblem)(void* Cptr);
#endif

/* Compatibility entry points so the solver link library can work with a GAMS version prior to GAMS 36 */

/* Implemented below */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCreate)(void** Cptr);

/* Implemented below */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XFree)(void** Cptr);

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg);

#ifdef GAMSSOLVER_HAVEMODIFYPROBLEM
/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr);
#endif

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCreate)(void** Cptr)
{
   char msg[255];
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Create)(Cptr, msg, sizeof(msg));
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XFree)(void** Cptr)
{
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Free)(Cptr);
}

DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg)
{
   return 1;
}

#ifdef GAMSSOLVER_HAVEMODIFYPROBLEM
DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr)
{
   return 0;
}
#endif

#if defined(__cplusplus)
}
#endif

