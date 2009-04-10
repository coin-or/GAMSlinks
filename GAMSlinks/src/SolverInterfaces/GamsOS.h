// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOS_H_
#define GAMSOS_H_

#include "GamsSolver.h"

typedef void os_Rec_t;

#if defined(__cplusplus)
extern "C" {
#endif

  DllExport int  STDCALL os_CallSolver(os_Rec_t *Cptr);
  DllExport int  STDCALL os_ModifyProblem(os_Rec_t *Cptr);
  DllExport int  STDCALL os_HaveModifyProblem(os_Rec_t *Cptr);
  DllExport int  STDCALL os_ReadyAPI(os_Rec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, dctHandle_t Dptr);
  DllExport void STDCALL os_Free(os_Rec_t **Cptr);
  DllExport void STDCALL os_Create(os_Rec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOS_H_*/
