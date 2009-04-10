// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_H_
#define GAMSIPOPT_H_

#include "GamsSolver.h"

typedef void ipoRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL ipoCallSolver(ipoRec_t *Cptr);
	DllExport int  STDCALL ipoModifyProblem(ipoRec_t *Cptr);
	DllExport int  STDCALL ipoHaveModifyProblem(ipoRec_t *Cptr);
	DllExport int  STDCALL ipoReadyAPI(ipoRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, gcdHandle_t Dptr);
	DllExport void STDCALL ipoFree(ipoRec_t **Cptr);
	DllExport void STDCALL ipoCreate(ipoRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSIPOPT_H_*/
