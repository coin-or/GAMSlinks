// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSOSISOPLEX_H_
#define GAMSOSISOPLEX_H_

#include "GamsSolver.h"

typedef void ospRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

DllExport int  STDCALL ospCallSolver(ospRec_t *Cptr);
DllExport int  STDCALL ospModifyProblem(ospRec_t *Cptr);
DllExport int  STDCALL ospHaveModifyProblem(ospRec_t *Cptr);
DllExport int  STDCALL ospReadyAPI(ospRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
DllExport void STDCALL ospFree(ospRec_t **Cptr);
DllExport void STDCALL ospCreate(ospRec_t **Cptr, char *msgBuf, int msgBufLen);
DllExport void STDCALL ospXCreate(ospRec_t **Cptr);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSISOPLEX_H_*/
