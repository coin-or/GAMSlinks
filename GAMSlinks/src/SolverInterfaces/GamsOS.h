// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOS_H_
#define GAMSOS_H_

typedef void os_Rec_t;
typedef struct gmoRec* gmoHandle_t;
typedef struct optRec* optHandle_t;
typedef struct gcdRec* gcdHandle_t;

#if defined(__cplusplus)
extern "C" {
#endif

  int    os_CallSolver(os_Rec_t *Cptr);
  int    os_ModifyProblem(os_Rec_t *Cptr);
  int    os_HaveModifyProblem(os_Rec_t *Cptr);
  int    os_ReadyAPI(os_Rec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, gcdHandle_t Dptr);
  void   os_Free(os_Rec_t **Cptr);
  void   os_Create(os_Rec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSOS_H_*/
