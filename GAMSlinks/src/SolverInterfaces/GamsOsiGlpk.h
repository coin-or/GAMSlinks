// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsOsiCplex.h 739 2009-09-09 21:08:08Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSOSIGLPK_H_
#define GAMSOSIGLPK_H_

#include "GamsSolver.h"

typedef void oglRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL oglCallSolver(oglRec_t *Cptr);
	DllExport int  STDCALL oglModifyProblem(oglRec_t *Cptr);
	DllExport int  STDCALL oglHaveModifyProblem(oglRec_t *Cptr);
	DllExport int  STDCALL oglReadyAPI(oglRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL oglFree(oglRec_t **Cptr);
	DllExport void STDCALL oglCreate(oglRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSIGLPK_H_*/
