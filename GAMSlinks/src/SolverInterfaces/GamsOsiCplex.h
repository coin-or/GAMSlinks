// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSOSICPLEX_H_
#define GAMSOSICPLEX_H_

#include "GamsSolver.h"

typedef void ocpRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

DllExport int  STDCALL ocpCallSolver(ocpRec_t *Cptr);
DllExport int  STDCALL ocpModifyProblem(ocpRec_t *Cptr);
DllExport int  STDCALL ocpHaveModifyProblem(ocpRec_t *Cptr);
DllExport int  STDCALL ocpReadyAPI(ocpRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
DllExport void STDCALL ocpFree(ocpRec_t **Cptr);
DllExport void STDCALL ocpCreate(ocpRec_t **Cptr, char *msgBuf, int msgBufLen);
DllExport void STDCALL ocpXCreate(ocpRec_t **Cptr);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSICPLEX_H_*/
