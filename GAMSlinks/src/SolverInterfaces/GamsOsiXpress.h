// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOSIXPRESS_H_
#define GAMSOSIXPRESS_H_

#include "GamsSolver.h"

typedef void oxpRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL oxpCallSolver(oxpRec_t *Cptr);
	DllExport int  STDCALL oxpModifyProblem(oxpRec_t *Cptr);
	DllExport int  STDCALL oxpHaveModifyProblem(oxpRec_t *Cptr);
	DllExport int  STDCALL oxpReadyAPI(oxpRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL oxpFree(oxpRec_t **Cptr);
	DllExport void STDCALL oxpCreate(oxpRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSIXPRESS_H_*/
