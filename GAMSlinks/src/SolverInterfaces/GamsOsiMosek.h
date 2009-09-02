// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsOsiMosek.h 660 2009-04-16 19:59:35Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSOSIMOSEK_H_
#define GAMSOSIMOSEK_H_

#include "GamsSolver.h"

typedef void omkRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL omkCallSolver(omkRec_t *Cptr);
	DllExport int  STDCALL omkModifyProblem(omkRec_t *Cptr);
	DllExport int  STDCALL omkHaveModifyProblem(omkRec_t *Cptr);
	DllExport int  STDCALL omkReadyAPI(omkRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL omkFree(omkRec_t **Cptr);
	DllExport void STDCALL omkCreate(omkRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSIMOSEK_H_*/
