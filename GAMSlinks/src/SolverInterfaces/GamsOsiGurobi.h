// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOSIGUROBI_H_
#define GAMSOSIGUROBI_H_

#include "GamsSolver.h"

typedef void oguRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL oguCallSolver(oguRec_t *Cptr);
	DllExport int  STDCALL oguModifyProblem(oguRec_t *Cptr);
	DllExport int  STDCALL oguHaveModifyProblem(oguRec_t *Cptr);
	DllExport int  STDCALL oguReadyAPI(oguRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL oguFree(oguRec_t **Cptr);
	DllExport void STDCALL oguCreate(oguRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSIGUROBI_H_*/
