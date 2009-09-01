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

typedef void omsRec_t;

#if defined(__cplusplus)
extern "C" {
#endif

	DllExport int  STDCALL omsCallSolver(omsRec_t *Cptr);
	DllExport int  STDCALL omsModifyProblem(omsRec_t *Cptr);
	DllExport int  STDCALL omsHaveModifyProblem(omsRec_t *Cptr);
	DllExport int  STDCALL omsReadyAPI(omsRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr);
	DllExport void STDCALL omsFree(omsRec_t **Cptr);
	DllExport void STDCALL omsCreate(omsRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOSIMOSEK_H_*/
