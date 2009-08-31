// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSSOLVER_H_
#define GAMSSOLVER_H_

#if defined(_WIN32)
# if ! defined(STDCALL)
#  define STDCALL   __stdcall
# endif
# if ! defined(DllExport)
#  define DllExport __declspec( dllexport )
# endif
#else
# if ! defined(STDCALL)
#  define STDCALL
# endif
# if ! defined(DllExport)
#  define DllExport
# endif
#endif

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct optRec* optHandle_t;

#endif /*GAMSSOLVER_H_*/
