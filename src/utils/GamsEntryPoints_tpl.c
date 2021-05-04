/** Copyright (C) GAMS Development and others 2011
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

#ifndef GAMSSOLVER_CONCAT3_
#define GAMSSOLVER_CONCAT3_(a, b, c) a ## b ## c
#endif

#ifndef GAMSSOLVER_CONCAT3
#define GAMSSOLVER_CONCAT3(a, b, c) GAMSSOLVER_CONCAT3_(a, b, c)
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
DllExport void STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,Initialize)(void);

/* Implemented below */
DllExport void STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,Finalize)(void);

/* Implemented below */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCreate)(void** Cptr);

/* Implemented below */
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XFree)(void** Cptr);

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg);

/* Needs to be implemented by solver interface */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,ReadyAPI)(void* Cptr, struct gmoRec* Gptr, struct optRec* Optr);

/* Needs to be implemented by solver interface */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,CallSolver)(void* Cptr);

#ifdef GAMSSOLVER_HAVEMODIFYPROBLEM
/* Needs to be implemented by solver interface if GAMSSOLVER_HAVEMODIFYPROBLEM is defined */
DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr);

/* Needs to be implemented by solver interface if modify-problem is supported */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr);

/* Needs to be implemented by solver interface if modify-problem is supported */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,ModifyProblem)(void* Cptr);
#endif

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,XAPIVersion)(int api, char* Msg, int* comp);

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(D__,GAMSSOLVER_ID,XAPIVersion)(int api, char* Msg, int* comp);

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg);

/* Implemented below */
DllExport int  STDCALL GAMSSOLVER_CONCAT3(D__,GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg);

DllExport void STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,Initialize)(void)
{
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)();
}

DllExport void STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,Finalize)(void)
{
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XCreate)(void** Cptr)
{
   char msg[255];
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Create)(Cptr, msg, sizeof(msg));
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,XFree)(void** Cptr)
{
   GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Free)(Cptr);
}

DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,ReadyAPI)(void* Cptr, struct gmoRec* Gptr, struct optRec* Optr)
{
   return GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(Cptr, Gptr);
}

DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   return GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(Cptr);
}

#ifdef GAMSSOLVER_HAVEMODIFYPROBLEM
DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr)
{
   return GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(Cptr);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr)
{
   return 0;
}

DllExport int  STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,ModifyProblem)(void* Cptr)
{
   return GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ModifyProblem)(Cptr);
}
#endif

/* comp returns the compatibility mode:
   0: client is too old for the DLL, no compatibility
   1: client version and DLL version are the same, full compatibility
   2: client is older than DLL, but defined as compatible, backward compatibility
   3: client is newer than DLL, forward compatibility
   FIXME: for now, we just claim full compatibility
*/
DllExport int STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,XAPIVersion)(int api, char* Msg, int* comp)
{
  *comp = 1;
  return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT3(D__,GAMSSOLVER_ID,XAPIVersion)(int api, char* Msg, int* comp)
{
  *comp = 1;
  return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT3(C__,GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg)
{
   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT3(D__,GAMSSOLVER_ID,XCheck)(const char* funcn, int ClNrArg, int Clsign[], char* Msg)
{
   return 1;
}

#if defined(__cplusplus)
}
#endif

