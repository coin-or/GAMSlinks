// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSBONMIN_H_
#define GAMSBONMIN_H_

#include "GamsSolver.h"

typedef void bonRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL bonCallSolver(bonRec_t *Cptr);
	DllExport int  STDCALL bonModifyProblem(bonRec_t *Cptr);
	DllExport int  STDCALL bonHaveModifyProblem(bonRec_t *Cptr);
	DllExport int  STDCALL bonReadyAPI(bonRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, dctHandle_t Dptr);
	DllExport void STDCALL bonFree(bonRec_t **Cptr);
	DllExport void STDCALL bonCreate(bonRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSBONMIN_H_*/
