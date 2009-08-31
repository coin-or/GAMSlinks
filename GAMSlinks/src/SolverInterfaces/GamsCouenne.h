// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSCOUENNE_H_
#define GAMSCOUENNE_H_

#include "GamsSolver.h"

typedef void couRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL couCallSolver(couRec_t *Cptr);
	DllExport int  STDCALL couModifyProblem(couRec_t *Cptr);
	DllExport int  STDCALL couHaveModifyProblem(couRec_t *Cptr);
	DllExport int  STDCALL couReadyAPI(couRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL couFree(couRec_t **Cptr);
	DllExport void STDCALL couCreate(couRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSCOUENNE_H_*/
