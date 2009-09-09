// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSCBC_H_
#define GAMSCBC_H_

#include "GamsSolver.h"

typedef void cbcRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL cbcCallSolver(cbcRec_t *Cptr);
	DllExport int  STDCALL cbcModifyProblem(cbcRec_t *Cptr);
	DllExport int  STDCALL cbcHaveModifyProblem(cbcRec_t *Cptr);
	DllExport int  STDCALL cbcReadyAPI(cbcRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL cbcFree(cbcRec_t **Cptr);
	DllExport void STDCALL cbcCreate(cbcRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSCBC_H_*/
