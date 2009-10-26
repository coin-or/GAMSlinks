// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSSCIP_H_
#define GAMSSCIP_H_

#include "GamsSolver.h"

typedef void scpRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL scpCallSolver(scpRec_t *Cptr);
	DllExport int  STDCALL scpModifyProblem(scpRec_t *Cptr);
	DllExport int  STDCALL scpHaveModifyProblem(scpRec_t *Cptr);
	DllExport int  STDCALL scpReadyAPI(scpRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL scpFree(scpRec_t **Cptr);
	DllExport void STDCALL scpCreate(scpRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSSCIP_H_*/
