// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsCbc.hpp 629 2009-03-22 20:49:16Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSCBC_H_
#define GAMSCBC_H_

typedef void cbcRec_t;
typedef struct gmoRec* gmoHandle_t;
typedef struct optRec* optHandle_t;
typedef struct gcdRec* gcdHandle_t;

#if defined(__cplusplus)
extern "C" {
#endif

  int    cbcCallSolver(cbcRec_t *Cptr);
  int    cbcModifyProblem(cbcRec_t *Cptr);
  int    cbcHaveModifyProblem(cbcRec_t *Cptr);
  int    cbcReadyAPI(cbcRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, gcdHandle_t Dptr);
  void   cbcFree(cbcRec_t **Cptr);
  void   cbcCreate(cbcRec_t **Cptr, char *msgBuf, int msgBufLen);

#if defined(__cplusplus)
}
#endif

#endif /*GAMSCBC_H_*/
