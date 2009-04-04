// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_H_
#define GAMSIPOPT_H_

typedef void ipoRec_t;
typedef struct gmoRec* gmoHandle_t;
typedef struct optRec* optHandle_t;
typedef struct gcdRec* gcdHandle_t;

#if defined(__cplusplus)
extern "C" {
#endif

  int    ipoCallSolver(ipoRec_t *Cptr);
  int    ipoModifyProblem(ipoRec_t *Cptr);
  int    ipoHaveModifyProblem(ipoRec_t *Cptr);
  int    ipoReadyAPI(ipoRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, gcdHandle_t Dptr);
  void   ipoFree(ipoRec_t **Cptr);
  void   ipoCreate(ipoRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSIPOPT_H_*/
